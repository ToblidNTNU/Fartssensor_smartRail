
#define RXD2 32
#define TXD2 33

HardwareSerial lidarSerial(2);

int dist;
int strength;
uint8_t uart[9];
long avstand[25];
int go = 0;

void settMaxFrekvens() {
  uint8_t cmd[] = {0x5A, 0x06, 0x03, 0xE8, 0x03, 0x00};
  lidarSerial.write(cmd, sizeof(cmd));
  delay(100);
  Serial.println("Frekvens satt til 1000 Hz");
}

void settMillimeterModus() {
  uint8_t cmd[] = {0x5A, 0x05, 0x05, 0x06, 0x6A};
  lidarSerial.write(cmd, sizeof(cmd));
  delay(100);
  Serial.println("MM-modus aktivert");
}

void publiserAvstand() {
  String payload = "[";
  for (int i = 0; i < 25; i++) {
    payload += String(avstand[i]);
    if (i < 24) payload += ",";
  }
  payload += "]";
  client.publish("esp32/avstand", payload.c_str());
  Serial.println("Publisert: " + payload);
}

bool lesAvstand() {
  while (lidarSerial.available() && lidarSerial.peek() != 0x59) {
    lidarSerial.read();
  }
  if (lidarSerial.available() < 9) return false;

  for (int i = 0; i < 9; i++) {
    uart[i] = lidarSerial.read();
  }

  if (uart[0] != 0x59 || uart[1] != 0x59) return false;

  uint8_t check = 0;
  for (int i = 0; i < 8; i++) check += uart[i];
  if (uart[8] != check) return false;

  dist     = uart[2] + uart[3] * 256;
  strength = uart[4] + uart[5] * 256;

  if (dist == 45000) return false;

  return true;
}

void setup() {
  // 460800 baud for å håndtere 1000 Hz
  lidarSerial.begin(460800, SERIAL_8N1, RXD2, TXD2);
  delay(500);
  while (lidarSerial.available()) lidarSerial.read();

  settMillimeterModus();
  settMaxFrekvens();
  delay(200);
  while (lidarSerial.available()) lidarSerial.read();

  Serial.println("TF02-Pro klar (1000 Hz, 460800 baud, mm-modus)");

  client.setServer(mqtt_server, 1884);
  client.setKeepAlive(60);
}

void loop() {
  if (!client.connected()) connect_mqttServer();
  client.loop();

  if (lesAvstand()) {
    Serial.print("Avstand: ");
    Serial.print(dist);
    Serial.println(" mm  |  Styrke: " + String(strength));

    avstand[go++] = dist;

    if (go >= 25) {
      publiserAvstand();
      go = 0;
    }
  }
}
