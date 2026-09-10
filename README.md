# CAN Chaos Fuzzer

> A CAN bus fuzzing and chaos-injection platform for embedded security, resilience, and adversarial verification testing.

CAN Chaos Fuzzer is an open-source embedded security research project for evaluating how CAN-based systems behave when normal assumptions about network traffic are intentionally violated.

Rather than simply generating random CAN frames, the project is being developed as a repeatable security test platform for manipulating:

- Message content
- CAN identifiers
- Message timing
- Message frequency
- Message ordering
- Arbitration priority
- Bus load
- Application state transitions

The central question behind the project is:

> **What happens to an embedded system when its CAN network stops behaving the way its developers assumed it would?**

The project combines traditional fuzz testing with concepts from adversarial testing, fault injection, resilience testing, and chaos engineering to evaluate not only whether a system fails, but **how it fails, how it responds, and how it recovers**.

---

## Project Status

**Milestone 1 — COMPLETE** (`v0.1.0-can-foundation`, hardware validated).

**Milestone 2 — IN PROGRESS: Milestone 2A — Deterministic Known-ID Payload Fuzzing.**

The six initial payload strategies and `fuzz start` command are implemented with
host regression coverage; fuzz hardware validation is pending. See the
[Milestone 2A specification](docs/milestone-2a-deterministic-fuzzing.md) for usage,
deterministic ordering, limits and validation procedure.

Milestones 1A and 1B established the software, control, and safety foundation.

### Software Foundation

- [x] PlatformIO project structure
- [x] Pinned Renesas RA platform and toolchain
- [x] Arduino UNO R4 WiFi firmware target
- [x] Separate CAN sniffer firmware target
- [x] Modular firmware architecture
- [x] Native RA4M1 CAN driver abstraction
- [x] SAFE / ARMED / RUNNING / FAULT state model
- [x] Safety enforcement at the CAN transmit boundary
- [x] Serial command interface
- [x] Bounded serial processing
- [x] Bounded CAN receive processing
- [x] Buffered and bounded logging
- [x] Runtime CAN fault propagation
- [x] Experiment cancellation on fault
- [x] CAN transmission rate limiting
- [x] Standard/extended RX frame preservation
- [x] Host-side automated testing
- [x] 18 host tests passing

### Hardware Acceptance

- [x] Flash firmware to physical UNO R4 WiFi
- [x] Validate SAFE state on hardware boot
- [x] Validate physical CAN initialization
- [x] Validate known-frame CAN transmission
- [x] Validate known-frame CAN reception
- [x] Validate independent sniffer observation
- [x] Validate STOP behavior on hardware
- [x] Validate CAN fault behavior
- [x] Validate reset/reconnect safety behavior

Milestone 1C is complete; see the [bench validation record](docs/milestone-1c-validation.md) for evidence and limitations. Quantitative STOP latency remains a follow-up measurement.

Milestone 2A adds deterministic payload strategies while preserving the Milestone 1 known-frame demo.

---

## Project Goals

The long-term goal is to create a small, reusable **CAN security test instrument** capable of conducting controlled and repeatable experiments against embedded systems.

The intended progression is:

```text
CAN Traffic Generator
        |
        v
CAN Fuzzer
        |
        v
CAN Chaos Injector
        |
        v
Instrumented Test Harness
        |
        v
Repeatable Embedded Security Test Platform
```

The project is specifically interested in testing assumptions involving:

```text
Content
Timing
Ordering
Frequency
Priority
State
Recovery
```

A useful security experiment should answer more than:

> Did the target crash?

It should help determine:

```text
What input caused the behavior?

Was the behavior deterministic?

What state did the target enter?

Was CAN communication degraded?

Did the target detect the condition?

Did the target recover automatically?

How long did recovery take?

Was a reset required?

Could the behavior be reproduced?
```

---

## Hardware

### Primary Fuzzer

The primary fuzzing node is an:

**Arduino UNO R4 WiFi**

The UNO R4 WiFi contains a Renesas RA4M1 microcontroller with a native CAN controller.

The project uses the RA4M1 CAN peripheral rather than an external MCP2515 CAN controller.

An external CAN transceiver provides the physical bus interface.

Current target transceiver:

**SN65HVD230**

### Hardware Architecture

```text
+---------------------------+
| Arduino UNO R4 WiFi       |
|                           |
| Renesas RA4M1             |
|                           |
| Native CAN Controller     |
+------------+--------------+
             |
         CANTX / CANRX
             |
+------------v--------------+
| SN65HVD230                |
| CAN Transceiver           |
+------------+--------------+
             |
        CANH / CANL
             |
============= CAN BUS =============
             |
     +-------+-------+
     |               |
     v               v
Target ECU       CAN Monitor
                 / Sniffer
```

Hardware development and testing are performed on an isolated CAN bench.

See the `hardware/` directory for wiring and bill-of-material information.

---

## Development Environment

Firmware development uses:

- Visual Studio Code
- PlatformIO
- Arduino framework
- C/C++
- Git
- GitHub

Host-side development uses or is expected to use:

- Python 3
- Native host testing
- Linux
- SocketCAN

The project intentionally uses a modular firmware architecture instead of a single Arduino `.ino` sketch.

---

## Repository Structure

```text
CAN-Chaos-Fuzzer/
|
├── include/
|   ├── can_driver.h
|   ├── command_interface.h
|   ├── experiment.h
|   ├── logger.h
|   └── safety.h
|
├── src/
|   ├── main.cpp
|   ├── sniffer_main.cpp
|   ├── can_driver.cpp
|   ├── command_interface.cpp
|   ├── experiment.cpp
|   ├── logger.cpp
|   └── safety.cpp
|
├── test/
|   ├── fakes/
|   └── test_control/
|
├── tools/
|   └── host_compiler.py
|
├── docs/
├── examples/
├── hardware/
|
├── platformio.ini
├── CHANGELOG.md
├── README.md
└── LICENSE
```

As development progresses, fuzzing strategies will be separated into dedicated modules.

Planned structure:

```text
src/
└── strategies/
    ├── random_fuzzer.cpp
    ├── payload_mutator.cpp
    ├── timing_fuzzer.cpp
    ├── replay_fuzzer.cpp
    ├── arbitration_fuzzer.cpp
    └── load_generator.cpp
```

These strategy modules are part of the planned architecture and are **not yet implemented**.

---

## Building

### Requirements

Install:

1. Visual Studio Code
2. PlatformIO

Clone the repository:

```bash
git clone https://github.com/gingerSou1/CAN-Chaos-Fuzzer.git
cd CAN-Chaos-Fuzzer
```

Build the primary firmware:

```bash
pio run -e uno_r4_wifi
```

Build the primary firmware and sniffer:

```bash
pio run -e uno_r4_wifi -e sniffer
```

Run the host-side test suite:

```bash
pio test -e host -v
```

Current software validation result:

```text
18 passed
0 failed
```

Upload the primary firmware:

```bash
pio run -e uno_r4_wifi -t upload
```

Upload the sniffer firmware:

```bash
pio run -e sniffer -t upload
```

Open the serial monitor:

```bash
pio device monitor
```

Default serial configuration:

```text
115200 baud
```

---

## Current Software Architecture

The current firmware foundation separates hardware access, control, safety, experiments, commands, and logging.

```text
             +-------------------+
             | Command Interface |
             +---------+---------+
                       |
                       v
             +-------------------+
             | Experiment Manager|
             +---------+---------+
                       |
             +---------+---------+
             |                   |
             v                   v
      +--------------+    +--------------+
      |Safety Manager|    |    Logger    |
      +------+-------+    +--------------+
             |
             v
      +--------------+
      |  CAN Driver  |
      +------+-------+
             |
             v
      +--------------+
      | RA4M1 CAN HW |
      +--------------+
```

Safety is enforced at the CAN driver boundary so higher-level components cannot transmit simply by bypassing experiment-state checks.

---

## Target Architecture

After the CAN foundation has been validated on hardware, fuzzing strategies will be introduced behind the experiment layer.

```text
                 +-------------------+
                 | Command Interface |
                 +---------+---------+
                           |
                           v
                 +-------------------+
                 | Experiment Manager|
                 +---------+---------+
                           |
               +-----------+-----------+
               |                       |
               v                       v
       +---------------+       +---------------+
       | Safety Manager|       |  Fuzz Engine  |
       +-------+-------+       +-------+-------+
               |                       |
               |                       v
               |              +----------------+
               |              | Fuzz Strategies|
               |              +-------+--------+
               |                      |
               +----------+-----------+
                          |
                          v
                  +---------------+
                  |  CAN Driver   |
                  +-------+-------+
                          |
                          v
                  +---------------+
                  | RA4M1 CAN HW  |
                  +---------------+
```

The fuzz engine and fuzz strategies shown above represent **planned functionality**.

---

## Safety Model

CAN fuzzing can rapidly disrupt a CAN network.

For this reason, transmission safety is treated as a core architectural requirement rather than an optional feature.

The device must always boot into:

```text
SAFE
```

The control model is:

```text
             BOOT
               |
               v
             SAFE
               |
              arm
               |
               v
            ARMED
               |
             start
               |
               v
           RUNNING
               |
              stop
               |
               v
            ARMED
               |
            disarm
               |
               v
             SAFE

Runtime CAN fault
       |
       v
     FAULT
```

A test must never begin automatically after:

- Power-on
- Reset
- Firmware upload
- Serial reconnect

A `start` command while the system is `SAFE` must be rejected.

Example:

```text
> start

ERROR: DEVICE NOT ARMED
```

Active experiments support cancellation, and runtime CAN errors propagate into a latched FAULT condition.

Safety checks are also enforced at the CAN transmit boundary.

---

## Default CAN Configuration

Initial development defaults include:

```text
CAN bitrate:       500 kbps
CAN format:        Classical CAN
TX identifiers:    11-bit standard
Boot state:        SAFE
Serial:            115200 baud
Automatic fuzzing: Disabled
```

Unsupported CAN bitrates are rejected.

Extended-frame format is preserved during CAN reception, while the current transmit implementation intentionally remains limited to standard identifiers.

---

## Command Interface

The current control interface uses USB serial.

The interface provides commands for device state and experiment control.

Representative interaction:

```text
> status

STATE: SAFE

> start

ERROR: DEVICE NOT ARMED

> arm

STATE: ARMED
```

Command parsing is bounded and rejects:

- Oversized commands
- Numeric overflow
- Unsupported arguments
- Malformed input
- Invalid state transitions

Serial processing is limited per main-loop iteration to prevent excessive command input from monopolizing firmware execution.

---

## Bounded Runtime Processing

A fuzzing platform must remain controllable even when traffic and logging activity increase.

The firmware therefore bounds several operations per main-loop iteration.

This includes:

- Serial input processing
- CAN reception
- Log output

Logging uses a bounded buffer and tracks whole-line drops when output cannot keep up.

This design is intended to prevent logging or input processing from starving safety-critical control behavior.

---

## CAN Transmission Safety

CAN transmission is protected at the driver boundary.

Current protections include:

- SAFE-state rejection
- State validation
- Standard-ID validation
- DLC validation
- Supported bitrate validation
- Transmission rate limiting
- Runtime CAN error handling
- Fault propagation

The current development rate limit enforces at least one second between accepted transmissions.

The rate limit is preserved across:

- STOP / START
- Counter reset
- Experiment restart

This intentionally conservative rate is used during foundation development and hardware bring-up.

---

## Software Validation

The host-side test suite currently contains **18 passing tests**.

Coverage includes:

- Safety state transitions
- Invalid state transitions
- Command parsing
- Malformed input rejection
- Numeric overflow
- Oversized commands
- Experiment cancellation
- Runtime faults
- CAN fault propagation
- Rate limiting
- Timer wraparound
- Counter behavior
- Logger backpressure
- Main-loop processing limits
- CAN frame handling

Validation performed:

```text
pio run -e uno_r4_wifi -t clean
PASS

pio run -e uno_r4_wifi -e sniffer
PASS

pio test -e host -v
18 passed, 0 failed
```

Passing host tests do **not** replace physical CAN hardware validation.

---

## Milestone 1C — Hardware Acceptance

The next development phase validates the software foundation against real CAN hardware.

Planned bench:

```text
        CAN CHAOS FUZZER

       Arduino UNO R4
              |
       SN65HVD230
              |
              |
            CANH
            CANL
              |
      =================
         ISOLATED BUS
      =================
              |
        +-----+-----+
        |           |
        v           v
     Sniffer     Test Node
```

Required hardware acceptance sequence:

```text
Firmware builds
       |
       v
Firmware uploads
       |
       v
Device boots SAFE
       |
       v
START while SAFE rejected
       |
       v
ARM accepted
       |
       v
Known CAN frame transmitted
       |
       v
Independent sniffer observes frame
       |
       v
ID / DLC / payload verified
       |
       v
Known CAN frame received
       |
       v
STOP behavior verified
       |
       v
CAN fault behavior verified
       |
       v
Reset / reconnect returns SAFE
```

Compilation alone does **not** constitute completion of Milestone 1.

---

## Known Hardware Validation Limitations

Several behaviors cannot be fully established through host-side tests.

These include:

- Physical CAN electrical compatibility
- Actual CAN TX/RX behavior
- Physical bus fault behavior
- Hardware CAN error callbacks
- Reboot behavior
- USB reconnect behavior
- Absolute STOP latency

Software cancellation prevents subsequent application-level writes after STOP or FAULT.

However, a CAN frame already accepted by the hardware controller cannot be retracted.

The currently pinned serial/UART implementation may also wait synchronously during some operations, so absolute STOP latency must be measured on hardware.

---

## Planned Fuzzing Capabilities

Fuzzing strategies are intentionally deferred until completion of hardware acceptance.

### Known-ID Payload Mutation

The first planned fuzzing primitive will mutate the payload of a known CAN identifier.

Planned mutations include:

```text
Random payload
Single-bit flip
Single-byte mutation
All zero
All FF
Increment
Decrement
Boundary values
Walking bit
```

Example:

```text
Original:

120#003C000000000000

Mutated:

120#FF3C000000000000
120#003D000000000000
120#0000000000000000
120#FFFFFFFFFFFFFFFF
```

---

## Deterministic Testing

Fuzzing should be reproducible.

Future experiments will therefore support deterministic random seeds.

Conceptually:

```text
Profile + Seed
      |
      v
Mutation Sequence
      |
      v
CAN Frames
      |
      v
Target Behavior
      |
      v
Evidence / Logs
```

The same profile and seed should generate the same mutation sequence.

---

## Random Frame Fuzzing

Future random fuzzing will support controlled randomization of:

- CAN IDs
- DLC values
- Payloads
- Transmission intervals

Random generation will remain subject to safety limits and experiment configuration.

---

## Timing Chaos

Future timing experiments may manipulate:

```text
Message period
Jitter
Delay
Burst timing
Message acceleration
Random intervals
```

Example:

```text
Expected:

100 ms
100 ms
100 ms
100 ms
100 ms

Injected:

100 ms
72 ms
143 ms
21 ms
286 ms
```

The objective is to evaluate how a target behaves when application-level timing assumptions are violated.

---

## Replay Testing

Future replay support will record and reproduce CAN traffic.

Captured frames may include:

```text
Relative timestamp
CAN ID
Frame format
DLC
Payload
```

Planned replay modes include:

```text
Single frame
Frame sequence
Timing-preserved sequence
Looped sequence
Accelerated sequence
Slowed sequence
```

Replay experiments can help evaluate whether systems improperly accept stale or repeated messages.

---

## Arbitration Testing

CAN arbitration gives numerically lower identifiers higher bus priority.

Future experiments will evaluate the effects of controlled high-priority traffic on legitimate network communication.

These tests will remain explicitly armed, rate limited, and constrained by experiment duration.

---

## Bus Load Testing

Future releases may support controlled CAN load generation.

Potential experiment targets include:

```text
10%
25%
50%
75%
90%
```

The objective is to identify system degradation and recovery thresholds rather than simply transmit at the maximum possible rate.

Measurements may include:

```text
Frames transmitted
Transmission failures
Message latency
Message loss
CAN controller errors
Bus-off events
Target behavior
Recovery time
```

---

## Target ECU Simulator

A later milestone will introduce a dedicated target ECU simulator.

The simulator will provide deterministic behavior so experiments can be reproduced without requiring production equipment.

Example message map:

| CAN ID | Function | Rate |
| --- | --- | ---: |
| `0x100` | Controller State | 500 ms |
| `0x120` | RPM | 50 ms |
| `0x130` | Temperature | 250 ms |
| `0x200` | Actuator Command | Event |
| `0x210` | Actuator Status | 100 ms |
| `0x700` | Heartbeat | 100 ms |

Example state model:

```text
OFF
 |
 v
INITIALIZING
 |
 v
READY
 |
 v
ACTIVE
 |
 +------> FAULT
```

Target telemetry may eventually expose:

- Heartbeat
- Current state
- Invalid-frame count
- Error count
- Reset count
- Last valid command
- Watchdog events
- Unexpected-state count

---

## Experiment Model

Future experiments are intended to follow a repeatable lifecycle:

```text
Initialize
    |
    v
Load Configuration
    |
    v
Capture Baseline
    |
    v
ARM
    |
    v
Inject
    |
    v
Observe
    |
    v
Stop Injection
    |
    v
Recovery Window
    |
    v
Store Results
```

Stopping hostile traffic does not necessarily end an experiment.

**Recovery is part of the measurement.**

---

## Experiment Profiles

Experiments will eventually be configuration driven.

Example:

```json
{
  "name": "RPM Payload Mutation",
  "mode": "payload_mutation",
  "target_id": "0x120",
  "mutation": "random",
  "interval_ms": 100,
  "duration_seconds": 30,
  "recovery_seconds": 30,
  "seed": 1337
}
```

Profiles will be validated before execution.

Invalid or unsafe configurations should fail closed rather than silently execute.

---

## Observability

The project is intended to measure target behavior rather than merely generate hostile traffic.

Relevant observations may include:

```text
Heartbeat loss
Message latency
Message loss
Invalid state transitions
Application faults
CAN errors
Bus-off
Watchdog reset
Unexpected reset
Automatic recovery
Manual recovery
```

The relationship of interest is:

```text
Injected Input
      |
      v
CAN Behavior
      |
      v
Application Behavior
      |
      v
System Impact
      |
      v
Recovery
```

---

## Recovery Testing

A system temporarily entering a degraded state is different from a system remaining permanently unavailable.

Recovery should therefore be explicitly measured.

Potential metrics include:

```text
Time to heartbeat recovery
Time to CAN recovery
Time to READY
Time to ACTIVE
Number of lost messages
Number of resets
Manual intervention required
```

Example severity model:

| Level | Result |
| ---: | --- |
| 0 | No observable impact |
| 1 | Temporary degradation |
| 2 | Automatic recovery required |
| 3 | Component reset required |
| 4 | Manual intervention required |
| 5 | Persistent unsafe or unexpected state |

---

## Host-Side Tooling

A future Python host application will provide experiment orchestration.

Desired interface:

```bash
canchaos status

canchaos profiles

canchaos capture --duration 60

canchaos run profiles/rpm_mutation.json

canchaos analyze results/CAN-2026-09-06-0042/
```

Initial host-to-device communication will use USB serial.

Future interfaces may include:

```text
SocketCAN
TCP/IP
WiFi
REST API
```

Network-facing control interfaces are **not currently implemented**.

Any future remote-control capability will require additional security analysis and controls before implementation.

---

## Linux / SocketCAN Integration

Linux may eventually provide independent CAN monitoring and experiment capture.

Potential tooling includes:

```text
candump
cansend
cangen
canplayer
canbusload
```

Long-term architecture:

```text
                  Linux Test Controller
                         |
              +----------+----------+
              |                     |
          SocketCAN              USB Serial
              |                     |
              v                     v
        CAN Monitor          CAN Chaos Fuzzer
              |                     |
              +----------+----------+
                         |
====================== CAN ======================
                         |
                         v
                    Target ECU
```

This provides separation between:

- Attack generation
- Monitoring
- Target behavior

---

## Security Considerations

The repository is intended to remain safe for public development.

Current project practices include:

- No hard-coded credentials
- No API keys required by the firmware
- No network-facing API in the current implementation
- No default passwords
- No WiFi credentials stored in firmware
- Local USB serial control
- Explicit SAFE boot state
- Explicit ARM requirement
- Driver-level transmit safety enforcement
- Bounded command processing
- Bounded logging
- Runtime fault propagation
- Repository exclusions for local secrets and generated artifacts

Sensitive local files such as environment files, credentials, private keys, logs, captures, and experiment output should not be committed.

Future network-facing interfaces will require their own threat model before being enabled.

---

## Threat Model

The project assumes a laboratory scenario in which a node capable of transmitting onto the CAN network is compromised, malfunctioning, malicious, or intentionally acting as an adversarial test device.

The project is primarily interested in **post-bus-access resilience**.

Questions include:

```text
What trust does the system place in CAN IDs?

What assumptions exist about payload values?

What assumptions exist about timing?

What happens when valid messages arrive at invalid times?

What happens when message priority is abused?

Can stale commands be replayed?

Can traffic cause unexpected state transitions?

Can malformed traffic degrade availability?

Can the system detect the condition?

Can the system recover?
```

See:

```text
docs/threat-model.md
```

---

## Aerospace and Embedded Context

CAN is used across automotive, industrial, robotics, aerospace, and other embedded systems.

This project is not intended to model one specific vehicle, aircraft, or production platform.

The methodology is particularly relevant to systems where CAN communication participates in:

- Distributed control
- Sensor communication
- Actuator control
- Subsystem coordination
- Health/status reporting

The repository includes additional documentation exploring relationships to aerospace and embedded CAN environments.

See:

```text
docs/aerospace-mapping.md
```

---

## Roadmap

### Milestone 1A — Software Foundation

- [x] PlatformIO environment
- [x] Pinned toolchain
- [x] Modular firmware
- [x] Native RA4M1 CAN abstraction
- [x] Separate sniffer target

### Milestone 1B — Control and Safety Validation

- [x] SAFE / ARMED / RUNNING / FAULT model
- [x] Serial command validation
- [x] Driver-level safety enforcement
- [x] Runtime fault propagation
- [x] Bounded processing
- [x] Buffered logging
- [x] Rate limiting
- [x] Host test infrastructure
- [x] 18 host tests passing

### Milestone 1C — Hardware Acceptance

- [x] Physical CAN bench
- [x] Firmware upload
- [x] SAFE-on-boot verification
- [x] Known-frame TX
- [x] Known-frame RX
- [x] Independent sniffer validation
- [x] Hardware fault validation
- [x] Reset/reconnect testing

### Follow-up measurements

- [ ] Measure quantitative STOP latency on hardware; it has not been measured. This is a future measurement outside the completed Milestone 1C acceptance scope.

### Milestone 2 — Core Fuzzer

- [ ] Known-ID payload mutation
- [ ] Mutation strategies
- [ ] Deterministic seeds
- [ ] Random frame fuzzing
- [ ] Experiment configuration
- [ ] Experiment duration
- [ ] Per-frame evidence logging

### Milestone 3 — Chaos Engine

- [ ] Timing jitter
- [ ] Bursts
- [ ] Replay
- [ ] Arbitration experiments
- [ ] Controlled load generation

### Milestone 4 — Target ECU

- [ ] Target simulator
- [ ] CAN message map
- [ ] Application state machine
- [ ] Health telemetry
- [ ] Fault behavior
- [ ] Recovery behavior

### Milestone 5 — Instrumentation

- [ ] Experiment IDs
- [ ] Structured logging
- [ ] Baseline capture
- [ ] Recovery measurement
- [ ] Result summaries
- [ ] Baseline comparison

### Milestone 6 — Host Controller

- [ ] Python CLI
- [ ] Experiment profiles
- [ ] Automated execution
- [ ] Capture management
- [ ] Result analysis

### v1.0 — CAN Security Test Platform

- [ ] Reproducible experiments
- [ ] Defined test cases
- [ ] Independent monitoring
- [ ] Evidence capture
- [ ] Resilience metrics
- [ ] Recovery analysis
- [ ] Complete documentation
- [ ] Reproducible build

---

## Future Ideas

Potential future expansion includes:

```text
CAN FD
DBC parsing
ISO-TP
UDS
Protocol-aware fuzzing
Coverage-guided fuzzing
SocketCAN orchestration
Hardware-in-the-loop testing
Multiple target ECUs
Multiple fuzzing nodes
Automated anomaly detection
Web dashboard
CI-connected hardware testing
```

These features are intentionally outside the initial development scope.

The immediate priority is building a **reliable, observable, safe, and reproducible CAN security testing foundation**.

---

## Responsible Use

CAN Chaos Fuzzer is intended for:

- Security research
- Embedded systems testing
- Hardware-in-the-loop laboratories
- Authorized penetration testing
- Security verification
- Resilience testing
- Education

CAN fuzzing can disrupt communications, trigger unexpected behavior, or render connected systems unavailable.

**Do not connect this tool to vehicles, aircraft, industrial equipment, medical devices, production networks, or other operational systems without explicit authorization and appropriate safety controls.**

Development and testing should be performed on isolated laboratory CAN networks using hardware you own or are explicitly authorized to test.

---

## License

See `LICENSE` for project licensing information.

---

## Project Philosophy

Traditional verification often asks:

> **Does the system behave correctly when given the expected input?**

Security verification adds another question:

> **What happens when we intentionally violate those expectations?**

CAN Chaos Fuzzer is being built to explore that boundary.

**Break the assumptions. Observe the behavior. Measure the recovery. Reproduce the result.**
