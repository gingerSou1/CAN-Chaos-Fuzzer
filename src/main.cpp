#include <Arduino.h>

#include "can_driver.h"
#include "command_interface.h"
#include "config.h"
#include "experiment.h"
#include "logger.h"
#include "safety.h"

using namespace canchaos;

CanDriver canDriver;
SafetyManager safety;
ExperimentManager experiment(canDriver, safety);
Logger logger(Serial);
CommandInterface commandInterface(Serial, safety, canDriver, experiment, logger);

void setup() {
  Serial.begin(kSerialBaud);
  delay(300);

  safety.begin();
  logger.banner();

  if (!canDriver.begin(kDefaultBitrate)) {
    safety.fault();
    logger.error(F("CAN INIT FAILED"));
  } else {
    logger.ok(F("CAN ONLINE 500000"));
  }

  commandInterface.begin();
}

void loop() {
  uint32_t const nowMs = millis();
  commandInterface.poll(nowMs);
  experiment.update(nowMs);

  CanFrame frame;
  while (canDriver.receive(frame)) {
    logger.frameRx(frame);
  }
}
