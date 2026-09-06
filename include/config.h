#pragma once

#include <stdint.h>

namespace canchaos {

static constexpr uint32_t kDefaultBitrate = 500000UL;
static constexpr uint32_t kDefaultTxIntervalMs = 1000UL;
static constexpr uint32_t kDefaultExperimentDurationMs = 30000UL;
static constexpr uint32_t kMaxExperimentDurationMs = 300000UL;
static constexpr uint32_t kKnownFrameId = 0x123UL;
static constexpr uint32_t kSerialBaud = 115200UL;

}  // namespace canchaos
