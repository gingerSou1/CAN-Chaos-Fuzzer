#include "can_driver.h"

#include <Arduino.h>
#include <Arduino_CAN.h>

namespace canchaos {

namespace {

CanBitRate toCanBitRate(uint32_t bitrate) {
  switch (bitrate) {
    case 125000UL:
      return CanBitRate::BR_125k;
    case 250000UL:
      return CanBitRate::BR_250k;
    case 500000UL:
    default:
      return CanBitRate::BR_500k;
  }
}

}  // namespace

bool CanDriver::begin(uint32_t bitrate) {
  if (!CAN.begin(toCanBitRate(bitrate))) {
    bitrate_ = 0;
    status_ = CanStatus::Error;
    return false;
  }

  bitrate_ = bitrate;
  status_ = CanStatus::Online;
  return true;
}

bool CanDriver::send(const CanFrame& frame) {
  if (status_ != CanStatus::Online || frame.length > 8 || frame.id > 0x7FFUL) {
    stats_.txErrors++;
    stats_.lastWriteResult = -1;
    return false;
  }

  CanMsg const msg(CanStandardId(frame.id), frame.length, frame.data);
  int const rc = CAN.write(msg);
  stats_.lastWriteResult = rc;

  if (rc == 1) {
    stats_.txFrames++;
    return true;
  }

  stats_.txErrors++;
  return false;
}

bool CanDriver::receive(CanFrame& frame) {
  if (status_ != CanStatus::Online || !CAN.available()) {
    return false;
  }

  CanMsg const msg = CAN.read();
  frame.id = msg.isStandardId() ? msg.getStandardId() : msg.getExtendedId();
  frame.length = msg.data_length > 8 ? 8 : msg.data_length;
  for (uint8_t i = 0; i < frame.length; ++i) {
    frame.data[i] = msg.data[i];
  }
  for (uint8_t i = frame.length; i < 8; ++i) {
    frame.data[i] = 0;
  }
  frame.timestampMs = millis();
  stats_.rxFrames++;
  return true;
}

CanStatus CanDriver::status() const {
  return status_;
}

const CanDriverStats& CanDriver::stats() const {
  return stats_;
}

uint32_t CanDriver::bitrate() const {
  return bitrate_;
}

}  // namespace canchaos
