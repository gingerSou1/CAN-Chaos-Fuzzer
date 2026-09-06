#include "can_driver.h"

#include <Arduino.h>
#include <Arduino_CAN.h>
#include "config.h"

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
  if (status_ != CanStatus::Offline || safety_.state() == SafetyState::Fault) return false;
  // Validate before touching the hardware; never silently substitute a bitrate.
  if (bitrate != 125000UL && bitrate != 250000UL && bitrate != 500000UL) {
    return false;
  }
  if (!CAN.begin(toCanBitRate(bitrate))) {
    bitrate_ = 0;
    status_ = CanStatus::Error;
    safety_.fault();
    return false;
  }

  bitrate_ = bitrate;
  status_ = CanStatus::Online;
  return true;
}

bool CanDriver::send(const CanFrame& frame) {
  pollHealth();
  if (!safety_.canTransmit() || status_ != CanStatus::Online || frame.extended ||
      frame.length > 8 || frame.id > 0x7FFUL) {
    stats_.txErrors++;
    stats_.lastWriteResult = -1;
    return false;
  }

  uint32_t const nowMs = millis();
  if (hasAcceptedTx_ && (nowMs - lastAcceptedTxMs_) < kMinTxIntervalMs) {
    ++stats_.txRateLimited;
    return false;
  }
  CanMsg const msg(CanStandardId(frame.id), frame.length, frame.data);
  int const rc = CAN.write(msg);
  stats_.lastWriteResult = rc;

  if (rc == 1) {
    // ArduinoCore-renesas 1.6.0: 1 means R_CAN_Write accepted the frame.
    // This is not an independently confirmed on-bus transmission.
    stats_.txFrames++;
    hasAcceptedTx_ = true;
    lastAcceptedTxMs_ = nowMs;
    return true;
  }

  stats_.txErrors++;
  status_ = CanStatus::Error;
  safety_.fault();
  return false;
}

void CanDriver::pollHealth() {
  int error = 0;
  if (status_ == CanStatus::Online && CAN.isError(error)) {
    lastControllerError_ = error;
    stats_.controllerErrors++;
    status_ = CanStatus::Error;
    safety_.fault();
  }
}

void CanDriver::resetStats() {
  stats_ = CanDriverStats{};
  if (status_ != CanStatus::Error) lastControllerError_ = 0;
}

bool CanDriver::receive(CanFrame& frame) {
  if (status_ != CanStatus::Online || !CAN.available()) {
    return false;
  }

  CanMsg const msg = CAN.read();
  frame.extended = !msg.isStandardId();
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
