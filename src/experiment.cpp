#include "experiment.h"

#include "config.h"

namespace canchaos {

ExperimentManager::ExperimentManager(CanDriver& can, SafetyManager& safety) : can_(can), safety_(safety) {}

bool ExperimentManager::startKnownFrameDemo(uint32_t nowMs, uint32_t durationMs, uint32_t intervalMs) {
  can_.pollHealth();
  if (active_ || can_.status() != CanStatus::Online || durationMs == 0 ||
      durationMs > kMaxExperimentDurationMs || intervalMs < kMinTxIntervalMs ||
      intervalMs > kMaxExperimentDurationMs) {
    stats_.rejectedStarts++;
    return false;
  }

  if (!safety_.start()) {
    stats_.rejectedStarts++;
    return false;
  }

  active_ = true;
  startedAtMs_ = nowMs;
  durationMs_ = durationMs;
  intervalMs_ = intervalMs;
  lastTxMs_ = nowMs - intervalMs_;
  sequence_ = 0;
  stats_.started++;
  return true;
}

void ExperimentManager::stop() {
  if (active_) {
    active_ = false;
    if (safety_.state() == SafetyState::Fault) {
      stats_.faulted++;
    } else {
      stats_.stopped++;
    }
  }
  (void)safety_.stop();
}

void ExperimentManager::update(uint32_t nowMs) {
  can_.pollHealth();
  if (active_ && !safety_.canTransmit()) {
    stop();
  }
  if (!active_) {
    return;
  }

  if ((nowMs - startedAtMs_) >= durationMs_) {
    active_ = false;
    stats_.completed++;
    (void)safety_.stop();
    return;
  }

  if ((nowMs - lastTxMs_) >= intervalMs_) {
    CanFrame const frame = buildKnownFrame(nowMs);
    if (can_.send(frame)) {
      stats_.knownFramesSent++;
      sequence_++;
    } else if (safety_.state() == SafetyState::Fault) {
      stop();
    }
    lastTxMs_ = nowMs;
  }
}

void ExperimentManager::resetStats() {
  stats_ = ExperimentStats{};
}

bool ExperimentManager::active() const {
  return active_ && safety_.canTransmit();
}

uint32_t ExperimentManager::remainingMs(uint32_t nowMs) const {
  if (!active() || (nowMs - startedAtMs_) >= durationMs_) {
    return 0;
  }
  return durationMs_ - (nowMs - startedAtMs_);
}

const ExperimentStats& ExperimentManager::stats() const {
  return stats_;
}

CanFrame ExperimentManager::buildKnownFrame(uint32_t nowMs) {
  CanFrame frame;
  frame.id = kKnownFrameId;
  frame.length = 8;
  frame.timestampMs = nowMs;
  frame.data[0] = 0xCA;
  frame.data[1] = 0xFE;
  frame.data[2] = 0x00;
  frame.data[3] = 0x01;
  frame.data[4] = static_cast<uint8_t>((sequence_ >> 24) & 0xFF);
  frame.data[5] = static_cast<uint8_t>((sequence_ >> 16) & 0xFF);
  frame.data[6] = static_cast<uint8_t>((sequence_ >> 8) & 0xFF);
  frame.data[7] = static_cast<uint8_t>(sequence_ & 0xFF);
  return frame;
}

}  // namespace canchaos
