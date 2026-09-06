// SPDX-License-Identifier: MIT
/**
 * @file
 * @brief Bounded serial command parser and control dispatch.
 */

#pragma once

#include <Arduino.h>

#include "can_driver.h"
#include "experiment.h"
#include "logger.h"
#include "safety.h"

namespace canchaos {

/**
 * @brief Main-loop-only parser; dependencies are non-owning and must outlive the interface.
 *
 * @note Input is untrusted but unauthenticated. LF ends a line; rejected lines are drained to LF.

 */
class CommandInterface {
 public:
  CommandInterface(Stream& serial, SafetyManager& safety, CanDriver& can,
                   ExperimentManager& experiment, Logger& logger);

  /// Reset parser state and enqueue help; does not arm or start an experiment.
  void begin();
  /// Consume at most kSerialBytesPerLoop bytes and one line, using a current millis() snapshot.
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
