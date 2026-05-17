#pragma once

/**
 * TargetProfiles.h
 * Define los umbrales ideales para cada perfil de tono (Ambient, Rhythm, Lead).
 * Valores basados en estándares de producción para guitarra eléctrica.
 */
namespace TargetProfiles {
// Una constante de -1000.0f indica que no hay límite (NULL)
constexpr float NO_LIMIT = -1000.0f;

constexpr float LUFS_000 = -30.0f;
constexpr float LUFS_100 = -5.0f;
constexpr float PLR_000 = 0.0f;
constexpr float PLR_100 = 20.0f;
constexpr float BRILLO_000 = 300.0f;
constexpr float BRILLO_100 = 3000.0f;
constexpr float CUERPO_000 = 0.0f;
constexpr float CUERPO_100 = 60.0f;
constexpr float CORTE_000 = 10.0f;
constexpr float CORTE_100 = 40.0f;
constexpr float BRILLO_RATIO_000 = 0.0f;
constexpr float BRILLO_RATIO_100 = 70.0f;

// =========================================================================
// AMBIENT PROFILE (Pads, Cleans etéreos)
// =========================================================================
constexpr float AMBIENT_LUFS_MIN = -22.0f;
constexpr float AMBIENT_LUFS_MAX = -16.0f;

constexpr float AMBIENT_PLR_MIN = 8.0f;
constexpr float AMBIENT_PLR_MAX = 15.0f;

constexpr float AMBIENT_BRILLO_MIN = 600.0f;
constexpr float AMBIENT_BRILLO_MAX = 1500.0f;

constexpr float AMBIENT_CUERPO_MIN = NO_LIMIT; // Sin mínimo
constexpr float AMBIENT_CUERPO_MAX = 15.0f;

constexpr float AMBIENT_CORTE_MIN = NO_LIMIT;
constexpr float AMBIENT_CORTE_MAX = 10.0f;

constexpr float AMBIENT_BRILLO_RATIO_MIN = 30.0f;
constexpr float AMBIENT_BRILLO_RATIO_MAX = 60.0f;

// =========================================================================
// RHYTHM PROFILE (Rock/Metal Chugging, Crunch)
// =========================================================================
constexpr float RHYTHM_LUFS_MIN = -18.0f;
constexpr float RHYTHM_LUFS_MAX = -14.0f;

constexpr float RHYTHM_PLR_MIN = 8.0f;
constexpr float RHYTHM_PLR_MAX = 12.0f;

constexpr float RHYTHM_BRILLO_MIN = 1200.0f;
constexpr float RHYTHM_BRILLO_MAX = 2000.0f;

constexpr float RHYTHM_CUERPO_MIN = 35.0f;
constexpr float RHYTHM_CUERPO_MAX = 45.0f;

constexpr float RHYTHM_CORTE_MIN = 15.0f;
constexpr float RHYTHM_CORTE_MAX = 25.0f;

constexpr float RHYTHM_BRILLO_RATIO_MIN = 20.0f;
constexpr float RHYTHM_BRILLO_RATIO_MAX = 40.0f;

// =========================================================================
// LEAD PROFILE (Solos, High Gain Sustain)
// =========================================================================
constexpr float LEAD_LUFS_MIN = -16.0f;
constexpr float LEAD_LUFS_MAX = -12.0f;

constexpr float LEAD_PLR_MIN = 10.0f;
constexpr float LEAD_PLR_MAX = 14.0f;

constexpr float LEAD_BRILLO_MIN = 1900.0f;
constexpr float LEAD_BRILLO_MAX = 2700.0f;

constexpr float LEAD_CUERPO_MIN = 20.0f;
constexpr float LEAD_CUERPO_MAX = 30.0f;

constexpr float LEAD_CORTE_MIN = 30.0f;
constexpr float LEAD_CORTE_MAX = 50.0f;

constexpr float LEAD_BRILLO_RATIO_MIN = 15.0f;
constexpr float LEAD_BRILLO_RATIO_MAX = 35.0f;

/**
 * Utilidad para validar si un valor está dentro del rango,
 * respetando la constante NO_LIMIT.
 */
inline bool isValid(float val, float min, float max) {
  bool minOk = (min == NO_LIMIT || val >= min);
  bool maxOk = (max == NO_LIMIT || val <= max);
  return minOk && maxOk;
}
} // namespace TargetProfiles
