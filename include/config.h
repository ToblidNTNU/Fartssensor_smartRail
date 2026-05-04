#pragma once

// ── WiFi ──────────────────────────────────────────────────────────────────────
#define WIFI_SSID       "KINE"
#define WIFI_PASSWORD   "Lars1111"

// ── MQTT ──────────────────────────────────────────────────────────────────────
#define MQTT_SERVER     "192.168.137.119"
#define MQTT_PORT       1884
#define MQTT_CLIENT_ID  "ESP32_lidar"
#define MQTT_TOPIC_PUB_ARRAY  "nr1/array"
#define MQTT_TOPIC_PUB_FART "nr1/fart"
#define MQTT_TOPIC_PUB_SNITT "nr1/snitt"
#define MQTT_TOPIC_PUB_AVSTAND "nr1/avstand"
#define MQTT_TOPIC_PUB_STYRKE "nr1/styrke"
#define MQTT_TOPIC_PUB_STATUS "nr1/status"

#define MQTT_TOPIC_SUB  "rpi/broadcast"

#define MQTT_TOPIC_CMD  "nr1/tilstand"   // Node-RED sender "av" eller "paa" hit
// ON; OFF; RESET;

#define MQTT_TOPIC_MODUS "nr1/modus"    // Node-RED sender "normal" eller "test" hit
//STYRKE; AVSTAND; FLATT

#define MQTT_TOPIC_TUNING "nr1/tuning"  // Node-RED sender JSON med tuning-parametre hit
// {"peakSize":3.0}



// ── Hardware ──────────────────────────────────────────────────────────────────
#define SENSOR_PIN      0
#define LED_PIN         2
#define LIDAR_RX_PIN    32 //gul/grønn
#define LIDAR_TX_PIN    33 //Hvit

// ── Systemstatus ─────────────────────────────────────────────────────────────
extern volatile bool system_aktiv;
extern volatile uint8_t lidar_modus; // 0 = avstand, 1 = styrke, 2 = flatt (for testformål)