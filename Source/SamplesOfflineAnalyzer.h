#pragma once
#include <JuceHeader.h>
#include "AnalysisEngine.h"
#include "TargetProfiles.h"

struct SampleAnalysisResult
{
    juce::String sourceFile;
    juce::String targetCategory; // "Ambient", "Rhythm", "Lead"
    float lufs = -70.0f;
    float plr = 0.0f;
    float brightness = 0.0f;
    float bodyRatio = 0.0f;
    float cutRatio = 0.0f;
    float brilloRatio = 0.0f;
    std::vector<float> fftCurve;
    bool success = false;
};

class SamplesOfflineAnalyzer
{
public:
    static SampleAnalysisResult analyzeFile(const juce::File& file, const juce::String& targetCategory)
    {
        SampleAnalysisResult result;
        result.sourceFile = file.getFileName();
        result.targetCategory = targetCategory;

        juce::AudioFormatManager formatManager;
        formatManager.registerBasicFormats();

        std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(file));
        if (reader == nullptr)
        {
            result.success = false;
            return result;
        }

        double sampleRate = reader->sampleRate;
        int64_t numSamples = reader->lengthInSamples;
        int numChannels = reader->numChannels;

        if (numSamples <= 0 || sampleRate <= 0.0)
        {
            result.success = false;
            return result;
        }

        // Leer todo el archivo en un buffer de audio
        juce::AudioBuffer<float> buffer(numChannels, static_cast<int>(numSamples));
        reader->read(&buffer, 0, static_cast<int>(numSamples), 0, true, true);

        // Crear una instancia temporal de AnalysisEngine
        AnalysisEngine offlineAnalyzer;
        offlineAnalyzer.prepare(sampleRate);

        // Configurar los límites de frecuencia de brillo según la categoría
        if (targetCategory.equalsIgnoreCase("Ambient"))
        {
            offlineAnalyzer.targetBrilloMin.store(TargetProfiles::AMBIENT_BRILLO_MIN);
            offlineAnalyzer.targetBrilloMax.store(TargetProfiles::AMBIENT_BRILLO_MAX);
        }
        else if (targetCategory.equalsIgnoreCase("Lead"))
        {
            offlineAnalyzer.targetBrilloMin.store(TargetProfiles::LEAD_BRILLO_MIN);
            offlineAnalyzer.targetBrilloMax.store(TargetProfiles::LEAD_BRILLO_MAX);
        }
        else // Rhythm o default
        {
            offlineAnalyzer.targetBrilloMin.store(TargetProfiles::RHYTHM_BRILLO_MIN);
            offlineAnalyzer.targetBrilloMax.store(TargetProfiles::RHYTHM_BRILLO_MAX);
        }

        // Procesar por bloques de 512 muestras
        int blockSize = 512;
        for (int pos = 0; pos < numSamples; pos += blockSize)
        {
            int samplesToProcess = std::min(blockSize, static_cast<int>(numSamples - pos));
            
            // Creamos un subbuffer del bloque
            juce::AudioBuffer<float> blockBuffer(buffer.getNumChannels(), samplesToProcess);
            for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            {
                blockBuffer.copyFrom(ch, 0, buffer, ch, pos, samplesToProcess);
            }

            // Alimentar el bloque
            offlineAnalyzer.processBlock(blockBuffer, 0, 0, true);
        }

        // Obtener los valores resultantes
        result.lufs = offlineAnalyzer.integratedLUFS.load();
        result.plr = offlineAnalyzer.currentPLR.load();
        result.brightness = offlineAnalyzer.averageCentroidHz.load();
        result.bodyRatio = offlineAnalyzer.averageBodyRatio.load();
        result.cutRatio = offlineAnalyzer.averageCutRatio.load();
        result.brilloRatio = offlineAnalyzer.averageBrilloRatio.load();
        
        result.fftCurve.resize(offlineAnalyzer.latestFFTData.size());
        std::copy(offlineAnalyzer.latestFFTData.begin(), offlineAnalyzer.latestFFTData.end(), result.fftCurve.begin());

        result.success = true;

        return result;
    }

    static bool saveAnalysisResult(const SampleAnalysisResult& result, const juce::File& targetFile)
    {
        juce::DynamicObject::Ptr jsonObj = new juce::DynamicObject();
        jsonObj->setProperty("source_file", result.sourceFile);
        jsonObj->setProperty("target_profile_category", result.targetCategory);
        jsonObj->setProperty("timestamp", juce::Time::getCurrentTime().toString(true, true));
        
        juce::DynamicObject::Ptr metricsObj = new juce::DynamicObject();
        metricsObj->setProperty("lufs", result.lufs);
        metricsObj->setProperty("plr", result.plr);
        metricsObj->setProperty("brightness", result.brightness);
        metricsObj->setProperty("body_ratio", result.bodyRatio);
        metricsObj->setProperty("cut_ratio", result.cutRatio);
        metricsObj->setProperty("brillo_ratio", result.brilloRatio);

        juce::Array<juce::var> fftVarArray;
        for (float val : result.fftCurve) {
            fftVarArray.add(val);
        }
        metricsObj->setProperty("fft_curve", fftVarArray);
        
        jsonObj->setProperty("metrics", juce::var(metricsObj.get()));

        juce::var jsonVar(jsonObj.get());
        juce::String jsonString = juce::JSON::toString(jsonVar);

        if (targetFile.existsAsFile())
            targetFile.deleteFile();

        std::unique_ptr<juce::FileOutputStream> fos(targetFile.createOutputStream());
        if (fos != nullptr)
        {
            fos->writeString(jsonString);
            return true;
        }

        return false;
    }

    static SampleAnalysisResult loadAnalysisResult(const juce::File& file)
    {
        SampleAnalysisResult result;
        result.success = false;

        juce::String jsonText = file.loadFileAsString();
        juce::var jsonVar;
        juce::Result jsonParseResult = juce::JSON::parse(jsonText, jsonVar);

        if (jsonParseResult.wasOk() && jsonVar.isObject())
        {
            if (auto* obj = jsonVar.getDynamicObject())
            {
                result.sourceFile = obj->getProperty("source_file").toString();
                result.targetCategory = obj->getProperty("target_profile_category").toString();
                
                if (obj->hasProperty("metrics"))
                {
                    juce::var metricsVar = obj->getProperty("metrics");
                    if (metricsVar.isObject())
                    {
                        if (auto* mObj = metricsVar.getDynamicObject())
                        {
                            result.lufs = static_cast<float>(mObj->getProperty("lufs"));
                            result.plr = static_cast<float>(mObj->getProperty("plr"));
                            result.brightness = static_cast<float>(mObj->getProperty("brightness"));
                            result.bodyRatio = static_cast<float>(mObj->getProperty("body_ratio"));
                            result.cutRatio = static_cast<float>(mObj->getProperty("cut_ratio"));
                            result.brilloRatio = mObj->hasProperty("brillo_ratio") ? static_cast<float>(mObj->getProperty("brillo_ratio")) : 0.0f;

                            if (mObj->hasProperty("fft_curve")) {
                                juce::var fftVar = mObj->getProperty("fft_curve");
                                if (auto* arr = fftVar.getArray()) {
                                    result.fftCurve.clear();
                                    for (int i = 0; i < arr->size(); ++i) {
                                        result.fftCurve.push_back(static_cast<float>((*arr)[i]));
                                    }
                                }
                            }
                            result.success = true;
                        }
                    }
                }
            }
        }

        return result;
    }
};
