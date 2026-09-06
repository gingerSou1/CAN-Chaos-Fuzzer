// SPDX-License-Identifier: MIT
/**
 * @file
 * @brief Lifecycle and statistics for the bounded known-frame demo.
 */

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
  uint32_t faulted = 0;
  uint32_t knownFramesSent = 0;
};

/// Owns demo scheduling. Dependencies are non-owning; call only from the cooperative main loop.
class ExperimentManager {
 public:
  explicit ExperimentManager(CanDriver& can, SafetyManager& safety);

  /**
   * @brief Start the known-frame demo only when ARMED and the controller is online.
   *
   * @param nowMs Current millis() snapshot; update() must use the same clock.
   * @param
   * durationMs Nonzero duration, at most kMaxExperimentDurationMs.
   * @param intervalMs Spacing
   * from kMinTxIntervalMs through kMaxExperimentDurationMs.
   * @return False on invalid
   * state/configuration; no frame is written by this call.
   */
  bool startKnownFrameDemo(uint32_t nowMs, uint32_t durationMs, uint32_t intervalMs);
  /// Cancel/account once; RUNNING becomes ARMED, while FAULT remains latched.
  void stop();
  /// Poll health, reconcile cancellation and attempt at most one scheduled frame; no catch-up
  /// burst.
  void update(uint32_t nowMs);
  /// Clear experiment counters only; command dispatch stops first before a user-requested reset.
  void resetStats();

  /// False immediately on loss of transmit permission; stop/update reconciles the outcome counter.
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
