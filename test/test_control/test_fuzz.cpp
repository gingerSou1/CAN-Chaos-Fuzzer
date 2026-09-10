// SPDX-License-Identifier: MIT
/**
 * @file
 * @brief Deterministic vectors and fuzz orchestration safety regressions.
 */
#include <unity.h>
#include <Arduino_CAN.h>
#include <string.h>
#include "command_interface.h"
#include "config.h"
#include "fuzz_engine.h"

using namespace canchaos;

namespace {
const uint8_t kBase[8] = {0xCA, 0xFE, 0, 1, 0, 0, 0, 0};

struct FuzzFixture {
  SafetyManager safety;
  CanDriver can{safety};
  ExperimentManager experiment{can, safety};
  FakeStream serial;
  Logger logger{serial};
  CommandInterface commands{serial, safety, can, experiment, logger};
  FuzzFixture() {
    safety.begin();
    can.begin(kDefaultBitrate);
  }
  void command(const std::string& line) {
    serial.feed(line);
    while (serial.available()) {
      commands.poll(fakeMillis);
    }
    for (unsigned i = 0; i < 100; ++i) {
      logger.poll();
    }
  }
  void tick(uint32_t now) {
    fakeMillis = now;
    experiment.update(now);
  }
};

void test_random_vectors_and_repeatability() {
  FuzzEngine a, b, other;
  a.begin(FuzzStrategy::Random, 1, kBase);
  b.begin(FuzzStrategy::Random, 1, kBase);
  other.begin(FuzzStrategy::Random, 1337, kBase);
  const uint8_t first[8] = {33, 1, 197, 79, 209, 208, 26, 178};
  uint8_t x[8], y[8], z[8];
  bool differs = false;
  for (unsigned i = 0; i < 100; ++i) {
    a.next(x);
    b.next(y);
    other.next(z);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(x, y, 8);
    if (i == 0) {
      TEST_ASSERT_EQUAL_UINT8_ARRAY(first, x, 8);
    }
    differs = differs || memcmp(x, z, 8) != 0;
  }
  TEST_ASSERT_TRUE(differs);
}

void test_seed_zero_and_engine_restart() {
  FuzzEngine a, b;
  a.begin(FuzzStrategy::Random, 0, kBase);
  b.begin(FuzzStrategy::Random, 0x6D2B79F5U, kBase);
  uint8_t first[8], x[8], y[8];
  a.next(first);
  b.next(y);
  TEST_ASSERT_EQUAL_UINT8_ARRAY(first, y, 8);
  a.next(x);
  a.begin(FuzzStrategy::Random, 0, kBase);
  a.next(x);
  TEST_ASSERT_EQUAL_UINT8_ARRAY(first, x, 8);
}

void test_constant_and_boundary_sequences() {
  const uint8_t values[] = {0, 1, 0x7F, 0x80, 0xFE, 0xFF};
  for (FuzzStrategy strategy : {FuzzStrategy::Zero, FuzzStrategy::FF, FuzzStrategy::Boundary}) {
    FuzzEngine engine;
    engine.begin(strategy, 42, kBase);
    for (unsigned i = 0; i < 18; ++i) {
      uint8_t payload[8];
      engine.next(payload);
      uint8_t expected = strategy == FuzzStrategy::Zero ? 0
                         : strategy == FuzzStrategy::FF ? 0xFF
                                                        : values[i % 6];
      for (uint8_t byte : payload) {
        TEST_ASSERT_EQUAL_UINT8(expected, byte);
      }
    }
  }
}

void test_walking_bit_complete_cycle() {
  FuzzEngine engine;
  engine.begin(FuzzStrategy::WalkingBit, UINT32_MAX, kBase);
  for (unsigned i = 0; i < 128; ++i) {
    uint8_t payload[8];
    engine.next(payload);
    for (unsigned j = 0; j < 8; ++j) {
      uint8_t expected = j == 7 - (i % 64) / 8 ? 1U << (i % 8) : 0;
      TEST_ASSERT_EQUAL_UINT8(expected, payload[j]);
    }
  }
}

void test_bitflip_base_copy_and_exact_bits() {
  FuzzEngine engine;
  uint8_t base[8];
  memcpy(base, kBase, 8);
  engine.begin(FuzzStrategy::BitFlip, 1, base);
  memset(base, 0, 8);  // Engine owns a copy.
  const uint8_t bits[] = {33, 1, 5, 15, 17, 16, 26, 50};
  for (uint8_t bit : bits) {
    uint8_t payload[8], expected[8];
    memcpy(expected, kBase, 8);
    expected[bit / 8] ^= 1U << (bit % 8);
    engine.next(payload);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expected, payload, 8);
  }
  engine.begin(FuzzStrategy::BitFlip, 1, kBase);
  uint8_t payload[8], expected[8];
  memcpy(expected, kBase, 8);
  expected[4] ^= 2;
  engine.next(payload);
  TEST_ASSERT_EQUAL_UINT8_ARRAY(expected, payload, 8);
}

void test_fuzz_explicit_arm_start_and_completion() {
  FuzzFixture f;
  f.command("fuzz start random 1 3 1000\n");
  f.tick(0);
  TEST_ASSERT_EQUAL(0, CAN.writeCalls);
  TEST_ASSERT_EQUAL(1, f.experiment.stats().rejectedStarts);
  f.command("arm\n");
  f.tick(0);
  TEST_ASSERT_EQUAL(0, CAN.writeCalls);
  f.command("fuzz start random 1 3 1000\n");
  TEST_ASSERT_EQUAL(0, CAN.writeCalls);
  f.tick(0);
  f.tick(1000);
  f.tick(2000);
  TEST_ASSERT_EQUAL(3, CAN.tx.size());
  TEST_ASSERT_EQUAL(3, f.experiment.fuzzRun().generated);
  TEST_ASSERT_EQUAL(3, f.experiment.fuzzRun().accepted);
  TEST_ASSERT_EQUAL(1, f.experiment.stats().completed);
  TEST_ASSERT_EQUAL(0, f.experiment.stats().knownFramesSent);
  TEST_ASSERT_EQUAL(SafetyState::Armed, f.safety.state());
  TEST_ASSERT_FALSE(f.experiment.active());
  for (const CanMsg& msg : CAN.tx) {
    TEST_ASSERT_TRUE(msg.isStandardId());
    TEST_ASSERT_EQUAL(0x123, msg.getStandardId());
    TEST_ASSERT_EQUAL(8, msg.data_length);
  }
}

void test_repeated_fuzz_experiments_reproduce_all_strategies() {
  FuzzFixture f;
  f.command("arm\n");
  uint32_t now = 0;
  for (FuzzStrategy strategy :
       {FuzzStrategy::Random, FuzzStrategy::BitFlip, FuzzStrategy::Zero, FuzzStrategy::FF,
        FuzzStrategy::Boundary, FuzzStrategy::WalkingBit}) {
    std::deque<CanMsg> first;
    for (unsigned run = 0; run < 2; ++run) {
      CAN.tx.clear();
      TEST_ASSERT_TRUE(f.experiment.startFuzz(now, strategy, 1337, 4, 1000));
      for (unsigned i = 0; i < 4; ++i) {
        f.tick(now);
        now += 1000;
      }
      TEST_ASSERT_EQUAL(4, CAN.tx.size());
      if (run == 0) {
        first = CAN.tx;
      } else {
        for (unsigned i = 0; i < 4; ++i) {
          TEST_ASSERT_EQUAL_UINT8_ARRAY(first[i].data, CAN.tx[i].data, 8);
        }
      }
    }
  }
}

void test_rate_limit_retains_generated_frame() {
  FuzzFixture f;
  f.command("arm\nstart 5000 1000\n");
  f.tick(0);
  f.command("stop\nfuzz start random 1 2 1000\n");
  f.tick(0);
  TEST_ASSERT_EQUAL(1, CAN.writeCalls);
  TEST_ASSERT_EQUAL(1, f.can.stats().txRateLimited);
  TEST_ASSERT_EQUAL(1, f.experiment.fuzzRun().generated);
  TEST_ASSERT_EQUAL(0, f.experiment.fuzzRun().accepted);
  uint8_t pending[8];
  memcpy(pending, f.experiment.fuzzRun().lastFrame.data, 8);
  f.experiment.resetStats();  // Direct API reset must retain active mode and pending data.
  TEST_ASSERT_TRUE(f.experiment.fuzzMode());
  f.tick(1000);
  TEST_ASSERT_EQUAL_UINT8_ARRAY(pending, CAN.tx.back().data, 8);
  TEST_ASSERT_EQUAL(1, f.experiment.fuzzRun().generated);
  TEST_ASSERT_EQUAL(1, f.experiment.fuzzRun().accepted);
  f.tick(2000);  // Deadline, not a false successful completion.
  TEST_ASSERT_FALSE(f.experiment.active());
  TEST_ASSERT_EQUAL(0, f.experiment.stats().completed);
  TEST_ASSERT_EQUAL(1, f.experiment.stats().stopped);
}

void test_fuzz_stop_disarm_reset_and_known_demo_preserved() {
  FuzzFixture f;
  f.command("arm\nfuzz start ff 1 10 1000\n");
  f.tick(0);
  f.command("stop\nstop\n");
  f.tick(1000);
  TEST_ASSERT_EQUAL(1, CAN.writeCalls);
  TEST_ASSERT_EQUAL(1, f.experiment.stats().stopped);
  f.command("fuzz start zero 1 2 1000\ndisarm\n");
  f.tick(2000);
  TEST_ASSERT_EQUAL(SafetyState::Safe, f.safety.state());
  TEST_ASSERT_EQUAL(1, CAN.writeCalls);
  f.command("reset\n");
  TEST_ASSERT_EQUAL(0, f.experiment.fuzzRun().generated);
  f.command("arm\nstart 5000 1000\n");
  f.tick(2000);
  f.tick(3000);
  TEST_ASSERT_EQUAL_UINT8_ARRAY(kBase, CAN.tx[1].data, 8);
  TEST_ASSERT_EQUAL(1, CAN.tx[2].data[7]);
  TEST_ASSERT_EQUAL(2, f.experiment.stats().knownFramesSent);
  TEST_ASSERT_FALSE(f.experiment.fuzzMode());
}

void test_fuzz_write_fault_latched_recovery_and_fresh_boot() {
  FuzzFixture f;
  f.command("arm\nfuzz start random 1 4 1000\n");
  f.tick(0);
  CAN.writeResult = -60003;
  f.tick(1000);
  TEST_ASSERT_EQUAL(SafetyState::Fault, f.safety.state());
  TEST_ASSERT_EQUAL(CanStatus::Error, f.can.status());
  TEST_ASSERT_EQUAL(1, f.experiment.stats().faulted);
  TEST_ASSERT_EQUAL(0, f.experiment.stats().completed);
  TEST_ASSERT_EQUAL(2, f.experiment.fuzzRun().generated);
  TEST_ASSERT_EQUAL(1, f.experiment.fuzzRun().accepted);
  CAN.writeResult = 1;
  f.command("reset\narm\nfuzz start zero 1 2 1000\n");
  f.tick(2000);
  f.tick(3000);
  TEST_ASSERT_EQUAL(2, CAN.writeCalls);
  TEST_ASSERT_EQUAL(SafetyState::Fault, f.safety.state());
  TEST_ASSERT_EQUAL(1, f.can.stats().txErrors);
  FuzzFixture rebooted;
  rebooted.tick(4000);
  TEST_ASSERT_EQUAL(SafetyState::Safe, rebooted.safety.state());
  TEST_ASSERT_FALSE(rebooted.experiment.active());
  TEST_ASSERT_EQUAL(0, rebooted.experiment.fuzzRun().generated);
  TEST_ASSERT_EQUAL(2, CAN.writeCalls);
}

void test_fuzz_async_error_cancels_before_generation() {
  FuzzFixture f;
  f.command("arm\nfuzz start zero 0 2 1000\n");
  CAN.error = true;
  CAN.errorCode = 42;
  f.tick(0);
  CAN.error = false;
  f.tick(1000);
  TEST_ASSERT_EQUAL(0, CAN.writeCalls);
  TEST_ASSERT_EQUAL(0, f.experiment.fuzzRun().generated);
  TEST_ASSERT_EQUAL(1, f.experiment.stats().faulted);
  TEST_ASSERT_EQUAL(1, f.can.stats().controllerErrors);
  TEST_ASSERT_EQUAL(42, f.can.lastControllerError());
  TEST_ASSERT_EQUAL(SafetyState::Fault, f.safety.state());
}

void test_fuzz_parser_rejects_invalid_input_without_side_effects() {
  FuzzFixture f;
  f.command("arm\n");
  for (const char* text : {"fuzz",
                           "fuzz start",
                           "fuzz stop random 1 1 1000",
                           "fuzz start unknown 1 1 1000",
                           "fuzz start RANDOM 1 1 1000",
                           "fuzz start random -1 1 1000",
                           "fuzz start random +1 1 1000",
                           "fuzz start random 0x10 1 1000",
                           "fuzz start random 1 x 1000",
                           "fuzz start random 1 1 x",
                           "fuzz start random 4294967296 1 1000",
                           "fuzz start random 1 4294967296 1000",
                           "fuzz start random 1 1 4294967296",
                           "fuzz start random 1 0 1000",
                           "fuzz start random 1 1 999",
                           "fuzz start random 1 1 300001",
                           "fuzz start random 1 301 1000",
                           "fuzz start random 1 4294967295 1000",
                           "fuzz start random 1 1 0",
                           "fuzz start random 1 1 1000 extra",
                           "fuzz start random 1 1"}) {
    f.serial.output.clear();
    f.command(std::string(text) + "\n");
    TEST_ASSERT_NOT_EQUAL(std::string::npos, f.serial.output.find("ERROR:"));
    TEST_ASSERT_FALSE(f.experiment.active());
    TEST_ASSERT_EQUAL(SafetyState::Armed, f.safety.state());
    TEST_ASSERT_EQUAL(0, f.experiment.fuzzRun().generated);
  }
  TEST_ASSERT_EQUAL(0, CAN.writeCalls);
  TEST_ASSERT_FALSE(f.experiment.startFuzz(0, static_cast<FuzzStrategy>(255), 1, 1, 1000));
  f.command("\tfuzz\vstart\frandom 4294967295 1 300000\r\n");
  TEST_ASSERT_TRUE(f.experiment.active());
  f.command("fuzz start zero 0 1 1000\nstart\n");
  TEST_ASSERT_EQUAL(UINT32_MAX, f.experiment.fuzzRun().seed);
  TEST_ASSERT_EQUAL(1, f.experiment.stats().started);
}

void test_fuzz_wraparound_delay_and_timeout() {
  FuzzFixture f;
  f.command("arm\n");
  uint32_t start = UINT32_MAX - 500;
  TEST_ASSERT_TRUE(f.experiment.startFuzz(start, FuzzStrategy::Boundary, 1, 5, 1000));
  f.tick(start);
  f.tick(start + 3000U);
  f.tick(start + 3000U);
  TEST_ASSERT_EQUAL(2, CAN.writeCalls);
  TEST_ASSERT_EQUAL(2000, f.experiment.remainingMs(start + 3000U));
  f.tick(start + 5000U);
  TEST_ASSERT_EQUAL(2, CAN.writeCalls);
  TEST_ASSERT_EQUAL(1, f.experiment.stats().stopped);
  TEST_ASSERT_EQUAL(0, f.experiment.stats().completed);
  TEST_ASSERT_EQUAL(SafetyState::Armed, f.safety.state());
}

void test_fuzz_status_and_logging_backpressure() {
  FuzzFixture f;
  f.command("arm\nfuzz start random 1 2 1000\n");
  f.tick(0);
  f.serial.output.clear();
  f.command("status\n");
  for (const char* field :
       {"FUZZ_STRATEGY: random", "FUZZ_SEED: 1", "FUZZ_REQUESTED: 2", "FUZZ_INTERVAL_MS: 1000",
        "FUZZ_GENERATED: 1", "FUZZ_TX_ACCEPTED: 1", "FUZZ_LAST_INDEX: 0",
        "id=0x123 format=STD dlc=8", "data=2101C54FD1D01AB2"}) {
    TEST_ASSERT_NOT_EQUAL_MESSAGE(std::string::npos, f.serial.output.find(field), field);
  }
  f.serial.writable = false;
  for (unsigned i = 0; i < 10; ++i) {
    f.command("status\n");
  }
  f.command("stop\n");
  f.tick(1000);
  TEST_ASSERT_FALSE(f.experiment.active());
  TEST_ASSERT_EQUAL(1, CAN.writeCalls);
  f.serial.writable = true;
  size_t before = f.serial.output.size();
  f.logger.poll();
  TEST_ASSERT_LESS_OR_EQUAL(kLogBytesPerLoop, f.serial.output.size() - before);
}
}  // namespace

void runFuzzTests() {
  RUN_TEST(test_random_vectors_and_repeatability);
  RUN_TEST(test_seed_zero_and_engine_restart);
  RUN_TEST(test_constant_and_boundary_sequences);
  RUN_TEST(test_walking_bit_complete_cycle);
  RUN_TEST(test_bitflip_base_copy_and_exact_bits);
  RUN_TEST(test_fuzz_explicit_arm_start_and_completion);
  RUN_TEST(test_repeated_fuzz_experiments_reproduce_all_strategies);
  RUN_TEST(test_rate_limit_retains_generated_frame);
  RUN_TEST(test_fuzz_stop_disarm_reset_and_known_demo_preserved);
  RUN_TEST(test_fuzz_write_fault_latched_recovery_and_fresh_boot);
  RUN_TEST(test_fuzz_async_error_cancels_before_generation);
  RUN_TEST(test_fuzz_parser_rejects_invalid_input_without_side_effects);
  RUN_TEST(test_fuzz_wraparound_delay_and_timeout);
  RUN_TEST(test_fuzz_status_and_logging_backpressure);
}
