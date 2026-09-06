// SPDX-License-Identifier: MIT
/**
 * @file
 * @brief Application transmit permissions and latched safety states.
 */

#pragma once

#include <stdint.h>

namespace canchaos {

enum class SafetyState : uint8_t {
  Boot,
  Safe,
  Armed,
  Running,
  Fault,
};

/// Main-loop-only application state gate; does not control hardware silent mode or queued frames.
class SafetyManager {
 public:
  /// Transition BOOT to SAFE once; cannot clear FAULT or reset an initialized instance.
  void begin();
  /// @return True only when SAFE transitions to ARMED. Does not permit transmission.
  bool arm();
  /// Enter SAFE from SAFE/ARMED/RUNNING; reject BOOT/FAULT. Experiment accounting needs
  /// stop/update.
  bool disarm();
  /// @return True only when ARMED transitions to RUNNING.
  bool start();
  /// @return True only when RUNNING transitions to ARMED.
  bool stop();
  /// Latch FAULT from any state; subsequent transition requests cannot clear it.
  void fault();

  SafetyState state() const;
  /// True only in RUNNING; callers must also validate controller health and rate limits.
  bool canTransmit() const;

 private:
  SafetyState state_ = SafetyState::Boot;
};

const char* toString(SafetyState state);

}  // namespace canchaos
