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

class SafetyManager {
 public:
  void begin();
  bool arm();
  bool disarm();
  bool start();
  bool stop();
  void fault();

  SafetyState state() const;
  bool canTransmit() const;

 private:
  SafetyState state_ = SafetyState::Boot;
};

const char* toString(SafetyState state);

}  // namespace canchaos
