// SPDX-License-Identifier: MIT
/**
 * @file
 * @brief Line rejection, argument validation and serial command dispatch.
 */

#include "command_interface.h"

#include <string.h>

#include "config.h"

namespace canchaos {

CommandInterface::CommandInterface(Stream& serial, SafetyManager& safety, CanDriver& can,
                                   ExperimentManager& experiment, Logger& logger)
    : serial_(serial), safety_(safety), can_(can), experiment_(experiment), logger_(logger) {}

void CommandInterface::begin() {
  length_ = 0;
  discarding_ = false;
  invalidCharacter_ = false;
  skipLf_ = false;
  buffer_[0] = '\0';
  logger_.consoleStart(safety_.state());
}

void CommandInterface::poll(uint32_t nowMs) {
  for (uint8_t consumed = 0; consumed < kSerialBytesPerLoop && serial_.available(); ++consumed) {
    int const input = serial_.read();
    if (input < 0) {
      break;
    }
    char const c = static_cast<char>(input);
    if (skipLf_) {
      skipLf_ = false;
      if (c == '\n') {
        continue;
      }
    }
    if (c == '\r' || c == '\n') {
      skipLf_ = c == '\r';
      logger_.endInputLine();
      buffer_[length_] = '\0';
      if (discarding_) {
        logger_.error(invalidCharacter_ ? F("INVALID COMMAND CHARACTER") : F("COMMAND TOO LONG"));
      } else if (length_ > 0) {
        handleLine(buffer_, nowMs);
      }
      length_ = 0;
      discarding_ = false;
      invalidCharacter_ = false;
      buffer_[0] = '\0';
      logger_.prompt();
      // One complete line per poll. Commands following STOP wait until next loop.
      return;
    }
    if (discarding_) {
      continue;
    }
    if (input == 0x08 || input == 0x7F) {
      if (length_ > 0) {
        buffer_[--length_] = '\0';
        logger_.eraseCharacter();
      }
      continue;
    }
    if ((input < 32 && c != '\t' && c != '\v' && c != '\f') || input > 126) {
      discarding_ = true;
      invalidCharacter_ = true;
      continue;
    }
    if (length_ < (sizeof(buffer_) - 1)) {
      buffer_[length_++] = c;
      logger_.echo(input >= 32 ? c : ' ');
    } else {
      length_ = 0;
      discarding_ = true;
      buffer_[0] = '\0';
    }
  }
}

void CommandInterface::handleLine(char* line, uint32_t nowMs) {
  char* command = strtok(line, " \t\r\v\f");
  char* arg1 = strtok(nullptr, " \t\r\v\f");
  char* arg2 = strtok(nullptr, " \t\r\v\f");
  char* extra = strtok(nullptr, " \t\r\v\f");

  if (command == nullptr) {
    return;
  }
  if (strcmp(command, "fuzz") == 0) {
    char* countText = strtok(nullptr, " \t\r\v\f");
    char* intervalText = strtok(nullptr, " \t\r\v\f");
    char* trailing = strtok(nullptr, " \t\r\v\f");
    if (arg1 == nullptr || strcmp(arg1, "start") != 0 || intervalText == nullptr ||
        trailing != nullptr) {
      logger_.error(F("EXPECTED fuzz start <strategy> <seed> <count> <interval_ms>"));
      return;
    }
    FuzzStrategy strategy;
    if (!parseFuzzStrategy(arg2, strategy)) {
      logger_.error(F("INVALID FUZZ STRATEGY"));
      return;
    }
    uint32_t seed = 0;
    uint32_t count = 0;
    uint32_t intervalMs = 0;
    if (!parseUnsigned(extra, seed)) {
      logger_.error(F("INVALID SEED"));
      return;
    }
    if (!parseUnsigned(countText, count)) {
      logger_.error(F("INVALID COUNT"));
      return;
    }
    if (!parseUnsigned(intervalText, intervalMs)) {
      logger_.error(F("INVALID INTERVAL"));
      return;
    }
    if (experiment_.startFuzz(nowMs, strategy, seed, count, intervalMs)) {
      logger_.ok(F("FUZZ EXPERIMENT STARTED"));
    } else {
      logger_.error(F("DEVICE NOT ARMED OR UNSAFE CONFIG"));
    }
    return;
  }
  if (extra != nullptr || (strcmp(command, "start") != 0 && arg1 != nullptr)) {
    logger_.error(F("UNEXPECTED ARGUMENT"));
    return;
  }

  if (strcmp(command, "status") == 0) {
    logger_.status(safety_.state(), can_, experiment_, nowMs);
  } else if (strcmp(command, "arm") == 0) {
    can_.pollHealth();
    if (can_.status() == CanStatus::Online && safety_.arm()) {
      logger_.ok(F("STATE ARMED"));
    } else {
      logger_.error(F("ARM REJECTED"));
    }
  } else if (strcmp(command, "disarm") == 0) {
    experiment_.stop();
    if (safety_.disarm()) {
      logger_.ok(F("STATE SAFE"));
    } else {
      logger_.error(F("DISARM REJECTED"));
    }
  } else if (strcmp(command, "start") == 0) {
    uint32_t durationMs = kDefaultExperimentDurationMs;
    uint32_t intervalMs = kDefaultTxIntervalMs;
    if (arg1 != nullptr && !parseUnsigned(arg1, durationMs)) {
      logger_.error(F("INVALID DURATION"));
      return;
    }
    if (arg2 != nullptr && !parseUnsigned(arg2, intervalMs)) {
      logger_.error(F("INVALID INTERVAL"));
      return;
    }
    if (experiment_.startKnownFrameDemo(nowMs, durationMs, intervalMs)) {
      logger_.ok(F("EXPERIMENT STARTED"));
    } else {
      logger_.error(F("DEVICE NOT ARMED OR UNSAFE CONFIG"));
    }
  } else if (strcmp(command, "stop") == 0) {
    experiment_.stop();
    logger_.ok(F("EXPERIMENT STOPPED"));
  } else if (strcmp(command, "stats") == 0) {
    logger_.status(safety_.state(), can_, experiment_, nowMs);
  } else if (strcmp(command, "reset") == 0) {
    experiment_.stop();
    if (safety_.disarm()) {
      experiment_.resetStats();
      can_.resetStats();
      logger_.resetStats();
      logger_.ok(F("CONTROL STATE RESET TO SAFE"));
    } else {
      logger_.error(F("RESET REJECTED IN FAULT"));
    }
  } else if (strcmp(command, "help") == 0 || strcmp(command, "menu") == 0) {
    logger_.help();
  } else {
    logger_.error(F("UNKNOWN COMMAND"));
  }
}

bool CommandInterface::parseUnsigned(const char* text, uint32_t& value) const {
  if (text == nullptr || *text == '\0') {
    return false;
  }

  uint32_t parsed = 0;
  for (size_t i = 0; text[i] != '\0'; ++i) {
    if (text[i] < '0' || text[i] > '9') {
      return false;
    }
    uint32_t const digit = static_cast<uint32_t>(text[i] - '0');
    if (parsed > (UINT32_MAX - digit) / 10U) {
      return false;
    }
    parsed = (parsed * 10U) + digit;
  }

  value = parsed;
  return true;
}

}  // namespace canchaos
