#include "lidar_modul.h"
#include "config.h"

// ── Konfigurasjon ─────────────────────────────────────────────────────────────
#define LIDAR_BAUD      460800      // 460800 baud for å håndtere 1000 Hz
#define LIDAR_UGYLDIG   45000       // Sensorens verdi for ugyldig måling

// ── Interne variabler ─────────────────────────────────────────────────────────
static HardwareSerial lidarSerial(2);
static uint8_t        uart_buf[9];

// ── Interne hjelpefunksjoner ──────────────────────────────────────────────────
static void sett_frekvens(int hz) {

    uint8_t lo = hz & 0xFF;
    uint8_t hi = (hz >> 8) & 0xFF;
    uint8_t cmd[] = {0x5A, 0x06, 0x03, lo, hi, 0x00};

    lidarSerial.write(cmd, sizeof(cmd));
    delay(100);
    Serial.printf("[lidar] Frekvens satt til %d Hz", hz);
}

static void sett_millimeter_modus() {
    uint8_t cmd[] = {0x5A, 0x05, 0x05, 0x06, 0x6A};
    lidarSerial.write(cmd, sizeof(cmd));
    delay(100);
    Serial.println("[lidar] Millimeter-modus aktivert");
}



// ── Offentlige funksjoner ─────────────────────────────────────────────────────


void rydd_buffer() {
    while (lidarSerial.available()) lidarSerial.read();
}


void lidar_init(int hz) {
    lidarSerial.begin(LIDAR_BAUD, SERIAL_8N1, LIDAR_RX_PIN, LIDAR_TX_PIN);
    delay(500);
    rydd_buffer();

    sett_millimeter_modus();
    sett_frekvens(hz);
    delay(200);
    rydd_buffer();

    Serial.printf("[lidar] TF02-Pro klar (%d Hz, 460800 baud, mm-modus)\n", hz);
}

bool lidar_les(int &avstand_ut, int &styrke_ut) {

    // Vent på startbyte, med timeout
    unsigned long timeout = micros() + 3000;
    while (micros() < timeout) {
        // Kast bytes frem til 0x59
        while (lidarSerial.available() && lidarSerial.peek() != 0x59) {
            lidarSerial.read();
        }
        // Sjekk om vi har startbyte OG full pakke
        if (lidarSerial.available() >= 9 && lidarSerial.peek() == 0x59) break;
    }

    if (lidarSerial.available() < 9) return false;

    // Les pakken
    for (int i = 0; i < 9; i++) {
        uart_buf[i] = lidarSerial.read();
    }

    // Sjekk header-bytes
    if (uart_buf[0] != 0x59 || uart_buf[1] != 0x59) return false;

    // Sjekk checksum
    uint8_t checksum = 0;
    for (int i = 0; i < 8; i++) checksum += uart_buf[i];
    if (uart_buf[8] != checksum) return false;

    int avstand = uart_buf[2] + (uart_buf[3] * 256);
    int styrke  = uart_buf[4] + (uart_buf[5] * 256);

    if (avstand == LIDAR_UGYLDIG) return false;

    avstand_ut = avstand;
    styrke_ut  = styrke;
    return true;
}