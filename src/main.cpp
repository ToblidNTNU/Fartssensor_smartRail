#include <Arduino.h>
#include "config.h"
#include "fft_modul.h"
#include "mqtt_modul.h"
#include "lidar_modul.h"


void setup() {
    Serial.begin(115200);
    delay(2000);  // Vent til Serial er klar

    fft_init(); // Initialiser FFT-modulen
    mqtt_init(); // Initialiser MQTT-modulen og koble til WiFi og MQTT-broker
    lidar_init(500); // Initialiser LIDAR-modulen med ønsket oppdateringsfrekvens (f.eks. 100 Hz)

    Serial.println("Klar.");
}

void loop() {
    mqtt_loop(); // Håndter MQTT-kommunikasjon kontinuerlig

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
    mqtt_send_status(); // Send status (f.eks. modus) til MQTT
    Serial.println("\n\n");

}