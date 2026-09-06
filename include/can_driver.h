// SPDX-License-Identifier: MIT
/**
 * @file
 * @brief Guarded CAN transport and controller health interface.
 */

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
  uint32_t txFrames = 0;  ///< Accepted writes; not confirmed on-bus delivery.
  uint32_t rxFrames = 0;
  uint32_t txErrors = 0;
  uint32_t txRateLimited = 0;
  uint32_t controllerErrors = 0;
  int lastWriteResult = 0;  ///< Last API result, or -1 for local validation/state rejection.
};

class CanDriver {
 public:
  /// @param safety Non-owning reference; must outlive this driver. Main-loop use only.
  explicit CanDriver(SafetyManager& safety) : safety_(safety) {}
  /**
   * @brief Initialize once at 125000, 250000 or 500000 bits/second.
   * @return False for unsupported rates or repeated/faulted initialization.
   * @note Hardware initialization failure latches FAULT; invalid rates leave hardware untouched.
   */
  bool begin(uint32_t bitrate);
  /// Latch observed Arduino_CAN errors into driver Error and safety FAULT; does not recover them.
  void pollHealth();
  /// Clear counters without changing safety, controller state or the transmit spacing limit.
  void resetStats();
  /**
   * @brief Attempt one standard data-frame write while RUNNING, online and within the rate limit.
   * @return True only for Arduino_CAN acceptance (1); false is not necessarily a hardware fault.
   * @note Local rejection/rate limiting does not fault; a failed hardware write does.
   * @note Does not confirm delivery or cancel previously accepted frames. Main-loop use only.
   */
  bool send(const CanFrame& frame);
  /// Dequeue at most one available data frame; preserve ID format and timestamp it using millis().
  bool receive(CanFrame& frame);

  /// Cached initialization/error state; call pollHealth() to observe new controller errors.
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
