#pragma once
#include <Arduino.h>

// ── Konfigurasjon ─────────────────────────────────────────────────────────────
// Endre disse etter behov
#define FFT_N           1024     // Antall samples – må være potens av 2
#define SAMPLEFREQ      300      // Samplingsfrekvens i Hz
#define BUFFER_SIZE     10      // Antall målinger i glidende buffer
#define SVILL_AVSTAND   0.6f    // Avstand mellom sensor og vei i meter

extern uint8_t peakSize;
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