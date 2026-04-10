#include <Arduino.h>
#include "config.h"
#include "fft_modul.h"
#include "mqtt_modul.h"

// ── Fartsdata-buffer ──────────────────────────────────────────────────────────
#define SEND_BUFFER_SIZE 10
static float    send_buffer[SEND_BUFFER_SIZE];
static int      send_index = 0;

void setup() {
    Serial.begin(115200);
    delay(2000);  // Vent til serial monitor er klar

    fft_init();
    mqtt_init();

    Serial.println("Klar.");
}

void loop() {
    mqtt_loop();

    float fart = 0.0f;
    if (fft_kjor(fart)) {
        Serial.print("Fart: ");
        Serial.print(fart, 1);
        Serial.print(" km/h  |  Snitt: ");
        Serial.print(fft_buffer_snitt(), 1);
        Serial.println(" km/h");

        // Legg til i send-buffer
        send_buffer[send_index++] = fart;

        // Send når bufferen er full
        if (send_index >= SEND_BUFFER_SIZE) {
            mqtt_send_fart(send_buffer, SEND_BUFFER_SIZE);
            send_index = 0;
        }
    } else {
        Serial.println("Ingen deteksjon.");
    }
}