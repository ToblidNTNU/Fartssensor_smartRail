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
 * Send statusinformasjon til MQTT-topic.
 */
void mqtt_send_status();

/**
 * Send variabler (peak, mag, terskel) til MQTT-topic for debugging/tuning.
 */
void mqtt_send_variabler(float peak, float mag, float terskel);

void mqtt_send_debug(float snitt, float peak);