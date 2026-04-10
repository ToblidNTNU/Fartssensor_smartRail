#pragma once

// ── WiFi ──────────────────────────────────────────────────────────────────────
#define WIFI_SSID       "KINE"
#define WIFI_PASSWORD   "Lars1111"

// ── MQTT ──────────────────────────────────────────────────────────────────────
#define MQTT_SERVER     "192.168.137.19"
#define MQTT_PORT       1884
#define MQTT_CLIENT_ID  "ESP32_client2"
#define MQTT_TOPIC_PUB_ARRAY  "nr1/array"
#define MQTT_TOPIC_PUB_FART "nr1/fart"
#define MQTT_TOPIC_PUB_SNITT "nr1/snitt"
#define MQTT_TOPIC_SUB  "rpi/broadcast"
#define MQTT_TOPIC_CMD  "nr1/kommando"   // Node-RED sender "av" eller "paa" hit

// ── Hardware ──────────────────────────────────────────────────────────────────
#define SENSOR_PIN      0
#define LED_PIN         2