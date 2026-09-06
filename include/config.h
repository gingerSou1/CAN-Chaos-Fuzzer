// SPDX-License-Identifier: MIT
/**
 * @file
 * @brief Milestone 1 timing, processing-budget and CAN defaults.
 */

#pragma once

#include <stdint.h>

namespace canchaos {

static constexpr uint32_t kDefaultBitrate = 500000UL;
static constexpr uint32_t kDefaultTxIntervalMs = 1000UL;
// Milestone 1 is a low-rate known-frame check (at most one frame/second).
static constexpr uint32_t kMinTxIntervalMs = 1000UL;
static constexpr uint8_t kSerialBytesPerLoop = 32;
static constexpr uint8_t kCanRxFramesPerLoop = 4;
static constexpr uint8_t kLogBytesPerLoop = 32;
static constexpr uint32_t kDefaultExperimentDurationMs = 30000UL;
static constexpr uint32_t kMaxExperimentDurationMs = 300000UL;
static constexpr uint32_t kKnownFrameId = 0x123UL;
static constexpr uint32_t kSerialBaud = 115200UL;

}  // namespace canchaos
