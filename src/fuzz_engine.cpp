// SPDX-License-Identifier: MIT
/**
 * @file
 * @brief Explicit xorshift32 and deterministic payload strategy ordering.
 */
#include "fuzz_engine.h"

#include <string.h>

namespace canchaos {

bool validFuzzStrategy(FuzzStrategy strategy) {
  return static_cast<uint8_t>(strategy) <= static_cast<uint8_t>(FuzzStrategy::WalkingBit);
}

const char* toString(FuzzStrategy strategy) {
  switch (strategy) {
    case FuzzStrategy::Random:
      return "random";
    case FuzzStrategy::BitFlip:
      return "bitflip";
    case FuzzStrategy::Zero:
      return "zero";
    case FuzzStrategy::FF:
      return "ff";
    case FuzzStrategy::Boundary:
      return "boundary";
    case FuzzStrategy::WalkingBit:
      return "walkingbit";
    default:
      return "unknown";
  }
}

bool parseFuzzStrategy(const char* name, FuzzStrategy& strategy) {
  if (name == nullptr) {
    return false;
  }
  for (uint8_t i = 0; i <= static_cast<uint8_t>(FuzzStrategy::WalkingBit); ++i) {
    auto candidate = static_cast<FuzzStrategy>(i);
    if (strcmp(name, toString(candidate)) == 0) {
      strategy = candidate;
      return true;
    }
  }
  return false;
}

bool FuzzEngine::begin(FuzzStrategy strategy, uint32_t seed, const uint8_t (&base)[8]) {
  if (!validFuzzStrategy(strategy)) {
    return false;
  }
  strategy_ = strategy;
  state_ = seed == 0 ? 0x6D2B79F5U : seed;
  position_ = 0;
  memcpy(base_, base, sizeof(base_));
  return true;
}

uint32_t FuzzEngine::randomWord() {
  state_ ^= state_ << 13;
  state_ ^= state_ >> 17;
  state_ ^= state_ << 5;
  return state_;
}

void FuzzEngine::next(uint8_t (&payload)[8]) {
  static constexpr uint8_t boundaries[] = {0x00, 0x01, 0x7F, 0x80, 0xFE, 0xFF};
  memcpy(payload, base_, sizeof(base_));
  switch (strategy_) {
    case FuzzStrategy::Random:
      for (uint8_t& byte : payload) {
        byte = static_cast<uint8_t>(randomWord());
      }
      break;
    case FuzzStrategy::BitFlip: {
      uint8_t bit = static_cast<uint8_t>(randomWord() & 63U);
      payload[bit / 8] ^= static_cast<uint8_t>(1U << (bit % 8));
      break;
    }
    case FuzzStrategy::Zero:
      memset(payload, 0, sizeof(payload));
      break;
    case FuzzStrategy::FF:
      memset(payload, 0xFF, sizeof(payload));
      break;
    case FuzzStrategy::Boundary:
      memset(payload, boundaries[position_], sizeof(payload));
      position_ = static_cast<uint8_t>((position_ + 1) % 6);
      break;
    case FuzzStrategy::WalkingBit:
      memset(payload, 0, sizeof(payload));
      payload[7 - position_ / 8] = static_cast<uint8_t>(1U << (position_ % 8));
      position_ = static_cast<uint8_t>((position_ + 1) % 64);
      break;
  }
}

}  // namespace canchaos
