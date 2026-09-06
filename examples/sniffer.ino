// SPDX-License-Identifier: MIT
/**
 * @file
 * @brief Bench CAN receiver example; not hardware listen-only mode.
 */

#include <Arduino.h>
#include <Arduino_CAN.h>

void setup() {
  Serial.begin(115200);
  delay(300);

  if (!CAN.begin(CanBitRate::BR_500k)) {
    Serial.println("CAN init failed!");
    while (1) {
      delay(1000);
    }
  }
  Serial.println("CAN Sniffer Ready (500 kbps)");
}

void loop() {
  if (CAN.available()) {
    CanMsg const msg = CAN.read();
    uint32_t const id = msg.isStandardId() ? msg.getStandardId() : msg.getExtendedId();

    Serial.print("ID: 0x");
    Serial.print(id, HEX);
    if (!msg.isStandardId()) {
      Serial.print(" (EXT)");
    }
    Serial.print(" DLC: ");
    Serial.print(msg.data_length);
    Serial.print(" Data: ");

    for (uint8_t i = 0; i < msg.data_length && i < 8; ++i) {
      byte const b = msg.data[i];
      if (b < 0x10) {
        Serial.print("0");
      }
      Serial.print(b, HEX);
      Serial.print(" ");
    }
    Serial.println();
  }
}
