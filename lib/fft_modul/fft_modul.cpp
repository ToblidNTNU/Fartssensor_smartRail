#include "fft_modul.h"
#include "config.h" 
#include "ESP_fft.h"
#include "lidar_modul.h" 
#include "mqtt_modul.h"


// ── Interne buffere/variabler ───────────────────────────────
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

// ── Hjelpefunksjoner ──────────────────────────────
static void samle_signal() {
    unsigned long intervall_us = 1000000 / SAMPLEFREQ;

    for (int i = 0; i < FFT_N; i++) {
        unsigned long start = micros();  // Ta tiden FØR lesing

        int avstand = 0;
        int styrke  = 0;

        if (lidar_les(avstand, styrke)) {
            if (lidar_modus) samples[i] = (float)avstand;
            else samples[i] = (float)styrke;
        } else {
            samples[i] = 0.0f;
        }

        unsigned long etter_les = micros();

        // Vent gjenværende av intervallet
        long resterende = intervall_us - (etter_les - start);
        if (resterende > 0) delayMicroseconds(resterende);
    }        
}

static void legg_i_buffer(float verdi) {
    fart_buffer[buffer_index] = verdi;
    buffer_index = (buffer_index + 1) % BUFFER_SIZE;   // Sirkulær – overskriver eldste
    if (buffer_fyllt < BUFFER_SIZE) buffer_fyllt++;
}

static void rens_signal(void) {

    // Beregn gjennomsnitt og standardavvik for normalisering
    float sum = 0.0f;
        for (int i = 0; i < FFT_N; i++) sum += samples[i];
        float snitt = sum / FFT_N;

        float varians = 0.0f;
        for (int i = 0; i < FFT_N; i++) {
            float diff = samples[i] - snitt;
            varians += diff * diff;
        }
        float stddev = sqrt(varians / FFT_N);

        // Normaliser
        if (stddev > 0.0f) {
            for (int i = 0; i < FFT_N; i++) {
                samples[i] = (samples[i] - snitt) / stddev;
            }
        }
    
    FFT->hammingWindow();
}

static bool godkjent_signal(float maxMag) {

    // Beregn gjennomsnittsverdi av alle bins
    float snitt_mag = 0.0f;
    for (int i = 1; i < FFT_N / 2; i++) snitt_mag += spectrum[i];
    snitt_mag /= (FFT_N / 2 - 1); // unntatt DC-komponenten

    // Godta kun topper som er betydelig over snittet og over absolutt mag-grense
    if (maxMag > snitt_mag * 3.0f && maxMag > MAG_GRENSE)
    {
        return true;
    } else { 
        return false;
    }
}

// ── Hoved-FFT-funksjon ────────────────────────────────────────────────────────
bool fft_kjor(float &fart_ut) {
    if (FFT == nullptr) {
        Serial.println("[fft_modul] FEIL: fft_init() ikke kalt!");
        return false;
    }

    rydd_buffer();
    samle_signal();
    rens_signal();

    FFT->execute();
    FFT->complexToMagnitude();

    float maxMag = (FFT->majorPeak() / 10000.0f) * 2.0f / FFT_N;
    float frekvens = FFT->majorPeakFreq();
    float fart_kmh = frekvens * SVILL_AVSTAND * 3.6f;


    //Debug-utskrift av FFT-resultater
    Serial.printf("Fundamental Freq : %f Hz\t Mag: %f g\n", FFT->majorPeakFreq(), (FFT->majorPeak()/10000)*2/FFT_N);
    for (int i=0; i< 20; i++) {
        Serial.printf("%f Hz: %f\n", FFT->frequency(i),spectrum[i]);
    }


    if (godkjent_signal(maxMag)) {
        
        legg_i_buffer(fart_kmh);
        fart_ut = fart_kmh;
        return true;

    } else {
        
        legg_i_buffer(0.0f);
        fart_ut = 0.0f;
        return false;
    }
    
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