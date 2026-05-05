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
 */
void mqtt_send_status();



