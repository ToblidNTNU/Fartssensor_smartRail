#include "lidar_modul.h"
#include "config.h"

// ── Konfigurasjon ─────────────────────────────────────────────────────────────
#define LIDAR_BAUD      460800      // 460800 baud for å håndtere 1000 Hz
#define LIDAR_UGYLDIG   45000       // Sensorens verdi for ugyldig måling

// ── Interne variabler ─────────────────────────────────────────────────────────
static HardwareSerial lidarSerial(2);
static uint8_t        uart_buf[9];

// ── Interne hjelpefunksjoner ──────────────────────────────────────────────────
static void sett_max_frekvens() {
    uint8_t cmd[] = {0x5A, 0x06, 0x03, 0xE8, 0x03, 0x00};
    lidarSerial.write(cmd, sizeof(cmd));
    delay(100);
    Serial.println("[lidar] Frekvens satt til 1000 Hz");
}

static void sett_millimeter_modus() {
    uint8_t cmd[] = {0x5A, 0x05, 0x05, 0x06, 0x6A};
    lidarSerial.write(cmd, sizeof(cmd));
    delay(100);
    Serial.println("[lidar] Millimeter-modus aktivert");
}

static void rydd_buffer() {
    while (lidarSerial.available()) lidarSerial.read();
}

// ── Offentlige funksjoner ─────────────────────────────────────────────────────
void lidar_init() {
    lidarSerial.begin(LIDAR_BAUD, SERIAL_8N1, LIDAR_RX_PIN, LIDAR_TX_PIN);
    delay(500);
    rydd_buffer();

    sett_millimeter_modus();
    sett_max_frekvens();
    delay(200);
    rydd_buffer();

    Serial.println("[lidar] TF02-Pro klar (1000 Hz, 460800 baud, mm-modus)");
}

bool lidar_les(int &avstand_ut, int &styrke_ut) {
    // Finn startbyte 0x59
    while (lidarSerial.available() && lidarSerial.peek() != 0x59) {
        lidarSerial.read();
    }

    // Vent til full pakke (9 bytes) er tilgjengelig
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
    //Byte 6 og 7 inneholder temperatur, men vi trenger den ikke nå

    // Filtrer ut sensorens ugyldig-verdi
    if (avstand == LIDAR_UGYLDIG) return false;

    avstand_ut = avstand;
    styrke_ut  = styrke;
    return true;
}