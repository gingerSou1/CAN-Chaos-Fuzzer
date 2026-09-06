#pragma once
// Host-only boundary doubles, not a model of RA4M1 peripheral behavior.
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <deque>
#include <string>
#include <type_traits>

class __FlashStringHelper;
#define F(text) reinterpret_cast<const __FlashStringHelper*>(text)
#define HEX 16
using byte = uint8_t;

class Print {
 public:
  virtual ~Print() = default;
  virtual size_t write(uint8_t value) = 0;
  size_t print(const char* value) {
    size_t count = 0;
    while (*value) count += write(static_cast<uint8_t>(*value++));
    return count;
  }
  size_t print(const __FlashStringHelper* value) { return print(reinterpret_cast<const char*>(value)); }
  size_t print(char value) { return write(static_cast<uint8_t>(value)); }
  template <typename T, typename = std::enable_if_t<std::is_integral<T>::value>>
  size_t print(T value, int base = 10) {
    if (base != HEX) return print(std::to_string(value).c_str());
    char text[32];
    std::snprintf(text, sizeof(text), "%llX", static_cast<unsigned long long>(value));
    return print(text);
  }
  size_t println() { return print("\r\n"); }
  template <typename T> size_t println(T value) { return print(value) + println(); }
  template <typename T> size_t println(T value, int base) { return print(value, base) + println(); }
};

class Stream : public Print {
 public:
  virtual int available() = 0;
  virtual int read() = 0;
};

class FakeStream : public Stream {
 public:
  std::deque<uint8_t> input;
  std::string output;
  bool writable = true;
  void begin(uint32_t) {}
  void feed(const std::string& text) { for (unsigned char c : text) input.push_back(c); }
  int available() override { return static_cast<int>(input.size()); }
  int read() override {
    if (input.empty()) return -1;
    auto c = input.front(); input.pop_front(); return c;
  }
  size_t write(uint8_t value) override {
    if (!writable) return 0;
    output.push_back(static_cast<char>(value)); return 1;
  }
};

inline FakeStream Serial;
inline uint32_t fakeMillis = 0;
inline uint32_t millis() { return fakeMillis; }
inline void delay(uint32_t ms) { fakeMillis += ms; }
