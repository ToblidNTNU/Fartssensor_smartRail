#include "fft_modul.h"
#include "config.h" 
#include "ESP_fft.h"
#include "lidar_modul.h" 


// ── Interne buffere/variabler ───────────────────────────────
static float samples[FFT_N];
static float spectrum[FFT_N];
static float fart_buffer[BUFFER_SIZE];
static int   buffer_index  = 0;
static int   buffer_fyllt  = 0;   // Sporer hvor mange gyldige målinger som er lagt inn

float PEAK_SIZE = 2.0f; // Hvor mye større må en topp være enn snittet for å godtas
float MAG_GRENSE = 70.0f;
float SVILLE_TERSKEL = 15.0f; // Maks amplitude for å regne som sville i threshold-analyse

static ESP_fft* FFT = nullptr;

// ── Init ──────────────────────────────────────────────────────────────────────
void fft_init() {
    FFT = new ESP_fft(FFT_N, SAMPLEFREQ, FFT_REAL, FFT_FORWARD,
                      samples, spectrum);
    fft_buffer_nullstill();
}

// ── Hjelpefunksjoner ──────────────────────────────

static void samle_signal() {
    memset(samples, 0, sizeof(samples));  // nullstill først
    unsigned long intervall_us = 1000000 / SAMPLEFREQ;

    for (int i = 0; i < FFT_N; i++) {
        unsigned long start = micros();  // Ta tiden FØR lesing

        int avstand = 0;
        int styrke  = 0;

        if (lidar_les(avstand, styrke)) {
            if (lidar_modus == 0) samples[i] = (float)avstand;
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


//______fft-analyse____________________

float fft_analyse(){
    rens_signal();

    FFT->execute();
    FFT->complexToMagnitude();


    float min_hz = 1.0f / (3.6f * SVILL_AVSTAND);    // ~0.46 Hz = 1 km/h
    float max_hz = 100.0f / (3.6f * SVILL_AVSTAND);   // ~46 Hz = 100 km/h

    int min_bin = max(1, (int)(min_hz * FFT_N / SAMPLEFREQ));
    int max_bin = (int)(max_hz * FFT_N / SAMPLEFREQ);

    float maxMag = 0.0f;
    int   maxBin = min_bin;
    for (int i = min_bin; i <= max_bin; i++) {
        if (spectrum[i] > maxMag) {
            maxMag = spectrum[i];
            maxBin = i;
        }
    }
    float frekvens = maxBin * ((float)SAMPLEFREQ / FFT_N);
    float fart_kmh = frekvens * SVILL_AVSTAND * 3.6f;


    //Debug-utskrift av FFT-resultater
    
    Serial.printf("Fundamental Freq : %f Hz\t Mag: %f\n", frekvens, maxMag);
    for (int i=0; i< 10; i++) {
        Serial.printf("%f Hz: %f\n", FFT->frequency(i),spectrum[i]);
    }


    if (godkjent_signal(maxMag, min_bin, max_bin)) {
        return fart_kmh;

    } else {
        return -1.0f; // Indikerer ugyldig måling
    }
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

static bool godkjent_signal(float maxMag, int min_bin, int max_bin) {

    // Beregn gjennomsnittsverdi av alle bins
    float snitt_mag = 0.0f;
    for (int i = min_bin; i <= max_bin; i++) snitt_mag += spectrum[i];
    snitt_mag /= (max_bin - min_bin + 1);

    Serial.println("[fft_modul] Snitt mag: " + String(snitt_mag) + ", Peak mag: " + String(maxMag));
    // Godta kun topper som er betydelig over snittet og over absolutt mag-grense
    if (maxMag > snitt_mag * PEAK_SIZE && maxMag > MAG_GRENSE)
    {
        return true;
    } else { 
        return false;
    }
}


//______threshold-analyse____________________
float threshold_analyse() {
    int antall_sviller = 0;
    int vindu = SAMPLEFREQ / 100; // 10 ms vindu, juster etter behov
    bool forrige_var_sville = false;

    for (int i = 0; i < FFT_N - vindu; i++) {
        float maks = samples[i];
        float min  = samples[i];
        for (int j = i; j < i + vindu; j++) {
            if (samples[j] > maks) maks = samples[j];
            if (samples[j] < min)  min  = samples[j];
        }

        bool er_sville = (maks - min) < SVILLE_TERSKEL;

        // Tell kun én gang per sville (stigende kant)
        if (er_sville && !forrige_var_sville) antall_sviller++;
        forrige_var_sville = er_sville;
    }

    float tid_sek = (float)FFT_N / SAMPLEFREQ;
    return (antall_sviller * SVILL_AVSTAND / tid_sek) * 3.6f;
}

// ── Hoved-FFT-funksjon ────────────────────────────────────────────────────────
bool fft_kjor(float &fart_ut) {
    if (FFT == nullptr) {
        Serial.println("[fft_modul] FEIL: fft_init() ikke kalt!");
        return false;
    }

    rydd_buffer();
    samle_signal();
    
    float fart_kmh = 0.0f;
    switch (lidar_modus) {
        case 0: //avstand fft
            fart_kmh = fft_analyse();
            break;
        case 1: //styrke
            fart_kmh = fft_analyse();
            break;
        case 2: //flatt
            fart_kmh = threshold_analyse();
            break;
    }


    if (fart_kmh > 0.0f) {
        legg_i_buffer(fart_kmh);
        fart_ut = fart_kmh;
        return true;
    } else {
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

//annet

void fft_sett_parametere(float mag, float peak, float terskel) {
    MAG_GRENSE = mag;
    PEAK_SIZE = peak;
    SVILLE_TERSKEL = terskel;
}