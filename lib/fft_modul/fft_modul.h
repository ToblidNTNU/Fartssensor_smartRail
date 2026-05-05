#pragma once
#include <Arduino.h>

// ── Konfigurasjon ─────────────────────────────────────────────────────────────
// ── Parametervalg ──────────────────────────────────────────────────────────
// SAMPLEFREQ > 10*v/0.6 → 500 Hz dekker 0-100 km/h (Nyquist med god margin)
// FFT_N/SAMPLEFREQ > 6/v(m/s) → 1024/500=2.05s dekker v > ~10 km/h (>10 sviller)
// For v < 10 km/h reduseres antall sviller, men frekvensdeteksjon er fortsatt mulig
#define FFT_N           1024     // Antall samples – må være potens av 2 - N/F må være større enn 6/v
#define SAMPLEFREQ      500      // Samplingsfrekvens i Hz - må være større enn 10*v/0.6
#define BUFFER_SIZE     10      // Antall målinger i glidende buffer
#define SVILL_AVSTAND   0.6f    // Avstand mellom sensor og vei i meter

extern float PEAK_SIZE;
extern float MAG_GRENSE;
extern float SVILLE_TERSKEL;

// ── Offentlig API ─────────────────────────────────────────────────────────────

/**
 * Initialiser FFT-modulen. Kall én gang i setup().
 */
void fft_init();

/**
 * Samle inn signal, kjør FFT og beregn fart.
 *
 * @param fart_ut  Settes til beregnet fart i km/h hvis målingen er gyldig.
 * @return         true hvis gyldig fart ble beregnet, false hvis signal var for svakt.
 */
bool fft_kjor(float &fart_ut);

/**
 * Returner gjennomsnittet av fart-bufferen.
 * Nyttig for å glatte ut målinger over tid.
 *
 * @return  Gjennomsnittlig fart i km/h.
 */
float fft_buffer_snitt();

/**
 * Nullstill fart-bufferen.
 */
void fft_buffer_nullstill();


// fft_modul.h
void fft_sett_parametere(float mag, uint8_t peak, float terskel);