#include <Arduino.h>
#include "config.h"
#include "fft_modul.h"
#include "mqtt_modul.h"

/*
 * Hovedprogram for SmartRail. Leser fartverdier fra FFT-modulen og sender dem til MQTT.
 * 
 * Fartverdier sendes individuelt så snart de er tilgjengelige, for å minimere forsinkelse.
 * Gjennomsnittsfart sendes separat etter hver måling.
 * 
 * Mottar kommandoer fra Node-RED for å aktivere/deaktivere systemet:
 * - "OFF": Deaktiverer systemet (stopper måling og sending)
 * - "ON": Aktiverer systemet (starter måling og sending på nytt)
 * 
 * Bruker et flagg (system_aktiv) for å styre om måling og sending skal skje, basert på mottatte kommandoer.
 * Flagget er volatile siden det endres i en callback som kan kalles når som helst.
 * 
 * Forbedringer:
 * - Legg til feilhåndtering ved MQTT-tilkobling og sending
 * - Implementer en mer robust måte å håndtere buffer-overflow hvis det kommer inn flere fartverdier enn buffer-størrelsen før sending
 * - Vurder å sende gjennomsnittsfart sammen med hver gruppe av fartverdier for mer kontekst
 */


 /*
// ── Fartsdata-buffer, for å sende array─────────────────────────────────────────────────
#define SEND_BUFFER_SIZE 10
static float    send_buffer[SEND_BUFFER_SIZE];
static int      send_index = 0; // Neste posisjon i buffer for innlegging av fartverdi
*/


void setup() {
    Serial.begin(115200);
    delay(2000);  // Vent til serial monitor er klar

    fft_init();
    mqtt_init();

    Serial.println("Klar.");
}

void loop() {
    mqtt_loop();

    if (!system_aktiv) {
    Serial.println("System deaktivert. Venter på 'ON'-kommando...");
    delay(1000);
    return;
    }

    float fart = 0.0f;
    if (fft_kjor(fart)) {

        /* for å sende array, bruk følgende kode:
        // Legg til i send-buffer
        send_buffer[send_index++] = fart;

        // Send når bufferen er full
        if (send_index >= SEND_BUFFER_SIZE) {
            mqtt_send_fart(send_buffer, SEND_BUFFER_SIZE);
            send_index = 0;
        }
        */
        mqtt_send_fart_int(fart);
        mqtt_send_snitt(fft_buffer_snitt());

    } else {
        Serial.println("ERROR. For lav magnitude.");
        mqtt_send_fart_int(fart);
        mqtt_send_snitt(fft_buffer_snitt());
    }
}