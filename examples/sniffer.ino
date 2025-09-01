#include <Arduino.h>
#include <CAN.h>

void setup() {
  Serial.begin(115200);
  while (!Serial) {}

  if (!CAN.begin(500E3)) {
    Serial.println("CAN init failed!");
    while (1) { delay(1000); }
  }
  Serial.println("CAN Sniffer Ready (500 kbps)");
}

void loop() {
  int packetSize = CAN.parsePacket();
  if (packetSize) {
    bool ext = CAN.packetExtended();
    uint32_t id = CAN.packetId();

    Serial.print("ID: 0x");
    Serial.print(id, HEX);
    if (ext) Serial.print(" (EXT)");
    Serial.print(" DLC: ");
    Serial.print(packetSize);
    Serial.print(" Data: ");

    while (CAN.available()) {
      byte b = CAN.read();
      if (b < 0x10) Serial.print("0");
      Serial.print(b, HEX);
      Serial.print(" ");
    }
    Serial.println();
  }
}
