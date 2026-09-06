// SPDX-License-Identifier: MIT
/**
 * @file
 * @brief Classical CAN data-frame representation and receive metadata.
 */

#pragma once

#include <stdint.h>

namespace canchaos {

struct CanFrame {
  uint32_t id = 0;
  bool extended = false;
  uint8_t length = 0;  ///< Payload bytes, 0..8. TX validation rejects larger values.
  uint8_t data[8] = {0};
  uint32_t timestampMs = 0;  ///< Software timestamp; wraps with millis(), not a bus timestamp.
};

}  // namespace canchaos
