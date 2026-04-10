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
void mqtt_send_fart(float* verdier, int antall);