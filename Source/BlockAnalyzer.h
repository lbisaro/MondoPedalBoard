#pragma once
#include <JuceHeader.h>
#include <atomic>
#include <array>
#include <vector>
#include <algorithm>
#include <cmath>

// =============================================================================
// BlockAnalyzer -- Real-Time Ultra-Fast EQ Curve Analyzer
//
// Optimized for instantaneous response (22 updates per second) and absolute
// stability, replicating the professional real-time feel of Waves Q-Clone.
//
// Key Principles:
//   1. Continuous Periodic Sweep of 2048 samples (approx 46 ms at 44.1 kHz).
//   2. Circular Deconvolution: Since the signal is periodic and repeating,
//      we convolve Y(f) with the exact inverse X_inv(f) directly.
//   3. Time-Gating: Transforms the result to the time domain, isolates the
//      EQ impulse response (448 samples window around the peak), and zeroes out
//      the rest to eliminate all high-frequency digital/analog noise.
//   4. Buttery-smooth EMA tracking for zero jitter and organic knob response.
// =============================================================================
class BlockAnalyzer
{
public:
    static constexpr int    sweepLengthSamples = 2048; // ~46 ms cycle for instant updates
    static constexpr double kSweepStartHz      = 20.0;
    static constexpr double kSweepEndHz        = 20000.0;
    static constexpr int    numBins            = 512;

    BlockAnalyzer() = default;

    // -------------------------------------------------------------------------
    void prepare (double sampleRate)
    {
        currentSampleRate = (sampleRate > 0.0) ? sampleRate : 44100.0;

        buildSweepAndInverseFilter();

        captureBuffer.assign (sweepLengthSamples, 0.0f);
        sweepWritePos = 0;
        capturePos    = 0;
        newCycleFlag.store (false);

        {
            juce::ScopedLock sl (resultLock);
            magnitudeDb.fill (0.0f);
            resultReady.store (false);
        }
    }

    // Returns the next sweep sample for the output channel.
    float getNextSweepSample()
    {
        if (sweepBuffer.empty()) return 0.0f;
        
        // Output at moderate amplitude (-12 dBFS)
        float s = sweepBuffer[sweepWritePos] * 0.25f;
        
        if (++sweepWritePos >= sweepLengthSamples)
        {
            sweepWritePos = 0;
            newCycleFlag.store (true);
        }
        return s;
    }

    // Feeds one processed sample from the Helix return into the capture buffer.
    void captureSample (float proc)
    {
        if (newCycleFlag.exchange (false))
        {
            if (capturePos == sweepLengthSamples)
                computeTransferFunction();
            capturePos = 0;
        }

        if (capturePos < sweepLengthSamples)
            captureBuffer[capturePos++] = proc;
    }

    void decay()
    {
        juce::ScopedLock sl (resultLock);
        for (auto& v : magnitudeDb) v *= 0.95f;
    }

    void getMagnitudeDb (std::array<float, numBins>& out) const
    {
        juce::ScopedLock sl (resultLock);
        std::copy (magnitudeDb.begin(), magnitudeDb.end(), out.begin());
    }

    bool hasResult()              const { return resultReady.load(); }
    int  getSweepLengthSamples()  const { return sweepLengthSamples; }

    void reset()
    {
        sweepWritePos = 0;
        capturePos    = 0;
        newCycleFlag.store (false);
        if (! captureBuffer.empty())
            std::fill (captureBuffer.begin(), captureBuffer.end(), 0.0f);
        juce::ScopedLock sl (resultLock);
        magnitudeDb.fill (0.0f);
        resultReady.store (false);
    }

private:
    // -------------------------------------------------------------------------
    // Build the periodic exponential sweep and its exact circular inverse
    // -------------------------------------------------------------------------
    void buildSweepAndInverseFilter()
    {
        sweepBuffer.resize (sweepLengthSamples);
        const double T = static_cast<double> (sweepLengthSamples) / currentSampleRate;
        const double f1 = kSweepStartHz;
        const double f2 = kSweepEndHz;
        const double R = std::log (f2 / f1);

        for (int n = 0; n < sweepLengthSamples; ++n)
        {
            double t = static_cast<double> (n) / currentSampleRate;
            double phase = 2.0 * juce::MathConstants<double>::pi
                           * (f1 * T / R) * (std::exp (t * R / T) - 1.0);
            sweepBuffer[n] = static_cast<float> (std::sin (phase));
        }

        // Apply a very micro 1 ms fade to prevent wrap-around clicking while keeping periodicity
        int fade = std::min (static_cast<int> (0.001 * currentSampleRate), sweepLengthSamples / 16);
        for (int i = 0; i < fade; ++i)
        {
            float w = static_cast<float> (i) / fade;
            sweepBuffer[i]                           *= w;
            sweepBuffer[sweepLengthSamples - 1 - i]  *= w;
        }

        // Compute exact Inverse Filter in the frequency domain
        juce::dsp::FFT fft (fftOrder);
        std::vector<float> sweepFFT (sweepLengthSamples * 2, 0.0f);
        std::copy (sweepBuffer.begin(), sweepBuffer.end(), sweepFFT.begin());
        fft.performRealOnlyForwardTransform (sweepFFT.data());

        invFFTComplex.assign (sweepLengthSamples * 2, 0.0f);
        const double nyq = currentSampleRate / 2.0;

        for (int k = 0; k <= sweepLengthSamples / 2; ++k)
        {
            double freq = static_cast<double> (k) * currentSampleRate / static_cast<double> (sweepLengthSamples);

            float re = sweepFFT[2 * k];
            float im = sweepFFT[2 * k + 1];
            float power = re * re + im * im;

            // Tikhonov regularizer
            float eps = 1e-4f;

            // Keep clean band limits matching the sweep range
            float bp = 1.0f;
            if (freq < 15.0)
                bp = static_cast<float> (std::max (0.0, (freq - 5.0) / 10.0));
            else if (freq > 21000.0)
                bp = static_cast<float> (std::max (0.0, (nyq - freq) / (nyq - 21000.0)));

            float denom = power + eps;
            invFFTComplex[2 * k]     = (re / denom) * bp;
            invFFTComplex[2 * k + 1] = (-im / denom) * bp;
        }
    }

    // -------------------------------------------------------------------------
    // Real-Time Circular Deconvolution & Noise Gating
    // -------------------------------------------------------------------------
    void computeTransferFunction()
    {
        juce::dsp::FFT fft (fftOrder);

        // 1. Forward FFT of the repeating cycle
        std::vector<float> capFFT (sweepLengthSamples * 2, 0.0f);
        std::copy (captureBuffer.begin(), captureBuffer.end(), capFFT.begin());
        fft.performRealOnlyForwardTransform (capFFT.data());

        // 2. Complex multiplication with pre-computed inverse spectrum
        std::vector<float> irSpec (sweepLengthSamples * 2, 0.0f);
        for (int k = 0; k <= sweepLengthSamples / 2; ++k)
        {
            float yr = capFFT[2 * k],         yi = capFFT[2 * k + 1];
            float fr = invFFTComplex[2 * k],  fi = invFFTComplex[2 * k + 1];

            irSpec[2 * k]     = yr * fr - yi * fi;
            irSpec[2 * k + 1] = yr * fi + yi * fr;
        }

        // 3. IFFT to obtain the periodic Impulse Response
        fft.performRealOnlyInverseTransform (irSpec.data());

        // 4. Find peak of the periodic IR (latency immune)
        float maxVal = 0.0f;
        int peakIdx = 0;
        for (int i = 0; i < sweepLengthSamples; ++i)
        {
            float val = std::abs (irSpec[i]);
            if (val > maxVal)
            {
                maxVal = val;
                peakIdx = i;
            }
        }

        // 5. Apply smooth time gate (compuerta temporal) to zero out noise
        // We capture 64 samples before the peak and 384 samples after
        const int gateLeft  = 64;
        const int gateRight = 384;
        const int gateSize  = gateLeft + gateRight; // 448 samples

        std::vector<float> gatedIR (sweepLengthSamples * 2, 0.0f);

        for (int i = 0; i < sweepLengthSamples; ++i)
        {
            int offset = i - gateLeft;
            int srcIdx = (peakIdx + offset + sweepLengthSamples) % sweepLengthSamples;

            float sample = irSpec[srcIdx];

            // Smooth Tukey window for the gate
            float w = 0.0f;
            if (i < gateSize)
            {
                w = 1.0f;
                if (i < 32)
                    w = 0.5f * (1.0f - std::cos (juce::MathConstants<float>::pi * i / 32.0f));
                else if (i > gateSize - 64)
                    w = 0.5f * (1.0f - std::cos (juce::MathConstants<float>::pi * (gateSize - i) / 64.0f));
            }

            gatedIR[i] = sample * w;
        }

        // 6. Forward FFT of Gated IR
        fft.performRealOnlyForwardTransform (gatedIR.data());

        // 7. Calculate magnitude dB
        std::array<float, numBins> newMag;
        const double nyq = currentSampleRate / 2.0;
        const int halfBins = sweepLengthSamples / 2;

        for (int b = 0; b < numBins; ++b)
        {
            double norm = static_cast<double> (b) / static_cast<double> (numBins - 1);
            double freq = kSweepStartHz * std::pow (kSweepEndHz / kSweepStartHz, norm);
            freq = std::min (freq, nyq * 0.999);

            int k = static_cast<int> (freq / nyq * static_cast<double> (halfBins));
            k = juce::jlimit (1, halfBins - 1, k);

            float re = gatedIR[2 * k];
            float im = gatedIR[2 * k + 1];
            float mag = std::sqrt (re * re + im * im) / static_cast<float> (sweepLengthSamples);

            newMag[b] = juce::Decibels::gainToDecibels (mag, -120.0f);
        }

        // 8. Log-frequency smoothing (1/12-octave window)
        {
            constexpr int W = 3;
            std::array<float, numBins> sm;
            for (int b = 0; b < numBins; ++b)
            {
                int lo = std::max (0, b - W);
                int hi = std::min (numBins - 1, b + W);
                float s = 0.0f;
                for (int j = lo; j <= hi; ++j) s += newMag[j];
                sm[b] = s / static_cast<float> (hi - lo + 1);
            }
            newMag = sm;
        }

        // 9. Subtract 45th percentile to keep the flat baseline perfectly at 0 dB
        {
            std::array<float, numBins> sorted = newMag;
            std::sort (sorted.begin(), sorted.end());
            float offset = sorted[numBins * 45 / 100];
            for (auto& v : newMag)
                v = juce::jlimit (-64.0f, 24.0f, v - offset);
        }

        // 10. Temporal EMA smoothing for fluid, analog-style movement
        {
            juce::ScopedLock sl (resultLock);
            if (resultReady.load())
            {
                // Moderate tracking speed (0.35 EMA) for ultra-snappy yet smooth movement
                const float ema = 0.35f;
                for (int i = 0; i < numBins; ++i)
                    magnitudeDb[i] = ema * magnitudeDb[i] + (1.0f - ema) * newMag[i];
            }
            else
            {
                magnitudeDb = newMag;
                resultReady.store (true);
            }
        }
    }

    // -------------------------------------------------------------------------
    double currentSampleRate  = 44100.0;
    static constexpr int fftOrder = 11; // 2^11 = 2048 samples

    std::vector<float> sweepBuffer;
    std::vector<float> invFFTComplex;

    std::vector<float>   captureBuffer;
    int                  sweepWritePos = 0;
    int                  capturePos    = 0;
    std::atomic<bool>    newCycleFlag  { false };

    mutable juce::CriticalSection  resultLock;
    std::array<float, numBins>     magnitudeDb {};
    std::atomic<bool>              resultReady { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BlockAnalyzer)
};
