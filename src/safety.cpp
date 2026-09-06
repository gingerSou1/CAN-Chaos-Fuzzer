#include "safety.h"

namespace canchaos {

void SafetyManager::begin() {
  state_ = SafetyState::Safe;
}

bool SafetyManager::arm() {
  if (state_ != SafetyState::Safe) {
    return false;
  }
  state_ = SafetyState::Armed;
  return true;
}

bool SafetyManager::disarm() {
  if (state_ == SafetyState::Running || state_ == SafetyState::Fault) {
    return false;
  }
  state_ = SafetyState::Safe;
  return true;
}

bool SafetyManager::start() {
  if (state_ != SafetyState::Armed) {
    return false;
  }
  state_ = SafetyState::Running;
  return true;
}

bool SafetyManager::stop() {
  if (state_ != SafetyState::Running) {
    return false;
  }
  state_ = SafetyState::Armed;
  return true;
}

void SafetyManager::fault() {
  state_ = SafetyState::Fault;
}

SafetyState SafetyManager::state() const {
  return state_;
}

bool SafetyManager::canTransmit() const {
  return state_ == SafetyState::Running;
}

const char* toString(SafetyState state) {
  switch (state) {
    case SafetyState::Boot:
      return "BOOT";
    case SafetyState::Safe:
      return "SAFE";
    case SafetyState::Armed:
      return "ARMED";
    case SafetyState::Running:
      return "RUNNING";
    case SafetyState::Fault:
      return "FAULT";
    default:
      return "UNKNOWN";
  }
}

}  // namespace canchaos
