# Software Architecture

The project began with native CAN bring-up and safe experiment control (Milestone 1).
Milestone 2A added deterministic payload generation behind the existing experiment
layer. The interactive console builds on those components without changing the
driver, mutation engine or experiment lifecycle.

```text
PC terminal -> CommandInterface -> ExperimentManager -> FuzzEngine (payload only)
                       |                 |
                       v                 v
                     Logger          CanDriver -> UNO R4 native CAN
                       |                 |
                  Bounded queue     SafetyManager (TX permission / FAULT)
```

## Components

- `CanDriver`: wraps `Arduino_CAN`, validates standard transmit frames and bitrate,
  preserves standard/extended RX format, and enforces safety and rate limits.
- `SafetyManager`: owns BOOT, SAFE, ARMED, RUNNING, and FAULT transitions.
- `CommandInterface`: edits and parses bounded USB serial input without allocation.
  It handles echo, backspace, CR/LF/CRLF and dispatch, using Logger for all output.
- `ExperimentManager`: runs the original known-frame demo or bounded fuzz run;
  schedules at most one eligible attempt per update and accounts for outcomes.
- `FuzzEngine`: pure C++ payload generation with explicit xorshift32/seed semantics.
  It owns a base copy and never accesses clocks, safety state or CAN hardware.
- `Logger`: queues status, errors, observed frames and console display fragments.

`CanDriver` holds a reference to the
`SafetyManager` so all application sends enforce the state gate, and propagates
latched controller errors into FAULT. `ExperimentManager` treats loss of transmit
permission as cancellation. `BufferedLogOutput` adds a fixed-size whole-line
queue inside Logger; each cooperative loop limits serial input, RX and log work.
Application code never bypasses the driver by sending directly from the parser
or mutation engine. Successful controller acceptance is not proof of delivery.

## Cooperative loop and console output

The loop polls CAN health and reconciles FAULT cancellation before commands,
processes at most 32 serial bytes and one submitted line, updates the experiment,
receives at most four CAN frames and drains at most 32 output bytes. There are no
catch-up bursts. STOP, FAULT and bounded scheduling do not abort already queued
controller frames; see the [developer guide](developer-guide.md) for UART limits.

The command buffer remains 80 bytes including its terminator. The console's
prompt, echo, erase and newline operations commit short fragments to the existing
2048-byte logger queue, without waiting for a whole input line. Ordinary log
records remain whole-line atomic; console fragments are whole-fragment atomic.
Full queues count drops rather than blocking command execution. The parser and
logger retain fixed storage and do not write directly to Serial during parsing.
See [protocol details](protocol.md) for display/backpressure behavior.

## Repository layout

| Path | Responsibility |
| --- | --- |
| `include/` | Public frame, safety, driver, experiment, mutation, command and logger contracts |
| `src/main.cpp` | Fuzzer setup and cooperative loop |
| `src/can_driver.cpp`, `src/safety.cpp` | Guarded hardware boundary and state transitions |
| `src/experiment.cpp`, `src/fuzz_engine.cpp` | Scheduling/outcomes and pure payload generation |
| `src/command_interface.cpp`, `src/logger.cpp` | Interactive serial control and bounded output |
| `src/sniffer_main.cpp`, `examples/sniffer.ino` | Independent observer build entry and sketch |
| `test/fakes/`, `test/test_control/` | Host serial/CAN boundary doubles and Unity regressions |
| `tools/host_compiler.py` | Existing portable host compiler selection |
| `platformio.ini` | Pinned WiFi, Minima sniffer and native test environments |
| `hardware/`, `docs/` | Physical bench instructions, contracts and validation evidence |

## Separation and future direction

The current sniffer is an independent two-node bench observer, not a simulated
target ECU. Payload mutations operate on fixed standard ID `0x123` and DLC 8;
the engine-to-driver separation leaves room for later work without a generic
transport layer. Future sensors, target controllers, physical outputs, Wi-Fi UI,
replay and timing experiments are not implemented.

The earlier README proposed a separate strategies directory and a host controller
with SocketCAN plus USB-serial paths. Those remain possible design choices, not
the present file structure. Their experiment/capture concepts are retained in
[experiment design](experiment-design.md). Current mutation algorithms and lifecycle
contracts are documented in [Milestone 2A](milestone-2a-deterministic-fuzzing.md).
