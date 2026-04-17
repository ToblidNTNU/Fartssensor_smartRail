#pragma once
#include <Arduino.h>


// ── Offentlig API ─────────────────────────────────────────────────────────────

/**
 * Koble til WiFi og MQTT-server. Kall én gang i setup().
 */
void mqtt_init();

/**
 * Vedlikehold MQTT-tilkobling. Kall én gang per loop()-iterasjon.
 * Kobler automatisk til på nytt hvis tilkoblingen er mistet.
 */
void mqtt_loop();

/**
 * Send en liste med fartverdier til MQTT-topic som JSON-array.
 * Eksempel: [12.10,11.60,10.00]
 *
 * @param verdier   Peker til array med fartverdier
 * @param antall    Antall verdier i arrayen
 */
void mqtt_send_fart_array(float* verdier, int antall);

/**
 * Send en enkelt fartverdi til MQTT-topic.
 *
 * @param verdi  Fartverdi
 */
void mqtt_send_fart_int(float verdi);

/**
 * Send gjennomsnittsfart til MQTT-topic som enkeltverdi.
 *
 * @param verdi  Gjennomsnittsfart
 */
void mqtt_send_snitt(float verdi);


/**
 * 
 * Send en liste med avstandsmålinger til MQTT-topic som JSON-array.
 * Eksempel: [120.0, 115.5, 130.2]
 * @param avstand  Peker til array med avstandsmålinger
 * @param antall    Antall målinger i arrayen
 */
void mqtt_send_avstand(float* avstand, int antall);



 
// Flagg satt av innkommende kommandoer fra Node-RED.
// volatile: hindrer kompilatoren fra å cache verdien, siden den kan endres naar som helst.
extern volatile bool system_aktiv;
extern volatile bool lidar_modus;