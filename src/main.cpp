
#include <Arduino.h>
#include "config.h"
#include "fft_modul.h"
#include "mqtt_modul.h"
#include "lidar_modul.h"
#include "state.h"

uint8_t counter = 10;


void send_fft_debug(float snitt, float peak) {
    mqtt_send_debug(snitt, peak);
}

void setup() {
    Serial.begin(115200);
    delay(2000);  // Vent til Serial er klar

    fft_debug_callback = send_fft_debug;

    fft_init(); // Initialiser FFT-modulen
    mqtt_init(); // Initialiser MQTT-modulen og koble til WiFi og MQTT-broker
    lidar_init(500); // Initialiser LIDAR-modulen med ønsket oppdateringsfrekvens (f.eks. 100 Hz)

    Serial.println("Klar.");
}

void loop() {
    mqtt_loop(); // Håndter MQTT-kommunikasjon kontinuerlig

    if ((counter >= 10) || motatt_melding_flagg) {
        mqtt_send_status();
        mqtt_send_variabler(PEAK_SIZE, MAG_GRENSE, SVILLE_TERSKEL);
        motatt_melding_flagg = false;
        counter = 0;
    }
    counter++;

    // Hvis systemet er deaktivert, gjør ingenting og vent litt før neste sjekk
    if (!system_aktiv) {
        Serial.print(". ");
        delay(1000);
        return;
    }

    float fart = 0.0f; 

    Serial.println("Starter måling...");
    // Hent fartverdier fra FFT-modulen og send til MQTT
    if (fft_kjor(fart)) {
       
        mqtt_send_fart_int(fart);
        mqtt_send_snitt(fft_buffer_snitt());
    } else {
        Serial.println("ERROR. For lav magnitude.");
        mqtt_send_fart_int(fart);
        mqtt_send_snitt(fft_buffer_snitt());
    }
    Serial.println("Måling ferdig");
    Serial.println("\n\n");

}