// SPDX-License-Identifier: MIT
/**
 * @file
 * @brief Interactive console editing, framing and bounded-output regressions.
 */
#include <unity.h>
#include <Arduino_CAN.h>
#include "command_interface.h"
#include "config.h"

using namespace canchaos;

namespace {
struct ConsoleFixture {
  SafetyManager safety;
  CanDriver can{safety};
  ExperimentManager experiment{can, safety};
  FakeStream serial;
  Logger logger{serial};
  CommandInterface commands{serial, safety, can, experiment, logger};
  ConsoleFixture() {
    safety.begin();
    can.begin(kDefaultBitrate);
    commands.begin();
    drain();
  }
  void drain() {
    for (unsigned i = 0; i < 100; ++i) {
      logger.poll();
    }
  }
  void input(const std::string& text) {
    serial.feed(text);
    while (serial.available()) {
      commands.poll(fakeMillis);
    }
    drain();
  }
};

size_t occurrences(const std::string& text, const std::string& part) {
  size_t count = 0;
  for (size_t pos = 0; (pos = text.find(part, pos)) != std::string::npos; pos += part.size()) {
    ++count;
  }
  return count;
}

void test_console_startup_prompt_and_printable_echo() {
  ConsoleFixture f;
  TEST_ASSERT_EQUAL_STRING("STATE: SAFE\r\nType 'help' for commands.\r\nCAN> ",
                           f.serial.output.c_str());
  f.serial.output.clear();
  f.input("sta");
  TEST_ASSERT_EQUAL_STRING("sta", f.serial.output.c_str());
  TEST_ASSERT_EQUAL(0, CAN.writeCalls);
  f.serial.output.clear();
  f.input("tus");
  TEST_ASSERT_EQUAL_STRING("tus", f.serial.output.c_str());
  TEST_ASSERT_EQUAL(SafetyState::Safe, f.safety.state());
}

void test_console_all_printable_bytes_echo_before_enter() {
  ConsoleFixture f;
  // Exercise every printable byte, in chunks shorter than the command limit.
  for (int begin = 32; begin <= 126; begin += 32) {
    std::string text;
    for (int c = begin; c < begin + 32 && c <= 126; ++c) {
      text += static_cast<char>(c);
    }
    f.serial.output.clear();
    f.input(text);
    TEST_ASSERT_EQUAL_STRING(text.c_str(), f.serial.output.c_str());
    f.input("\n");
  }
}

void test_console_lf_executes_once_then_prompt() {
  ConsoleFixture f;
  f.serial.output.clear();
  f.input("arm\n");
  TEST_ASSERT_EQUAL_STRING("arm\r\nOK: STATE ARMED\r\nCAN> ", f.serial.output.c_str());
  TEST_ASSERT_EQUAL(SafetyState::Armed, f.safety.state());
  f.input("start\n");
  TEST_ASSERT_EQUAL(1, f.experiment.stats().started);
}

void test_console_cr_executes_once_then_prompt() {
  ConsoleFixture f;
  f.serial.output.clear();
  f.input("arm\r");
  TEST_ASSERT_EQUAL_STRING("arm\r\nOK: STATE ARMED\r\nCAN> ", f.serial.output.c_str());
  f.input("start\r");
  TEST_ASSERT_EQUAL(1, f.experiment.stats().started);
  TEST_ASSERT_EQUAL(2, occurrences(f.serial.output, "CAN> "));
}

void test_console_crlf_split_and_empty_enter() {
  ConsoleFixture f;
  f.serial.output.clear();
  f.input("arm\r");
  auto first = f.serial.output;
  f.input("\n");
  TEST_ASSERT_EQUAL_STRING(first.c_str(), f.serial.output.c_str());
  f.input("start\r\n");
  TEST_ASSERT_EQUAL(1, f.experiment.stats().started);
  TEST_ASSERT_EQUAL(0, f.experiment.stats().rejectedStarts);
  TEST_ASSERT_EQUAL(2, occurrences(f.serial.output, "CAN> "));
  f.serial.output.clear();
  f.input("\r\n\n\r");
  TEST_ASSERT_EQUAL_STRING("\r\nCAN> \r\nCAN> \r\nCAN> ", f.serial.output.c_str());
  TEST_ASSERT_EQUAL(1, f.experiment.stats().started);
}

void test_console_backspace_and_delete_edit_buffer_and_display() {
  for (char key : {'\b', '\x7F'}) {
    ConsoleFixture f;
    f.serial.output.clear();
    f.input(std::string(2, key));  // Empty buffer cannot erase the prompt.
    TEST_ASSERT_TRUE(f.serial.output.empty());
    f.input(std::string("arx") + key + "m\n");
    TEST_ASSERT_EQUAL_STRING("arx\b \bm\r\nOK: STATE ARMED\r\nCAN> ", f.serial.output.c_str());
    TEST_ASSERT_EQUAL(SafetyState::Armed, f.safety.state());
  }
}

void test_console_whitespace_editing_and_length_boundary() {
  ConsoleFixture f;
  f.serial.output.clear();
  f.input("arm\t\b\v\b\f\b\n");
  TEST_ASSERT_EQUAL_STRING("arm \b \b \b \b \b \b\r\nOK: STATE ARMED\r\nCAN> ",
                           f.serial.output.c_str());
  f.input("disarm\n");
  f.input("arm" + std::string(76, ' ') + "\b \r\n");
  TEST_ASSERT_EQUAL(SafetyState::Armed, f.safety.state());
  TEST_ASSERT_EQUAL(0, occurrences(f.serial.output, "COMMAND TOO LONG"));
}

void test_console_overflow_discard_cannot_be_edited_into_command() {
  ConsoleFixture f;
  f.serial.output.clear();
  f.input(std::string(80, 'x') + std::string(80, '\b') + "arm\r\n");
  std::string expected = std::string(79, 'x') + "\r\nERROR: COMMAND TOO LONG\r\nCAN> ";
  TEST_ASSERT_EQUAL_STRING(expected.c_str(), f.serial.output.c_str());
  TEST_ASSERT_EQUAL(SafetyState::Safe, f.safety.state());
  f.input("arm\n");
  TEST_ASSERT_EQUAL(SafetyState::Armed, f.safety.state());
}

void test_console_malformed_input_is_not_echoed_or_executed() {
  for (char bad : {'\0', '\x1B', static_cast<char>(0x80)}) {
    ConsoleFixture f;
    f.serial.output.clear();
    f.input(std::string("ar") + bad + "m\b\r\n");
    TEST_ASSERT_EQUAL_STRING("ar\r\nERROR: INVALID COMMAND CHARACTER\r\nCAN> ",
                             f.serial.output.c_str());
    TEST_ASSERT_EQUAL(SafetyState::Safe, f.safety.state());
    f.input("arm\n");
    TEST_ASSERT_EQUAL(SafetyState::Armed, f.safety.state());
  }
}

void test_console_help_menu_alias_in_all_control_states() {
  ConsoleFixture f;
  for (int state = 0; state < 4; ++state) {
    if (state == 1) {
      f.input("arm\n");
    }
    if (state == 2) {
      f.input("start\n");
    }
    if (state == 3) {
      f.safety.fault();
      f.experiment.update(0);
    }
    auto before = f.safety.state();
    f.serial.output.clear();
    f.input("help\n");
    auto help = f.serial.output.substr(6);  // Omit echoed command and CRLF.
    f.serial.output.clear();
    f.input("menu\n");
    TEST_ASSERT_EQUAL_STRING(help.c_str(), f.serial.output.substr(6).c_str());
    for (const char* command :
         {"Available commands:", "status", "stats", "arm", "disarm",
          "start [duration_ms] [interval_ms]", "fuzz start <strategy> <seed> <count> <interval_ms>",
          "stop", "reset", "help / menu", "CAN> "}) {
      TEST_ASSERT_NOT_EQUAL_MESSAGE(std::string::npos, help.find(command), command);
    }
    TEST_ASSERT_EQUAL(before, f.safety.state());
  }
  f.input("menu extra\n");
  TEST_ASSERT_NOT_EQUAL(std::string::npos, f.serial.output.find("UNEXPECTED ARGUMENT"));
  TEST_ASSERT_EQUAL(0, CAN.writeCalls);
}

void test_console_crlf_byte_budget_and_one_command_per_poll() {
  ConsoleFixture f;
  f.serial.output.clear();
  f.serial.feed(std::string(28, ' ') + "arm\r\nstart\r\n");
  int before = f.serial.available();
  f.commands.poll(0);
  TEST_ASSERT_EQUAL(kSerialBytesPerLoop, before - f.serial.available());
  TEST_ASSERT_EQUAL(SafetyState::Armed, f.safety.state());
  TEST_ASSERT_FALSE(f.experiment.active());
  TEST_ASSERT_TRUE(f.serial.output.empty());  // Parser never writes directly to serial.
  f.commands.poll(0);
  TEST_ASSERT_EQUAL(1, f.experiment.stats().started);
  f.commands.poll(0);
  TEST_ASSERT_EQUAL(1, f.experiment.stats().started);
  f.drain();
  TEST_ASSERT_EQUAL(2, occurrences(f.serial.output, "CAN> "));
}

void test_console_fragments_are_atomic_under_queue_pressure() {
  BufferedLogOutput buffer;
  FakeStream serial;
  // Fill to 2047 bytes, leaving insufficient room for a three-byte erase sequence.
  for (unsigned i = 0; i < 2047; ++i) {
    buffer.write('x');
    buffer.flushFragment();
  }
  buffer.print("\b \b");
  buffer.flushFragment();
  TEST_ASSERT_EQUAL(1, buffer.droppedLines());
  for (unsigned i = 0; i < 100; ++i) {
    buffer.drain(serial, kLogBytesPerLoop);
  }
  TEST_ASSERT_EQUAL(2047, serial.output.size());
  TEST_ASSERT_EQUAL(std::string::npos, serial.output.find('\b'));
  buffer.print("CAN> ");
  buffer.flushFragment();
  buffer.drain(serial, 5);
  TEST_ASSERT_EQUAL_STRING("CAN> ", serial.output.substr(2047).c_str());
}

void test_console_echo_backpressure_does_not_block_stop() {
  ConsoleFixture f;
  f.input("arm\nfuzz start random 1 10 1000\n");
  f.experiment.update(0);
  f.serial.writable = false;
  for (unsigned i = 0; i < 100; ++i) {
    f.input("status\n");
  }
  f.input("stop\r\n");
  fakeMillis = 1000;
  f.experiment.update(fakeMillis);
  TEST_ASSERT_FALSE(f.experiment.active());
  TEST_ASSERT_EQUAL(SafetyState::Armed, f.safety.state());
  TEST_ASSERT_EQUAL(1, CAN.writeCalls);
  f.serial.writable = true;
  size_t before = f.serial.output.size();
  f.logger.poll();
  TEST_ASSERT_LESS_OR_EQUAL(kLogBytesPerLoop, f.serial.output.size() - before);
  f.drain();
  f.serial.output.clear();
  f.input("help\n");
  TEST_ASSERT_NOT_EQUAL(std::string::npos, f.serial.output.find("Available commands:"));
}
}  // namespace

void runConsoleTests() {
  RUN_TEST(test_console_startup_prompt_and_printable_echo);
  RUN_TEST(test_console_all_printable_bytes_echo_before_enter);
  RUN_TEST(test_console_lf_executes_once_then_prompt);
  RUN_TEST(test_console_cr_executes_once_then_prompt);
  RUN_TEST(test_console_crlf_split_and_empty_enter);
  RUN_TEST(test_console_backspace_and_delete_edit_buffer_and_display);
  RUN_TEST(test_console_whitespace_editing_and_length_boundary);
  RUN_TEST(test_console_overflow_discard_cannot_be_edited_into_command);
  RUN_TEST(test_console_malformed_input_is_not_echoed_or_executed);
  RUN_TEST(test_console_help_menu_alias_in_all_control_states);
  RUN_TEST(test_console_crlf_byte_budget_and_one_command_per_poll);
  RUN_TEST(test_console_fragments_are_atomic_under_queue_pressure);
  RUN_TEST(test_console_echo_backpressure_does_not_block_stop);
}
