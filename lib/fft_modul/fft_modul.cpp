#include "fft_modul.h"
#include "config.h" 
#include "ESP_fft.h"

// ── Interne buffere (kun synlige i denne filen) ───────────────────────────────
static float samples[FFT_N];
static float spectrum[FFT_N];
static float fart_buffer[BUFFER_SIZE];
static int   buffer_index  = 0;
static int   buffer_fyllt  = 0;   // Sporer hvor mange gyldige målinger som er lagt inn

static ESP_fft* FFT = nullptr;

// ── Init ──────────────────────────────────────────────────────────────────────
void fft_init() {
    FFT = new ESP_fft(FFT_N, SAMPLEFREQ, FFT_REAL, FFT_FORWARD,
                      samples, spectrum);
    fft_buffer_nullstill();
}

// ── Hjelpefunksjon: samle inn samples fra sensor ──────────────────────────────
static void samle_signal() {
    for (int i = 0; i < FFT_N; i++) {
        // TODO: bytt ut med faktisk sensorlesing, f.eks. analogRead(SENSOR_PIN)
        samples[i] = (digitalRead(SENSOR_PIN) ? 1.0f : -1.0f) * 5.0f;
        delayMicroseconds(1000000 / SAMPLEFREQ);
    }
}

// ── Hjelpefunksjon: legg verdi i sirkulær buffer ──────────────────────────────
static void legg_i_buffer(float verdi) {
    fart_buffer[buffer_index] = verdi;
    buffer_index = (buffer_index + 1) % BUFFER_SIZE;   // Sirkulær – overskriver eldste
    if (buffer_fyllt < BUFFER_SIZE) buffer_fyllt++;
}

// ── Hoved-FFT-funksjon ────────────────────────────────────────────────────────
bool fft_kjor(float &fart_ut) {
    if (FFT == nullptr) {
        Serial.println("[fft_modul] FEIL: fft_init() ikke kalt!");
        return false;
    }

    samle_signal();

    FFT->hammingWindow();
    FFT->removeDC();
    FFT->execute();
    FFT->complexToMagnitude();

    float maxMag = (FFT->majorPeak() / 10000.0f) * 2.0f / FFT_N;

    // Signal for svakt – sannsynligvis ingen bil
    if (maxMag < MAG_GRENSE) {
        //legg_i_buffer(0.0f); -> finne annen håndtering av ugyldige måleringer
        return false;
    }

    float frekvens = FFT->majorPeakFreq();
    float fart_kmh = frekvens * SVILL_AVSTAND * 3.6f;

    legg_i_buffer(fart_kmh);
    fart_ut = fart_kmh;
    return true;
}

// ── Buffer-hjelpefunksjoner ───────────────────────────────────────────────────
float fft_buffer_snitt() {
    if (buffer_fyllt == 0) return 0.0f;
    float sum = 0.0f;
    for (int i = 0; i < buffer_fyllt; i++) sum += fart_buffer[i];
    return sum / buffer_fyllt;
}

void fft_buffer_nullstill() {
    for (int i = 0; i < BUFFER_SIZE; i++) fart_buffer[i] = 0.0f;
    buffer_index = 0;
    buffer_fyllt = 0;
}