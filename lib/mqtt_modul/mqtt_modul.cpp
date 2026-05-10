#include "mqtt_modul.h"
#include "config.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include "fft_modul.h"
#include "state.h"

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
    String tekst = "";
    for (unsigned int i = 0; i < lengde; i++) {
        tekst += (char)melding[i];
    }

    Serial.print("Mottatt på ");
    Serial.print(topic);
    Serial.print(": ");
    Serial.println(tekst);

    mottatt_melding_flagg = true;

    if (String(topic) == MQTT_TOPIC_SUB_CMD) {
        
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
        }
    }

    if (String(topic) == MQTT_TOPIC_SUB_MODUS) {
        if (tekst == "AVSTAND") {
            Serial.println("Bytter til AVSTAND-modus");
            delay(500);
            lidar_modus = 0;
        } else if (tekst == "STYRKE") {
            Serial.println("Bytter til STYRKE-modus");
            delay(500);
            lidar_modus = 1;
        } else if (tekst == "FLATT") {
            Serial.println("Bytter til FLATT-modus");
            delay(500);
            lidar_modus = 2; 
        } else {
            Serial.println("Ukjent modus.");
        }
    }

    if (String(topic) == MQTT_TOPIC_SUB_TUNING) {
        if (tekst.startsWith("MAG:")) {
            //Da kan du sende "MAG:50" eller "PEAK:3" fra Node-RED for å justere parameterne 
            fft_sett_parametere(tekst.substring(4).toFloat(), PEAK_SIZE, SVILLE_TERSKEL);
            Serial.println("MAG_GRENSE satt til: " + String(MAG_GRENSE));
        } else if (tekst.startsWith("PEAK:")) {
            fft_sett_parametere(MAG_GRENSE, tekst.substring(5).toFloat(), SVILLE_TERSKEL);
            Serial.println("PEAK_SIZE satt til: " + String(PEAK_SIZE));
        } else if (tekst.startsWith("TERSKEL:")) {
            fft_sett_parametere(MAG_GRENSE, PEAK_SIZE, tekst.substring(8).toFloat());
            Serial.println("SVILLE_TERSKEL satt til: " + String(SVILLE_TERSKEL));
        } else {
            Serial.println("Ukjent modus.");
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
            mqttClient.subscribe(MQTT_TOPIC_SUB_CMD);
            mqttClient.subscribe(MQTT_TOPIC_SUB_MODUS); 
            mqttClient.subscribe(MQTT_TOPIC_SUB_TUNING);
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
        Serial.println("MQTT frakoblet. Prøver å koble til igjen...");
        koble_til_mqtt();
    }
    mqttClient.loop();
}


//----MqTT publish-funksjoner for ulike data----

void mqtt_send_fart_int(float verdi) {
    if (!mqttClient.connected()) return;

    String payload = String(verdi, 2);

    mqttClient.publish(MQTT_TOPIC_PUB_FART, payload.c_str());
    Serial.print("Sendt fart: ");
    Serial.println(payload);
}

void mqtt_send_snitt(float verdi) {
    if (!mqttClient.connected()) return;

    String payload = String(verdi, 2);

    mqttClient.publish(MQTT_TOPIC_PUB_SNITT, payload.c_str());
    Serial.print("Sendt gjennomsnittsfart: ");
    Serial.println(payload);
}

void mqtt_send_status() {
    if (!mqttClient.connected()) return;

    String payload = String(lidar_modus == 0 ? "AVSTAND" : (lidar_modus == 1 ? "STYRKE" : "FLATT")) + ", " + (system_aktiv ? "AKTIV" : "DEAKTIVERT");


    mqttClient.publish(MQTT_TOPIC_PUB_STATUS, payload.c_str());
    Serial.print("Sendt status: ");
    Serial.println(payload);
}

void mqtt_send_variabler(float peak, float mag, float terskel) {
    if (!mqttClient.connected()) return;

    String payload = String("Peak: ") + String(peak) + ", Mag: " + String(mag) + ", Terskel: " + String(terskel);

    mqttClient.publish(MQTT_TOPIC_PUB_VARIABLER, payload.c_str());
    Serial.print("Sendt variabler: ");
    Serial.println(payload);
}

void mqtt_send_debug(float snitt, float peak) {
    if (!mqttClient.connected()) return;
    String payload = String("Snitt: ") + String(snitt, 2) + ", Peak: " + String(peak, 2);
    mqttClient.publish(MQTT_TOPIC_PUB_FFT, payload.c_str());
}