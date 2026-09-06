#pragma once

#include <stdint.h>

#include "can_frame.h"

namespace canchaos {

enum class CanStatus : uint8_t {
  Offline,
  Online,
  Error,
};

struct CanDriverStats {
  uint32_t txFrames = 0;
  uint32_t rxFrames = 0;
  uint32_t txErrors = 0;
  int lastWriteResult = 0;
};

class CanDriver {
 public:
  bool begin(uint32_t bitrate);
  bool send(const CanFrame& frame);
  bool receive(CanFrame& frame);

  CanStatus status() const;
  const CanDriverStats& stats() const;
  uint32_t bitrate() const;

 private:
  CanStatus status_ = CanStatus::Offline;
  CanDriverStats stats_;
  uint32_t bitrate_ = 0;
};

}  // namespace canchaos
