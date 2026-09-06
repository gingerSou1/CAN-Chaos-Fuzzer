#include <unity.h>
#include <Arduino_CAN.h>
#include "command_interface.h"
#include "config.h"

using namespace canchaos;
void setup();
void loop();
extern SafetyManager safety;
extern CanDriver canDriver;
extern ExperimentManager experiment;

void setUp() { CAN = FakeCAN{}; fakeMillis = 0; }
void tearDown() {}

struct Fixture {
  SafetyManager safety;
  CanDriver can{safety};
  ExperimentManager experiment{can, safety};
  FakeStream serial;
  Logger logger{serial};
  CommandInterface commands{serial, safety, can, experiment, logger};
  Fixture() { safety.begin(); can.begin(kDefaultBitrate); commands.begin(); flush(); serial.output.clear(); }
  void flush() { for (unsigned i = 0; i < 100; ++i) logger.poll(); }
  void command(const std::string& line) {
    serial.feed(line);
    while (serial.available()) commands.poll(fakeMillis);
    flush();
  }
};

void test_transitions_and_invalid_transitions() {
  SafetyManager s;
  TEST_ASSERT_FALSE(s.arm()); TEST_ASSERT_FALSE(s.start());
  TEST_ASSERT_FALSE(s.stop()); TEST_ASSERT_FALSE(s.disarm());
  s.begin();
  TEST_ASSERT_EQUAL(SafetyState::Safe, s.state());
  TEST_ASSERT_FALSE(s.start()); TEST_ASSERT_FALSE(s.canTransmit());
  TEST_ASSERT_TRUE(s.arm()); TEST_ASSERT_FALSE(s.arm());
  TEST_ASSERT_FALSE(s.canTransmit()); TEST_ASSERT_TRUE(s.start());
  TEST_ASSERT_TRUE(s.canTransmit()); TEST_ASSERT_FALSE(s.start());
  TEST_ASSERT_FALSE(s.arm()); TEST_ASSERT_TRUE(s.stop());
  TEST_ASSERT_EQUAL(SafetyState::Armed, s.state());
  TEST_ASSERT_TRUE(s.disarm()); TEST_ASSERT_TRUE(s.arm());
  TEST_ASSERT_TRUE(s.start()); TEST_ASSERT_TRUE(s.disarm());
  TEST_ASSERT_EQUAL(SafetyState::Safe, s.state());
}

void test_fault_is_latched_from_every_state() {
  for (int state = 0; state < 4; ++state) {
    SafetyManager s;
    if (state >= 1) s.begin();
    if (state >= 2) s.arm();
    if (state >= 3) s.start();
    s.fault(); s.begin();
    TEST_ASSERT_EQUAL(SafetyState::Fault, s.state());
    TEST_ASSERT_FALSE(s.canTransmit()); TEST_ASSERT_FALSE(s.arm());
    TEST_ASSERT_FALSE(s.start()); TEST_ASSERT_FALSE(s.stop()); TEST_ASSERT_FALSE(s.disarm());
  }
}

void test_arm_and_start_are_separate_and_driver_gates_tx() {
  Fixture f;
  CanFrame frame;
  TEST_ASSERT_FALSE(f.can.send(frame));
  f.command("start\n"); TEST_ASSERT_FALSE(f.experiment.active());
  f.command("arm\n"); TEST_ASSERT_FALSE(f.can.send(frame));
  f.experiment.update(0); TEST_ASSERT_EQUAL(0, CAN.writeCalls);
  f.command("start\n"); f.experiment.update(0);
  TEST_ASSERT_EQUAL(1, CAN.writeCalls);
  TEST_ASSERT_EQUAL(1, f.can.stats().txFrames);
}

void test_overlong_line_discards_suffix_until_newline() {
  Fixture f;
  f.command(std::string(80, 'x') + "arm\nstart\n");
  TEST_ASSERT_EQUAL(SafetyState::Safe, f.safety.state());
  TEST_ASSERT_NOT_EQUAL(std::string::npos, f.serial.output.find("COMMAND TOO LONG"));
  f.command("arm\n"); TEST_ASSERT_EQUAL(SafetyState::Armed, f.safety.state());
  // Boundary: 79 characters fit; 80 including trailing whitespace do not.
  f.command("disarm\n");
  f.command("arm" + std::string(76, ' ') + "\n");
  TEST_ASSERT_EQUAL(SafetyState::Armed, f.safety.state());
}

void test_numeric_overflow_and_boundaries() {
  Fixture f; f.command("arm\n");
  for (const char* text : {"4294967296", "4294968296", "999999999999999999999"}) {
    f.serial.output.clear(); f.command(std::string("start ") + text + "\n");
    TEST_ASSERT_FALSE(f.experiment.active());
    TEST_ASSERT_NOT_EQUAL(std::string::npos, f.serial.output.find("INVALID DURATION"));
  }
  f.serial.output.clear(); f.command("start 4294967295\n");
  TEST_ASSERT_EQUAL(std::string::npos, f.serial.output.find("INVALID DURATION"));
  TEST_ASSERT_FALSE(f.experiment.active()); // Parses uint32 max, rejected by duration policy.
  f.command("start 5000 4294968296\n"); TEST_ASSERT_FALSE(f.experiment.active());
  f.command("start 300000 1000\n"); TEST_ASSERT_TRUE(f.experiment.active());
}

void test_malformed_and_extra_arguments() {
  Fixture f;
  for (const char* name : {"arm", "disarm", "stop", "reset", "stats", "status", "help"}) {
    f.serial.output.clear(); f.command(std::string(name) + " unexpected\n");
    TEST_ASSERT_NOT_EQUAL(std::string::npos, f.serial.output.find("UNEXPECTED ARGUMENT"));
    TEST_ASSERT_EQUAL(SafetyState::Safe, f.safety.state());
  }
  f.command("arm\n");
  for (const char* line : {"start 5000 1000 extra\n", "start -1\n", "start +1\n",
                          "start 0x10\n", "start 1.5\n", "start 0\n", "start 300001\n",
                          "start 5000 0\n", "start 5000 999\n", "unknown\n"}) {
    f.command(line); TEST_ASSERT_FALSE(f.experiment.active());
  }
  f.command(std::string("start\0 5000\n", 12));
  TEST_ASSERT_FALSE(f.experiment.active());
}

void test_whitespace_and_crlf() {
  Fixture f;
  f.command(" \t\r\n\t arm \r\n");
  TEST_ASSERT_EQUAL(SafetyState::Armed, f.safety.state());
  f.command("\vstart\t5000\f1000\t\r\n");
  TEST_ASSERT_TRUE(f.experiment.active());
}

void test_serial_budget_and_one_command_per_poll() {
  Fixture f;
  f.serial.feed(std::string(100, ' ') + "arm\n");
  int before = f.serial.available(); f.commands.poll(0);
  TEST_ASSERT_EQUAL(kSerialBytesPerLoop, before - f.serial.available());
  while (f.serial.available()) f.commands.poll(0);
  f.serial.feed("arm\nstart\n"); f.commands.poll(0);
  TEST_ASSERT_EQUAL(SafetyState::Armed, f.safety.state());
  TEST_ASSERT_FALSE(f.experiment.active());
  f.commands.poll(0); TEST_ASSERT_TRUE(f.experiment.active());
}

void test_stop_disarm_and_repeated_cancellation() {
  Fixture f; f.command("arm\nstart\n"); f.experiment.update(0);
  f.command("stop\n"); f.experiment.update(2000);
  TEST_ASSERT_EQUAL(1, CAN.writeCalls); TEST_ASSERT_FALSE(f.experiment.active());
  TEST_ASSERT_EQUAL(SafetyState::Armed, f.safety.state());
  f.command("stop\n"); TEST_ASSERT_EQUAL(1, f.experiment.stats().stopped);
  f.command("start\ndisarm\n"); f.experiment.update(3000);
  TEST_ASSERT_EQUAL(SafetyState::Safe, f.safety.state());
  TEST_ASSERT_EQUAL(1, CAN.writeCalls);
}

void test_completion_wraparound_and_no_catchup_burst() {
  Fixture f; f.safety.arm();
  uint32_t start = UINT32_MAX - 500;
  TEST_ASSERT_TRUE(f.experiment.startKnownFrameDemo(start, 5000, 1000));
  fakeMillis = start; f.experiment.update(start);
  fakeMillis = start + 4000U; f.experiment.update(fakeMillis);
  TEST_ASSERT_EQUAL(2, CAN.writeCalls); // One frame per update, no catch-up burst.
  f.experiment.update(start + 5000U);
  TEST_ASSERT_FALSE(f.experiment.active()); TEST_ASSERT_EQUAL(1, f.experiment.stats().completed);
  TEST_ASSERT_EQUAL(SafetyState::Armed, f.safety.state());
}

void test_runtime_write_failure_faults_and_cancels() {
  Fixture f; f.command("arm\nstart\n"); CAN.writeResult = -7;
  f.experiment.update(0);
  TEST_ASSERT_EQUAL(SafetyState::Fault, f.safety.state());
  TEST_ASSERT_EQUAL(CanStatus::Error, f.can.status());
  TEST_ASSERT_FALSE(f.experiment.active()); TEST_ASSERT_EQUAL(0, f.experiment.remainingMs(0));
  TEST_ASSERT_EQUAL(1, f.experiment.stats().faulted);
  TEST_ASSERT_EQUAL(0, f.can.stats().txFrames); TEST_ASSERT_EQUAL(-7, f.can.stats().lastWriteResult);
  f.command("reset\narm\nstart\ndisarm\n"); f.experiment.update(1000);
  TEST_ASSERT_EQUAL(SafetyState::Fault, f.safety.state()); TEST_ASSERT_EQUAL(1, CAN.writeCalls);
  TEST_ASSERT_EQUAL(1, f.can.stats().txErrors); // Rejected reset preserves evidence.
}

void test_async_controller_error_and_external_fault() {
  Fixture f; f.command("arm\nstart\n");
  CAN.error = true; CAN.errorCode = 42; f.can.pollHealth();
  TEST_ASSERT_FALSE(f.experiment.active());
  f.experiment.update(0); f.experiment.update(1000);
  TEST_ASSERT_EQUAL(0, CAN.writeCalls); TEST_ASSERT_EQUAL(1, f.experiment.stats().faulted);
  TEST_ASSERT_EQUAL(1, f.can.stats().controllerErrors); TEST_ASSERT_EQUAL(42, f.can.lastControllerError());
  TEST_ASSERT_FALSE(f.can.begin(500000));
  CAN = FakeCAN{};
  Fixture g; g.command("arm\nstart\n"); g.safety.fault();
  TEST_ASSERT_FALSE(g.experiment.active()); g.experiment.update(0);
  TEST_ASSERT_EQUAL(1, g.experiment.stats().faulted); TEST_ASSERT_EQUAL(0, CAN.writeCalls);
}

void test_bitrate_validation_and_initialization_failure() {
  SafetyManager s; s.begin(); CanDriver driver(s);
  TEST_ASSERT_FALSE(driver.begin(123456)); TEST_ASSERT_EQUAL(0, CAN.beginCalls);
  TEST_ASSERT_EQUAL(CanStatus::Offline, driver.status()); TEST_ASSERT_EQUAL(0, driver.bitrate());
  CAN.beginResult = false; TEST_ASSERT_FALSE(driver.begin(500000));
  TEST_ASSERT_EQUAL(SafetyState::Fault, s.state()); TEST_ASSERT_EQUAL(CanStatus::Error, driver.status());
  for (uint32_t bitrate : {125000U, 250000U, 500000U}) {
    CAN = FakeCAN{}; SafetyManager other; other.begin(); CanDriver can(other);
    TEST_ASSERT_TRUE(can.begin(bitrate)); TEST_ASSERT_EQUAL(bitrate, CAN.bitrate);
    TEST_ASSERT_FALSE(can.begin(123456)); TEST_ASSERT_EQUAL(bitrate, can.bitrate());
  }
}

void test_frame_formats_and_write_contract() {
  Fixture f; f.command("arm\nstart\n");
  CanFrame frame; frame.extended = true;
  TEST_ASSERT_FALSE(f.can.send(frame)); frame.extended = false; frame.id = 0x800;
  TEST_ASSERT_FALSE(f.can.send(frame)); frame.id = 0x123; frame.length = 9;
  TEST_ASSERT_FALSE(f.can.send(frame)); TEST_ASSERT_EQUAL(0, CAN.writeCalls);
  frame.length = 8; TEST_ASSERT_TRUE(f.can.send(frame)); TEST_ASSERT_EQUAL(1, CAN.writeCalls);
  uint8_t data[8] = {1, 2, 3};
  CAN.rx.emplace_back(CanExtendedId(0x123), 3, data);
  TEST_ASSERT_TRUE(f.can.receive(frame)); TEST_ASSERT_TRUE(frame.extended);
  TEST_ASSERT_EQUAL(0x123, frame.id); TEST_ASSERT_EQUAL(3, frame.length); TEST_ASSERT_EQUAL(0, frame.data[7]);
  CAN.rx.emplace_back(CanStandardId(0x123), 3, data);
  TEST_ASSERT_TRUE(f.can.receive(frame)); TEST_ASSERT_FALSE(frame.extended);
}

void test_rate_limit_survives_restart_and_counter_reset() {
  Fixture f; f.command("arm\nstart\n"); f.experiment.update(0);
  f.command("stop\nstart\n"); f.experiment.update(0);
  TEST_ASSERT_EQUAL(1, CAN.writeCalls); TEST_ASSERT_EQUAL(1, f.can.stats().txRateLimited);
  f.command("reset\narm\nstart\n"); f.experiment.update(0);
  TEST_ASSERT_EQUAL(1, CAN.writeCalls); TEST_ASSERT_EQUAL(1, f.can.stats().txRateLimited);
  fakeMillis = 1000; f.experiment.update(fakeMillis);
  TEST_ASSERT_EQUAL(2, CAN.writeCalls);
}

void test_reset_counters_and_status() {
  Fixture f; f.command("start\narm\nstart\n"); f.experiment.update(0);
  f.command("reset\n");
  TEST_ASSERT_EQUAL(SafetyState::Safe, f.safety.state()); TEST_ASSERT_FALSE(f.experiment.active());
  TEST_ASSERT_EQUAL(0, f.can.stats().txFrames); TEST_ASSERT_EQUAL(0, f.can.stats().rxFrames);
  TEST_ASSERT_EQUAL(0, f.can.stats().txErrors); TEST_ASSERT_EQUAL(0, f.experiment.stats().started);
  TEST_ASSERT_EQUAL(0, f.experiment.stats().stopped); TEST_ASSERT_EQUAL(0, f.experiment.stats().rejectedStarts);
  CAN.error = true; CAN.errorCode = 1; f.can.pollHealth(); f.command("status\n");
  TEST_ASSERT_NOT_EQUAL(std::string::npos, f.serial.output.find("CAN: ERROR"));
}

void test_logging_budget_backpressure_and_whole_line_drops() {
  BufferedLogOutput out; FakeStream sink;
  for (int i = 0; i < 500; ++i) out.println("1234567890");
  TEST_ASSERT_GREATER_THAN(0, out.droppedLines());
  out.drain(sink, 32); TEST_ASSERT_EQUAL(32, sink.output.size());
  sink.writable = false; out.drain(sink, 32); TEST_ASSERT_EQUAL(32, sink.output.size());
  sink.writable = true; for (int i = 0; i < 100; ++i) out.drain(sink, 32);
  TEST_ASSERT_EQUAL(0, sink.output.size() % 12);
  out.resetStats(); TEST_ASSERT_EQUAL(0, out.droppedLines());
}

void test_actual_loop_bounds_and_stop_with_rx_backlog() {
  Serial = FakeStream{}; setup();
  Serial.feed("arm\nstart\n"); loop(); loop();
  TEST_ASSERT_TRUE(experiment.active());
  uint8_t data[8] = {};
  for (int i = 0; i < 100; ++i) CAN.rx.emplace_back(CanStandardId(0x123), 8, data);
  Serial.feed("stop\n"); auto writes = CAN.writeCalls; auto bytes = Serial.output.size();
  fakeMillis += 2000; loop();
  TEST_ASSERT_FALSE(experiment.active()); TEST_ASSERT_EQUAL(writes, CAN.writeCalls);
  TEST_ASSERT_EQUAL(kCanRxFramesPerLoop, CAN.readCalls);
  TEST_ASSERT_LESS_OR_EQUAL(kLogBytesPerLoop, Serial.output.size() - bytes);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_transitions_and_invalid_transitions);
  RUN_TEST(test_fault_is_latched_from_every_state);
  RUN_TEST(test_arm_and_start_are_separate_and_driver_gates_tx);
  RUN_TEST(test_overlong_line_discards_suffix_until_newline);
  RUN_TEST(test_numeric_overflow_and_boundaries);
  RUN_TEST(test_malformed_and_extra_arguments);
  RUN_TEST(test_whitespace_and_crlf);
  RUN_TEST(test_serial_budget_and_one_command_per_poll);
  RUN_TEST(test_stop_disarm_and_repeated_cancellation);
  RUN_TEST(test_completion_wraparound_and_no_catchup_burst);
  RUN_TEST(test_runtime_write_failure_faults_and_cancels);
  RUN_TEST(test_async_controller_error_and_external_fault);
  RUN_TEST(test_bitrate_validation_and_initialization_failure);
  RUN_TEST(test_frame_formats_and_write_contract);
  RUN_TEST(test_rate_limit_survives_restart_and_counter_reset);
  RUN_TEST(test_reset_counters_and_status);
  RUN_TEST(test_logging_budget_backpressure_and_whole_line_drops);
  RUN_TEST(test_actual_loop_bounds_and_stop_with_rx_backlog);
  return UNITY_END();
}
