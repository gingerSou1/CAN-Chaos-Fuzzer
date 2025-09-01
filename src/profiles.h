# pragma once
struct FuzzProfile {
  const char* name;
  uint16_t baseIdMin;
  uint16_t baseIdMax;
  uint8_t  lenMin;
  uint8_t  lenMax;
  uint8_t  bitflipPct;    // 0..100 chance to flip a random bit
  uint16_t jitterMs;      // +/- jitter applied to tx interval
  uint16_t burstCount;    // number of frames per burst (0 = disabled)
  uint16_t burstCooldownMs;
};
