#include "logger.h"

#include <Arduino.h>

namespace canchaos {

Logger::Logger(Print& out) : out_(out) {}

void Logger::banner() {
  out_.println(F("CAN Chaos Fuzzer Milestone 1"));
  out_.println(F("Boot state: SAFE. Transmit requires arm then start."));
}

void Logger::help() {
  out_.println(F("Commands:"));
  out_.println(F("  status"));
  out_.println(F("  arm"));
  out_.println(F("  disarm"));
  out_.println(F("  start [duration_ms] [interval_ms]"));
  out_.println(F("  stop"));
  out_.println(F("  stats"));
  out_.println(F("  reset"));
  out_.println(F("  help"));
}

void Logger::status(SafetyState state, uint32_t bitrate, const CanDriverStats& stats, bool experimentActive,
                    uint32_t remainingMs) {
  out_.print(F("STATE: "));
  out_.println(toString(state));
  out_.print(F("CAN: "));
  out_.println(bitrate > 0 ? F("ONLINE") : F("OFFLINE"));
  out_.print(F("BITRATE: "));
  out_.println(bitrate);
  out_.print(F("EXPERIMENT: "));
  out_.println(experimentActive ? F("ACTIVE") : F("IDLE"));
  out_.print(F("REMAINING_MS: "));
  out_.println(remainingMs);
  out_.print(F("TX: "));
  out_.println(stats.txFrames);
  out_.print(F("RX: "));
  out_.println(stats.rxFrames);
  out_.print(F("TX_ERRORS: "));
  out_.println(stats.txErrors);
  out_.print(F("LAST_WRITE_RESULT: "));
  out_.println(stats.lastWriteResult);
}

void Logger::frameRx(const CanFrame& frame) {
  printFrame(F("RX"), frame);
}

void Logger::frameTx(const CanFrame& frame) {
  printFrame(F("TX"), frame);
}

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
