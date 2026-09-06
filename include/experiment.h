#pragma once

#include <stdint.h>

#include "can_driver.h"
#include "safety.h"

namespace canchaos {

struct ExperimentStats {
  uint32_t started = 0;
  uint32_t completed = 0;
  uint32_t stopped = 0;
  uint32_t rejectedStarts = 0;
  uint32_t knownFramesSent = 0;
};

class ExperimentManager {
 public:
  explicit ExperimentManager(CanDriver& can, SafetyManager& safety);

  bool startKnownFrameDemo(uint32_t nowMs, uint32_t durationMs, uint32_t intervalMs);
  void stop();
  void update(uint32_t nowMs);
  void resetStats();

  bool active() const;
  uint32_t remainingMs(uint32_t nowMs) const;
  const ExperimentStats& stats() const;

 private:
  CanFrame buildKnownFrame(uint32_t nowMs);

  CanDriver& can_;
  SafetyManager& safety_;
  ExperimentStats stats_;
  bool active_ = false;
  uint32_t startedAtMs_ = 0;
  uint32_t durationMs_ = 0;
  uint32_t intervalMs_ = 0;
  uint32_t lastTxMs_ = 0;
  uint32_t sequence_ = 0;
};

}  // namespace canchaos
