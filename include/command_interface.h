#pragma once

#include <Arduino.h>

#include "can_driver.h"
#include "experiment.h"
#include "logger.h"
#include "safety.h"

namespace canchaos {

class CommandInterface {
 public:
  CommandInterface(Stream& serial, SafetyManager& safety, CanDriver& can, ExperimentManager& experiment,
                   Logger& logger);

  void begin();
  void poll(uint32_t nowMs);

 private:
  void handleLine(char* line, uint32_t nowMs);
  bool parseUnsigned(const char* text, uint32_t& value) const;

  Stream& serial_;
  SafetyManager& safety_;
  CanDriver& can_;
  ExperimentManager& experiment_;
  Logger& logger_;
  char buffer_[80] = {0};
  uint8_t length_ = 0;
  bool discarding_ = false;
};

}  // namespace canchaos
