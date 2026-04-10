#include "mqtt_modul.h"
#include "config.h"
#include <WiFi.h>
#include <PubSubClient.h>

// ── Interne objekter ──────────────────────────────────────────────────────────
static WiFiClient   wifiClient;
static PubSubClient mqttClient(wifiClient);

// ── Interne hjelpefunksjoner ──────────────────────────────────────────────────
static void blink_led(int ganger, int varighet_ms) {
    for (int i = 0; i < ganger; i++) {
        digitalWrite(LED_PIN, HIGH);
        delay(varighet_ms);
        digitalWrite(LED_PIN, LOW);
        delay(varighet_ms);
    }
}

static void koble_til_wifi() {
    if (WiFi.status() == WL_CONNECTED) return;

    Serial.print("Kobler til WiFi: ");
    Serial.println(WIFI_SSID);

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("\nWiFi tilkoblet. IP: ");
    Serial.println(WiFi.localIP());
}

static void mottatt_melding(char* topic, byte* melding, unsigned int lengde) {
    if (String(topic) == MQTT_TOPIC_SUB) {
        String tekst = "";
        for (unsigned int i = 0; i < lengde; i++) {
            tekst += (char)melding[i];
        }
        Serial.print("Mottatt på ");
        Serial.print(topic);
        Serial.print(": ");
        Serial.println(tekst);
    }
}

static void koble_til_mqtt() {
    while (!mqttClient.connected()) {
        koble_til_wifi();

        Serial.print("Kobler til MQTT...");
        if (mqttClient.connect(MQTT_CLIENT_ID)) {
            Serial.println("tilkoblet.");
            mqttClient.subscribe(MQTT_TOPIC_SUB);
        } else {
            Serial.print("Feil, rc=");
            Serial.print(mqttClient.state());
            Serial.println(". Prøver igjen om 2 sekunder.");
            blink_led(3, 200);
            delay(2000);
        }
    }
}

// ── Offentlige funksjoner ─────────────────────────────────────────────────────
void mqtt_init() {
    pinMode(LED_PIN, OUTPUT);
    koble_til_wifi();
    mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
    mqttClient.setCallback(mottatt_melding);
    koble_til_mqtt();
}

void mqtt_loop() {
    if (!mqttClient.connected()) {
        koble_til_mqtt();
    }
    mqttClient.loop();
}

void mqtt_send_fart(float* verdier, int antall) {
    if (!mqttClient.connected()) return;

    // Bygg JSON-array: [12.10,11.60,10.00]
    String payload = "[";
    for (int i = 0; i < antall; i++) {
        payload += String(verdier[i], 2);
        if (i < antall - 1) payload += ",";
    }
    payload += "]";

    mqttClient.publish(MQTT_TOPIC_PUB, payload.c_str());
    Serial.print("Sendt: ");
    Serial.println(payload);
}
