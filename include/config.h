#pragma once

// ── WiFi ──────────────────────────────────────────────────────────────────────
#define WIFI_SSID       "KINE"
#define WIFI_PASSWORD   "Lars1111"

// ── MQTT ──────────────────────────────────────────────────────────────────────
#define MQTT_SERVER     "192.168.137.19"
#define MQTT_PORT       1884
#define MQTT_CLIENT_ID  "ESP32_client2"
#define MQTT_TOPIC_PUB  "nr1"
#define MQTT_TOPIC_SUB  "rpi/broadcast"

// ── Hardware ──────────────────────────────────────────────────────────────────
#define SENSOR_PIN      0
#define LED_PIN         2