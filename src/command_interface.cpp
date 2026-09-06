#include "command_interface.h"

#include <string.h>

#include "config.h"

namespace canchaos {

CommandInterface::CommandInterface(Stream& serial, SafetyManager& safety, CanDriver& can,
                                   ExperimentManager& experiment, Logger& logger)
    : serial_(serial), safety_(safety), can_(can), experiment_(experiment), logger_(logger) {}

void CommandInterface::begin() {
  length_ = 0;
  buffer_[0] = '\0';
  logger_.help();
}

void CommandInterface::poll(uint32_t nowMs) {
  while (serial_.available()) {
    char const c = static_cast<char>(serial_.read());
    if (c == '\r') {
      continue;
    }
    if (c == '\n') {
      buffer_[length_] = '\0';
      trimLine(buffer_);
      if (length_ > 0) {
        handleLine(buffer_, nowMs);
      }
      length_ = 0;
      buffer_[0] = '\0';
      continue;
    }
    if (length_ < (sizeof(buffer_) - 1)) {
      buffer_[length_++] = c;
    } else {
      length_ = 0;
      buffer_[0] = '\0';
      logger_.error(F("COMMAND TOO LONG"));
    }
  }
}

void CommandInterface::handleLine(char* line, uint32_t nowMs) {
  char* command = strtok(line, " ");
  char* arg1 = strtok(nullptr, " ");
  char* arg2 = strtok(nullptr, " ");

  if (command == nullptr) {
    return;
  }

  if (strcmp(command, "status") == 0) {
    logger_.status(safety_.state(), can_.bitrate(), can_.stats(), experiment_.active(),
                   experiment_.remainingMs(nowMs));
  } else if (strcmp(command, "arm") == 0) {
    if (safety_.arm()) {
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
    logger_.status(safety_.state(), can_.bitrate(), can_.stats(), experiment_.active(),
                   experiment_.remainingMs(nowMs));
  } else if (strcmp(command, "reset") == 0) {
    experiment_.stop();
    experiment_.resetStats();
    if (safety_.disarm()) {
      logger_.ok(F("CONTROL STATE RESET TO SAFE"));
    } else {
      logger_.error(F("RESET REJECTED IN FAULT"));
    }
  } else if (strcmp(command, "help") == 0) {
    logger_.help();
  } else {
    logger_.error(F("UNKNOWN COMMAND"));
  }
}

void CommandInterface::trimLine(char* line) {
  while (length_ > 0 && (line[length_ - 1] == ' ' || line[length_ - 1] == '\t')) {
    line[--length_] = '\0';
  }
  uint8_t leading = 0;
  while (line[leading] == ' ' || line[leading] == '\t') {
    leading++;
  }
  if (leading > 0) {
    memmove(line, line + leading, length_ - leading + 1);
    length_ -= leading;
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
    parsed = (parsed * 10UL) + static_cast<uint32_t>(text[i] - '0');
  }

  value = parsed;
  return true;
}

}  // namespace canchaos
