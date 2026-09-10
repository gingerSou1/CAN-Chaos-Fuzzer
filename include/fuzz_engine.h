// SPDX-License-Identifier: MIT
/**
 * @file
 * @brief Pure deterministic eight-byte payload mutation, without hardware or clocks.
 */
#pragma once

#include <stdint.h>

namespace canchaos {

enum class FuzzStrategy : uint8_t { Random, BitFlip, Zero, FF, Boundary, WalkingBit };
bool parseFuzzStrategy(const char* name, FuzzStrategy& strategy);
const char* toString(FuzzStrategy strategy);
bool validFuzzStrategy(FuzzStrategy strategy);

class FuzzEngine {
 public:
  /// Copy eight base bytes and restart the sequence. Seed zero maps to 0x6D2B79F5.
  /// Invalid strategy leaves the engine unchanged. No hardware, allocation or clock access.
  bool begin(FuzzStrategy strategy, uint32_t seed, const uint8_t (&base)[8]);
  /// Produce eight bytes, advancing once. Output never aliases the stored base.
  /// Determinism covers payload bytes, not transmission timestamps or physical delivery.
  void next(uint8_t (&payload)[8]);

 private:
  uint32_t randomWord();
  FuzzStrategy strategy_ = FuzzStrategy::Zero;
  uint32_t state_ = 0x6D2B79F5U;
  uint8_t base_[8] = {};
  uint8_t position_ = 0;
};

}  // namespace canchaos
