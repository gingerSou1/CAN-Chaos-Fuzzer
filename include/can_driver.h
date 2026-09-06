#pragma once

#include <stdint.h>

#include "can_frame.h"
#include "safety.h"

namespace canchaos {

enum class CanStatus : uint8_t {
  Offline,
  Online,
  Error,
};

struct CanDriverStats {
  uint32_t txFrames = 0;
  uint32_t rxFrames = 0;
  uint32_t txErrors = 0;
  uint32_t txRateLimited = 0;
  uint32_t controllerErrors = 0;
  int lastWriteResult = 0;
};

class CanDriver {
 public:
  explicit CanDriver(SafetyManager& safety) : safety_(safety) {}
  bool begin(uint32_t bitrate);
  void pollHealth();
  void resetStats();
  bool send(const CanFrame& frame);
  bool receive(CanFrame& frame);

  CanStatus status() const;
  const CanDriverStats& stats() const;
  uint32_t bitrate() const;
  int lastControllerError() const { return lastControllerError_; }

 private:
  SafetyManager& safety_;
  int lastControllerError_ = 0;
  bool hasAcceptedTx_ = false;
  uint32_t lastAcceptedTxMs_ = 0;
  CanStatus status_ = CanStatus::Offline;
  CanDriverStats stats_;
  uint32_t bitrate_ = 0;
};

}  // namespace canchaos
