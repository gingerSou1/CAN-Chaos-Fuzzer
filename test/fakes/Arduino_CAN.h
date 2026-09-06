#pragma once
#include "Arduino.h"
#include <algorithm>

enum class CanBitRate { BR_125k = 125000, BR_250k = 250000, BR_500k = 500000 };
struct FakeCanId { uint32_t value; bool extended; };
inline FakeCanId CanStandardId(uint32_t value) { return {value, false}; }
inline FakeCanId CanExtendedId(uint32_t value) { return {value, true}; }
struct CanMsg {
  FakeCanId id;
  uint8_t data_length;
  uint8_t data[8] = {};
  CanMsg(FakeCanId value, uint8_t length, const uint8_t* bytes) : id(value), data_length(length) {
    std::copy(bytes, bytes + std::min<uint8_t>(8, length), data);
  }
  bool isStandardId() const { return !id.extended; }
  uint32_t getStandardId() const { return id.value; }
  uint32_t getExtendedId() const { return id.value; }
};
class FakeCAN {
 public:
  bool beginResult = true;
  int writeResult = 1; // Verified R7FA4M1_CAN::write contract in core 1.6.0.
  bool error = false;
  int errorCode = 0;
  unsigned beginCalls = 0;
  unsigned writeCalls = 0;
  unsigned readCalls = 0;
  uint32_t bitrate = 0;
  std::deque<CanMsg> rx;
  std::deque<CanMsg> tx;
  bool begin(CanBitRate value) { ++beginCalls; bitrate = static_cast<uint32_t>(value); return beginResult; }
  int write(const CanMsg& msg) { ++writeCalls; if (writeResult == 1) tx.push_back(msg); return writeResult; }
  bool isError(int& code) const { code = errorCode; return error; }
  size_t available() const { return rx.size(); }
  CanMsg read() { ++readCalls; auto msg = rx.front(); rx.pop_front(); return msg; }
};
inline FakeCAN CAN;
