// SPDX-License-Identifier: MIT
/**
 * @file
 * @brief Bounded whole-line logging and control/status formatting.
 */

#pragma once

#include <Arduino.h>
#include <Print.h>

#include "can_driver.h"
#include "can_frame.h"
#include "safety.h"
#include "experiment.h"

namespace canchaos {

/// Main-loop-only line queue; overflow drops whole lines rather than blocking control processing.
class BufferedLogOutput : public Print {
 public:
  using Print::write;
  size_t write(uint8_t value) override;
  /// Queue an unfinished console fragment atomically, without adding a newline.
  /// Dropped fragments share droppedLines accounting; completed log lines stay atomic.
  void flushFragment();
  /// Write at most budget bytes; destination.write() may itself block inside the framework.
  void drain(Print& destination, size_t budget);
  uint32_t droppedLines() const { return droppedLines_; }
  void resetStats() { droppedLines_ = 0; }

 private:
  uint8_t queue_[2048] = {};
  uint8_t line_[160] = {};
  size_t head_ = 0;
  size_t count_ = 0;
  size_t lineLength_ = 0;
  bool lineOverflow_ = false;
  uint32_t droppedLines_ = 0;
};

class Logger {
 public:
  /// @param out Non-owning output sink that must outlive the logger.
  explicit Logger(Print& out);

  void banner();
  void help();
  void consoleStart(SafetyState state);
  void prompt();
  /// Queue one sanitized display character; caller converts token whitespace to a space.
  void echo(char value);
  void eraseCharacter();
  void endInputLine();
  void status(SafetyState state, const CanDriver& can, const ExperimentManager& experiment,
              uint32_t nowMs);
  /// Drain one output budget. Responses may be dropped; commands do not wait for output delivery.
  void poll();
  void resetStats() { buffer_.resetStats(); }
  void frameRx(const CanFrame& frame);
  void frameTx(const CanFrame& frame);
  void ok(const __FlashStringHelper* message);
  void error(const __FlashStringHelper* message);

 private:
  void printFrame(const __FlashStringHelper* direction, const CanFrame& frame);

  Print& destination_;
  BufferedLogOutput buffer_;
  Print& out_;
};

}  // namespace canchaos
