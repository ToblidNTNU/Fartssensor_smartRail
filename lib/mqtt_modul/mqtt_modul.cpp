#include "mqtt_modul.h"
#include "config.h"
#include <WiFi.h>
#include <PubSubClient.h>


// ── Interne objekter ──────────────────────────────────────────────────────────
static WiFiClient   wifiClient; 
static PubSubClient mqttClient(wifiClient);

// ── Flagg ─────────────────────────────────────────────────────────────────────
volatile bool system_aktiv = true;
volatile bool lidar_modus = true;


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
    String tekst = "";
    for (unsigned int i = 0; i < lengde; i++) {
        tekst += (char)melding[i];
    }

    Serial.print("Mottatt på ");
    Serial.print(topic);
    Serial.print(": ");
    Serial.println(tekst);

    if (String(topic) == MQTT_TOPIC_CMD) {
        if (tekst == "OFF") {
            Serial.println("System deaktivert.");
            system_aktiv = false;
        } else if (tekst == "ON") {
            Serial.println("System aktivert...");
            system_aktiv = true;
        } else if (tekst == "RESET") {
            Serial.println("System resettes...");
            delay(500);
            ESP.restart();
        } else if (tekst == "STYRKE") {
            Serial.println("Bytter til STYRKE-modus");
            delay(500);
            lidar_modus = false;
        } else if (tekst == "AVSTAND") {
            Serial.println("Bytter til AVSTAND-modus");
            delay(500);
            lidar_modus = true;
        } else {
            Serial.println("Ukjent kommando.");
        }
    }
}

static void koble_til_mqtt() {
    while (!mqttClient.connected()) {
        koble_til_wifi();

        Serial.print("Kobler til MQTT...");
        if (mqttClient.connect(MQTT_CLIENT_ID)) {
            Serial.println("tilkoblet.");
            mqttClient.subscribe(MQTT_TOPIC_SUB);
            mqttClient.subscribe(MQTT_TOPIC_CMD);
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

void mqtt_send_fart_int(float verdi) {
    if (!mqttClient.connected()) return;

    String payload = String(verdi, 2);

    mqttClient.publish(MQTT_TOPIC_PUB_FART, payload.c_str());
    Serial.print("Sendt fart: ");
    Serial.println(payload);
}

void mqtt_send_avstand(float* avstand, int antall) {
    if (!mqttClient.connected()) return;
  String payload = "[";
  for (int i = 0; i < antall; i++) {
    payload += String(avstand[i]);
    if (i < antall-1) payload += ",";
  }
  payload += "]";
  mqttClient.publish(MQTT_TOPIC_PUB_AVSTAND, payload.c_str());
  Serial.println("Publisert: " + payload);
}

void mqtt_send_snitt(float verdi) {
    if (!mqttClient.connected()) return;

    String payload = String(verdi, 2);

    mqttClient.publish(MQTT_TOPIC_PUB_SNITT, payload.c_str());
    Serial.print("Sendt gjennomsnittsfart: ");
    Serial.println(payload);
}
