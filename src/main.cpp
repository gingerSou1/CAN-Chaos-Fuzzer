// SPDX-License-Identifier: MIT
/**
 * @file
 * @brief Firmware startup and bounded cooperative loop.
 */

#include <Arduino.h>

#include "can_driver.h"
#include "command_interface.h"
#include "config.h"
#include "experiment.h"
#include "logger.h"
#include "safety.h"

using namespace canchaos;

SafetyManager safety;
CanDriver canDriver(safety);
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
  canDriver.pollHealth();
  // Reconcile a fault before processing commands that report experiment counters.
  if (safety.state() == SafetyState::Fault) {
    experiment.stop();
  }
  commandInterface.poll(millis());
  experiment.update(millis());

  CanFrame frame;
  for (uint8_t received = 0; received < kCanRxFramesPerLoop && canDriver.receive(frame);
       ++received) {
    logger.frameRx(frame);
  }
  logger.poll();
}
