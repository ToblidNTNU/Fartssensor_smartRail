#include "fft_modul.h"
#include "config.h" 
#include "ESP_fft.h"
#include "lidar_modul.h" 
#include "mqtt_modul.h"

/*
*  fft_modul.cpp - Modul for å håndtere FFT-beregninger for fartsmåling
*  Denne modulen samler inn signaldata fra en sensor, utfører FFT for å finne den
*  dominerende frekvensen, og konverterer denne til en fartsmåling i km/t. Resultatene
*  lagres i en sirkulær buffer for senere bruk, f.eks. for å beregne gjennomsnittsfart 
*  eller for å vise på en display.
*  Forutsetninger:
*  - Sensoren gir et digitalt signal som kan leses med digitalRead() (kan tilpasses for analogRead() eller annen sensor)
*  - FFT_N og SAMPLEFREQ er definert i config.h
*  - SVILL_AVSTAND er avstanden mellom to sviller i meter, definert i config.h
*  - MAG_GRENSE er en magnisitetsgrense for å avgjøre om det er et gyldig signal (bil) eller ikke, definert i config.h
*  - BUFFER_SIZE er størrelsen på den sirkulære bufferen for fartsmålinger, definert i config.h
*  Bruk:
*  - Kall fft_init() i setup() for å initialisere FFT-modulen
*  - Kall fft_kjor() i loop() for å utføre FFT og få fartsmålingen. Den returnerer true hvis en gyldig måling er gjort, og false hvis signalet var for svakt.
*  - Bruk fft_buffer_snitt() for å få gjennomsnittsfarten basert på de siste målingene i bufferen, og fft_buffer_nullstill() for å tømme bufferen ved behov.
*/


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
    unsigned long total_start = micros();
    unsigned long intervall_us = 1000000 / SAMPLEFREQ;

    for (int i = 0; i < FFT_N; i++) {
        unsigned long start = micros();  // Ta tiden FØR lesing

        int avstand = 0;
        int styrke  = 0;

        if (lidar_les(avstand, styrke)) {
            if (lidar_modus) samples[i] = (float)avstand;
            else if (!lidar_modus) samples[i] = (float)styrke;
        } else {
            samples[i] = 0.0f;
        }

        unsigned long etter_les = micros();


        // Vent kun gjenværende tid av intervallet
        long resterende = intervall_us - (etter_les - start);
        if (resterende > 0) delayMicroseconds(resterende);


        /* for sjekking av timing
        if (i < 3) {
            Serial.printf("Sample %d: les=%lu µs, resterende=%ld µs, total=%lu µs\n",
                i, etter_les - start, resterende, micros() - start);
        }
        */
    }
    //for sjekking av total timing
    /*
    Serial.printf("Totalt: %lu ms for %d samples\n", 
        (micros() - total_start) / 1000, FFT_N);
        */
    
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

    rydd_buffer();
    samle_signal();

    // Etter samle_signal(), før FFT->hammingWindow()
    // Beregn gjennomsnitt og standardavvik
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
    FFT->removeDC();
    FFT->execute();
    FFT->complexToMagnitude();

    float maxMag = (FFT->majorPeak() / 10000.0f) * 2.0f / FFT_N;

    // Signal for svakt – sannsynligvis ingen bil
    if (maxMag < MAG_GRENSE) {
        legg_i_buffer(0.0f); // -> finne annen håndtering av ugyldige måleringer

        /* Hvis vi vil returnere den målte farten i stedet for å ignorere målingen, kan vi gjøre det her:
        float frekvens = FFT->majorPeakFreq();
        float fart_kmh = frekvens * SVILL_AVSTAND * 3.6f;

        legg_i_buffer(fart_kmh);
        fart_ut = fart_kmh;
        */
        return false;
    }

    float frekvens = FFT->majorPeakFreq();
    float fart_kmh = frekvens * SVILL_AVSTAND * 3.6f;


    //Debug-utskrift av FFT-resultater
    /*
    Serial.printf("Fundamental Freq : %f Hz\t Mag: %f g\n", FFT->majorPeakFreq(), (FFT->majorPeak()/10000)*2/FFT_N);
    for (int i=0; i< 10; i++) {
        Serial.printf("%f Hz: %f\n", FFT->frequency(i),spectrum[i]);
    }*/

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