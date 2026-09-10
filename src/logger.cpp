// SPDX-License-Identifier: MIT
/**
 * @file
 * @brief Fixed-size log buffering and diagnostic text output.
 */

#include "logger.h"

#include <Arduino.h>
#include "config.h"

namespace canchaos {

size_t BufferedLogOutput::write(uint8_t value) {
  if (lineLength_ < sizeof(line_)) {
    line_[lineLength_++] = value;
  } else {
    lineOverflow_ = true;
  }
  if (value == '\n') {
    if (lineOverflow_ || lineLength_ > sizeof(queue_) - count_) {
      ++droppedLines_;
    } else {
      for (size_t i = 0; i < lineLength_; ++i) {
        queue_[(head_ + count_) % sizeof(queue_)] = line_[i];
        ++count_;
      }
    }
    lineLength_ = 0;
    lineOverflow_ = false;
  }
  return 1;
}

void BufferedLogOutput::drain(Print& destination, size_t budget) {
  while (budget-- > 0 && count_ > 0) {
    if (destination.write(queue_[head_]) != 1) {
      break;
    }
    head_ = (head_ + 1) % sizeof(queue_);
    --count_;
  }
}

Logger::Logger(Print& out) : destination_(out), out_(buffer_) {}

void Logger::poll() {
  // UNO R4 WiFi's pinned core maps Serial to UART, whose write is synchronous.
  // Limit output to 32 bytes (~2.8 ms at 115200 baud with a healthy UART).
  buffer_.drain(destination_, kLogBytesPerLoop);
}

void Logger::banner() {
  out_.println(F("CAN Chaos Fuzzer Milestone 2A"));
  out_.println(F("Boot state: SAFE. Transmit requires arm then start."));
}

void Logger::help() {
  out_.println(F("Commands:"));
  out_.println(F("  status"));
  out_.println(F("  arm"));
  out_.println(F("  disarm"));
  out_.println(F("  start [duration_ms] [interval_ms]"));
  out_.println(F("  fuzz start <strategy> <seed> <count> <interval_ms>"));
  out_.println(F("    random bitflip zero ff boundary walkingbit"));
  out_.println(F("  stop"));
  out_.println(F("  stats"));
  out_.println(F("  reset"));
  out_.println(F("  help"));
}

void Logger::status(SafetyState state, const CanDriver& can, const ExperimentManager& experiment,
                    uint32_t nowMs) {
  const CanDriverStats& stats = can.stats();
  out_.print(F("STATE: "));
  out_.println(toString(state));
  out_.print(F("CAN: "));
  out_.println(can.status() == CanStatus::Online  ? F("ONLINE")
               : can.status() == CanStatus::Error ? F("ERROR")
                                                  : F("OFFLINE"));
  out_.print(F("BITRATE: "));
  out_.println(can.bitrate());
  out_.print(F("EXPERIMENT: "));
  out_.println(experiment.active() ? F("ACTIVE") : F("IDLE"));
  out_.print(F("REMAINING_MS: "));
  out_.println(experiment.remainingMs(nowMs));
  out_.print(F("TX_ACCEPTED: "));
  out_.println(stats.txFrames);
  out_.print(F("RX: "));
  out_.println(stats.rxFrames);
  out_.print(F("TX_ERRORS: "));
  out_.println(stats.txErrors);
  out_.print(F("TX_RATE_LIMITED: "));
  out_.println(stats.txRateLimited);
  out_.print(F("LAST_WRITE_RESULT: "));
  out_.println(stats.lastWriteResult);
  out_.print(F("CONTROLLER_ERRORS: "));
  out_.println(stats.controllerErrors);
  out_.print(F("LAST_CONTROLLER_ERROR: "));
  out_.println(can.lastControllerError());
  out_.print(F("EXPERIMENTS_STARTED: "));
  out_.println(experiment.stats().started);
  out_.print(F("EXPERIMENTS_COMPLETED: "));
  out_.println(experiment.stats().completed);
  out_.print(F("EXPERIMENTS_STOPPED: "));
  out_.println(experiment.stats().stopped);
  out_.print(F("EXPERIMENTS_FAULTED: "));
  out_.println(experiment.stats().faulted);
  out_.print(F("REJECTED_STARTS: "));
  out_.println(experiment.stats().rejectedStarts);
  out_.print(F("KNOWN_FRAMES_ACCEPTED: "));
  out_.println(experiment.stats().knownFramesSent);
  out_.print(F("EXPERIMENT_MODE: "));
  out_.println(experiment.fuzzMode() ? F("FUZZ") : F("KNOWN"));
  if (experiment.fuzzMode()) {
    const FuzzRun& run = experiment.fuzzRun();
    out_.print(F("FUZZ_STRATEGY: "));
    out_.println(toString(run.strategy));
    out_.print(F("FUZZ_SEED: "));
    out_.println(run.seed);
    out_.print(F("FUZZ_REQUESTED: "));
    out_.println(run.requested);
    out_.print(F("FUZZ_INTERVAL_MS: "));
    out_.println(run.intervalMs);
    out_.print(F("FUZZ_GENERATED: "));
    out_.println(run.generated);
    out_.print(F("FUZZ_TX_ACCEPTED: "));
    out_.println(run.accepted);
    if (run.generated > 0) {
      out_.print(F("FUZZ_LAST_INDEX: "));
      out_.println(run.generated - 1);
      printFrame(F("FUZZ_LAST_GENERATED"), run.lastFrame);
    }
  }
  out_.print(F("LOG_DROPPED_LINES: "));
  out_.println(buffer_.droppedLines());
}

void Logger::frameRx(const CanFrame& frame) { printFrame(F("RX"), frame); }

void Logger::frameTx(const CanFrame& frame) { printFrame(F("TX"), frame); }

void Logger::ok(const __FlashStringHelper* message) {
  out_.print(F("OK: "));
  out_.println(message);
}

void Logger::error(const __FlashStringHelper* message) {
  out_.print(F("ERROR: "));
  out_.println(message);
}

void Logger::printFrame(const __FlashStringHelper* direction, const CanFrame& frame) {
  out_.print(direction);
  out_.print(F(" t="));
  out_.print(frame.timestampMs);
  out_.print(F(" id=0x"));
  out_.print(frame.id, HEX);
  out_.print(frame.extended ? F(" format=EXT") : F(" format=STD"));
  out_.print(F(" dlc="));
  out_.print(frame.length);
  out_.print(F(" data="));
  for (uint8_t i = 0; i < frame.length; ++i) {
    if (frame.data[i] < 0x10) {
      out_.print('0');
    }
    out_.print(frame.data[i], HEX);
  }
  out_.println();
}

}  // namespace canchaos
