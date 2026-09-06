#pragma once

#include <stdint.h>

namespace canchaos {

struct CanFrame {
  uint32_t id = 0;
  uint8_t length = 0;
  uint8_t data[8] = {0};
  uint32_t timestampMs = 0;
};

}  // namespace canchaos
