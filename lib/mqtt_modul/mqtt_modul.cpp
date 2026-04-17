#include "mqtt_modul.h"
#include "config.h"
#include <WiFi.h>
#include <PubSubClient.h>



/*
*  mqtt_modul.cpp - Modul for å håndtere MQTT-kommunikasjon
*  Denne modulen kobler til WiFi og en MQTT-broker, og håndterer sending av data og mottak av kommandoer. Den bruker PubSubClient-biblioteket for MQTT-funksjonalitet.
*  Forutsetninger:
*  - WiFi-credentials (SSID og passord) er definert i config.h
*  - MQTT-server og port er definert i config.h
*  - Temaer for MQTT-kommunikasjon er definert i config.h
*  Bruk:
*  - Kall mqtt_init() i setup() for å initialisere MQTT-modulen og koble til WiFi og MQTT-broker
*  - Kall mqtt_loop() i loop() for å håndtere MQTT-kommunikasjon kontinuerlig
*  - Bruk mqtt_send_fart_array(), mqtt_send_fart_int() og mqtt_send_snitt() for å sende data til MQTT-broker
*  - Mottatte MQTT-meldinger på temaet MQTT_TOPIC_CMD håndteres i mottatt_melding() og kan brukes til å styre systemets tilstand (f.eks. ON/OFF/RESET)
*  - Systemets tilstand styres av flagget system_aktiv, som kan settes til false for å deaktivere systemet (f.eks. ved å stoppe datainnsamling og sending), og true for å aktivere det igjen.
*  - For å restarte systemet, kan du sende "RESET" som kommando, som vil kalle ESP.restart() for å starte hele systemet på nytt.
* - For å indikere feil ved tilkobling til MQTT-broker, blinker LED_PIN 3 ganger raskt.
*  Merk: Denne modulen fokuserer på MQTT-kommunikasjon og tilkobling. Den håndterer ikke selve datainnsamlingen eller FFT-beregningene, som bør implementeres i andre moduler (f.eks. fft_modul). Det er viktig å sørge for at mqtt_loop() kalles ofte nok i loop() for å opprettholde MQTT-tilkoblingen og håndtere innkommende meldinger.
*/


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

void mqtt_send_fart_array(float* verdier, int antall) {
    if (!mqttClient.connected()) return;

    String payload = "[";
    for (int i = 0; i < antall; i++) {
        payload += String(verdier[i], 2);
        if (i < antall - 1) payload += ",";
    }
    payload += "]";

    mqttClient.publish(MQTT_TOPIC_PUB_ARRAY, payload.c_str());
    Serial.print("Sendt: ");
    Serial.println(payload);
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
  for (int i = 0; i < 25; i++) {
    payload += String(avstand[i]);
    if (i < 24) payload += ",";
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
