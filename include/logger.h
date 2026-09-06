#pragma once

#include <Arduino.h>
#include <Print.h>

#include "can_driver.h"
#include "can_frame.h"
#include "safety.h"

namespace canchaos {

class Logger {
 public:
  explicit Logger(Print& out);

  void banner();
  void help();
  void status(SafetyState state, uint32_t bitrate, const CanDriverStats& stats, bool experimentActive,
              uint32_t remainingMs);
  void frameRx(const CanFrame& frame);
  void frameTx(const CanFrame& frame);
  void ok(const __FlashStringHelper* message);
  void error(const __FlashStringHelper* message);

 private:
  void printFrame(const __FlashStringHelper* direction, const CanFrame& frame);

  Print& out_;
};

}  // namespace canchaos
