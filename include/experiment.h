// SPDX-License-Identifier: MIT
/**
 * @file
 * @brief Lifecycle and statistics for bounded known-frame and payload fuzz experiments.
 */

#pragma once

#include <stdint.h>

#include "can_driver.h"
#include "safety.h"
#include "fuzz_engine.h"

namespace canchaos {

struct FuzzRun {
  FuzzStrategy strategy = FuzzStrategy::Zero;
  uint32_t seed = 0;
  uint32_t requested = 0;
  uint32_t generated = 0;
  uint32_t accepted = 0;
  uint32_t intervalMs = 0;
  CanFrame lastFrame;
};

struct ExperimentStats {
  uint32_t started = 0;
  uint32_t completed = 0;
  uint32_t stopped = 0;
  uint32_t rejectedStarts = 0;
  uint32_t faulted = 0;
  uint32_t knownFramesSent = 0;
};

/// Owns experiment scheduling. Dependencies are non-owning; cooperative main-loop use only.
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
  /// Start from ARMED. Count*interval must fit the existing duration limit.
  /// Retain a rate-limited payload for retry; timeout accounts as stopped, not completed.
  bool startFuzz(uint32_t nowMs, FuzzStrategy strategy, uint32_t seed, uint32_t count,
                 uint32_t intervalMs);
  bool fuzzMode() const { return fuzzMode_; }
  const FuzzRun& fuzzRun() const { return fuzzRun_; }
  /// Cancel/account once; RUNNING becomes ARMED, while FAULT remains latched.
  void stop();
  /// Poll health, reconcile cancellation and attempt at most one scheduled frame; no catch-up
  /// burst.
  void update(uint32_t nowMs);
  /// Clear aggregate counters; clear fuzz metadata only when inactive, preserving active progress.
  /// Command dispatch stops first before a user-requested reset.
  void resetStats();

  /// False immediately on loss of transmit permission; stop/update reconciles the outcome counter.
  bool active() const;
  uint32_t remainingMs(uint32_t nowMs) const;
  const ExperimentStats& stats() const;

 private:
  CanFrame buildKnownFrame(uint32_t nowMs);
  void updateFuzz(uint32_t nowMs);

  CanDriver& can_;
  SafetyManager& safety_;
  ExperimentStats stats_;
  FuzzEngine fuzzEngine_;
  FuzzRun fuzzRun_;
  bool fuzzMode_ = false;
  bool fuzzPending_ = false;
  bool active_ = false;
  uint32_t startedAtMs_ = 0;
  uint32_t durationMs_ = 0;
  uint32_t intervalMs_ = 0;
  uint32_t lastTxMs_ = 0;
  uint32_t sequence_ = 0;
};

}  // namespace canchaos
