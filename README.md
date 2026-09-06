# CAN Chaos Fuzzer

Contributor guidance: [coding and release conventions](CONTRIBUTING.md),
[security policy](SECURITY.md), and [build/validation guide](docs/developer-guide.md).

> A CAN bus fuzzing and chaos-injection platform for embedded security, resilience, and adversarial verification testing.

CAN Chaos Fuzzer is an open-source embedded security research project designed to evaluate how CAN-based systems behave when normal assumptions about network traffic are intentionally violated.

Rather than simply generating random CAN frames, the project is intended to provide a repeatable test platform for manipulating:

* Message content
* CAN identifiers
* Message timing
* Message frequency
* Message ordering
* Arbitration priority
* Bus load
* Application state transitions

The central question behind the project is:

> **What happens to an embedded system when its CAN network stops behaving the way its developers assumed it would?**

The project combines traditional fuzz testing with concepts from adversarial testing, fault injection, and chaos engineering to evaluate not only whether a system fails, but **how it fails and how it recovers**.

---

# Project Status

> **Current Phase: CAN Foundation / Milestone 1**

The project is under active development.

The current implementation is focused on establishing a reliable and safe CAN testing foundation before implementing the full fuzzing engine.

Current development priorities:

* [x] Migration from Arduino sketch structure to PlatformIO
* [x] Modular firmware architecture
* [ ] Native RA4M1 CAN controller bring-up
* [ ] CAN transmit validation
* [ ] CAN receive validation
* [ ] SAFE / ARMED / RUNNING state validation
* [ ] Serial command interface validation
* [ ] Experiment lifecycle
* [ ] Structured logging
* [ ] Hardware acceptance testing

Fuzzing and chaos-injection strategies will be implemented after the CAN foundation has been validated on hardware.

---

# Goals

The long-term goal is to create a small, reusable **CAN security test instrument** capable of conducting repeatable experiments against embedded systems.

The intended progression is:

```text
CAN Traffic Generator
        ↓
CAN Fuzzer
        ↓
CAN Chaos Injector
        ↓
Instrumented Test Harness
        ↓
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

A successful experiment should answer more than:

> Did the target crash?

It should help answer:

```text
What input caused the behavior?

Was the failure deterministic?

What state did the target enter?

Was CAN communication degraded?

Did the target detect the condition?

Did the target recover automatically?

How long did recovery take?

Was a reset required?

Could the behavior be reproduced?
```

---

# Hardware

## Fuzzer

The primary fuzzing node is:

**Arduino UNO R4 WiFi**

The UNO R4 WiFi contains a Renesas RA4M1 microcontroller with a native CAN controller.

The project uses the RA4M1 CAN peripheral rather than an external MCP2515 CAN controller.

An external CAN transceiver provides the physical CAN interface.

Current target transceiver:

**SN65HVD230**

Architecture:

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
============ CAN BUS ============
```

---

# Development Environment

The firmware is developed using:

* Visual Studio Code
* PlatformIO
* Arduino framework
* C/C++
* Git
* GitHub

Host-side tooling will primarily use:

* Python 3
* Linux
* SocketCAN

The project intentionally uses a modular firmware architecture instead of a single Arduino `.ino` sketch.

---

# Repository Structure

```text
CAN-Chaos-Fuzzer/
│
├── platformio.ini
├── README.md
├── LICENSE
├── CHANGELOG.md
│
├── include/
│   ├── can_driver.h
│   ├── command_interface.h
│   ├── experiment.h
│   ├── logger.h
│   └── safety.h
│
├── src/
│   ├── main.cpp
│   ├── can_driver.cpp
│   ├── command_interface.cpp
│   ├── experiment.cpp
│   ├── logger.cpp
│   └── safety.cpp
│
├── docs/
│   ├── architecture.md
│   ├── threat-model.md
│   ├── experiment-design.md
│   └── aerospace-mapping.md
│
├── examples/
│
├── hardware/
│   ├── BOM.md
│   └── wiring.md
│
├── tools/
│
└── tests/
```

As development progresses, fuzzing strategies will be separated into their own modules.

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

---

# Building

## Requirements

Install:

1. Visual Studio Code
2. PlatformIO

Clone the repository:

```bash
git clone https://github.com/gingerSou1/CAN-Chaos-Fuzzer.git
cd CAN-Chaos-Fuzzer
```

Build:

```bash
pio run
```

Upload to the Arduino UNO R4 WiFi:

```bash
pio run --target upload
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

# Software Architecture

The firmware is divided into independent functional components.

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
       +---------------+       +-------+-------+
                                       |
                                       v
                              +----------------+
                              | Fuzz Strategies|
                              +-------+--------+
                                      |
                                      v
                              +----------------+
                              |   CAN Driver   |
                              +-------+--------+
                                      |
                                      v
                              +----------------+
                              | RA4M1 CAN HW   |
                              +----------------+
```

The CAN driver is responsible for hardware interaction.

The experiment layer manages test execution.

The safety manager controls whether active transmission is permitted.

Future fuzzing strategies operate through the CAN abstraction rather than directly manipulating hardware.

This separation is intended to make the project easier to test, maintain, and extend.

---

# Safety Model

CAN fuzzing can rapidly disrupt a CAN network.

For this reason, transmission safety is treated as a core architectural requirement rather than an optional feature.

The device must always boot into:

```text
SAFE
```

The expected state machine is:

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
```

A test must never begin automatically after:

* Power-on
* Reset
* Firmware upload
* Serial reconnect

A `start` command while the system is `SAFE` must be rejected.

Example:

```text
> start

ERROR: DEVICE NOT ARMED
```

Active experiments must support immediate termination.

---

# Default CAN Configuration

Initial development defaults:

```text
CAN bitrate:       500 kbps
CAN format:        Classical CAN
Identifier:        11-bit standard
Boot state:        SAFE
Serial:            115200 baud
Automatic fuzzing: Disabled
```

These values may become configurable as development progresses.

---

# Command Interface

The initial control interface uses USB serial.

Planned command set:

```text
help
status
arm
disarm
start
stop
profile
stats
reset
```

Example:

```text
> status

CAN CHAOS FUZZER

STATE: SAFE
CAN: ONLINE
BITRATE: 500000
TX: 0
RX: 1242
ERRORS: 0
```

Arming:

```text
> arm

STATE: ARMED
```

Attempting to start without arming:

```text
> start

ERROR: DEVICE NOT ARMED
```

The serial protocol will eventually also provide the interface used by the Python host controller.

---

# Milestone 1 — CAN Foundation

The first milestone establishes the underlying CAN test platform.

Required functionality:

```text
PlatformIO environment
        ↓
RA4M1 CAN initialization
        ↓
CAN transmission
        ↓
CAN reception
        ↓
Safety state machine
        ↓
Serial control
        ↓
Experiment foundation
        ↓
Logging foundation
```

Compilation alone does **not** constitute completion.

Milestone 1 must be validated on an isolated CAN test bench.

---

# Milestone 1 Acceptance Test

The following sequence must succeed:

```text
PlatformIO build succeeds
        ↓
Firmware uploads to UNO R4
        ↓
UNO R4 boots SAFE
        ↓
"start" while SAFE is rejected
        ↓
User issues "arm"
        ↓
Device enters ARMED
        ↓
Known CAN frame is transmitted
        ↓
Independent sniffer observes frame
        ↓
ID / DLC / payload verified
        ↓
UNO R4 receives known CAN frame
        ↓
"stop" immediately stops transmission
        ↓
Device returns safely to SAFE
```

Only after this test succeeds should development proceed to active fuzzing.

---

# Planned Fuzzing Capabilities

## Random Frame Fuzzing

Generate randomized:

* CAN IDs
* DLC values
* Payloads
* Transmission intervals

Random fuzzing should support deterministic seeds so discovered behavior can be reproduced.

---

## Known-ID Mutation

Given a legitimate CAN identifier, mutate its payload while preserving the identifier.

Planned mutation strategies:

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

# Timing Chaos

Many embedded systems depend on message timing even when CAN itself does not enforce application-level timing semantics.

The platform will support manipulation of:

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

The objective is to determine how the target handles violations of expected timing.

---

# Replay Testing

The platform will support recording and replaying CAN traffic.

A captured frame will contain:

```text
Timestamp
CAN ID
DLC
Payload
```

Planned replay modes:

```text
Single frame
Frame sequence
Timing-preserved sequence
Looped sequence
Accelerated sequence
Slowed sequence
```

This can be used to evaluate whether systems improperly accept stale or repeated commands.

---

# Arbitration Testing

CAN arbitration gives numerically lower identifiers higher priority.

The project will support controlled experiments involving high-priority traffic.

Example:

```text
Normal:

0x500
0x520
0x700

Test traffic:

0x001
```

Experiments will evaluate whether higher-priority traffic affects the ability of legitimate nodes to transmit within expected timing constraints.

These tests will be explicitly armed and rate limited.

---

# Bus Load Testing

The platform will eventually support controlled CAN load generation.

Potential targets:

```text
10%
25%
50%
75%
90%
```

The purpose is to identify degradation thresholds rather than simply transmitting at the maximum possible rate.

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

# Target ECU Simulator

A later milestone will introduce a dedicated target ECU simulator.

The simulator will provide predictable CAN behavior so experiments can be reproduced without requiring production hardware.

Example message map:

| CAN ID  | Function         |   Rate |
| ------- | ---------------- | -----: |
| `0x100` | Controller State | 500 ms |
| `0x120` | RPM              |  50 ms |
| `0x130` | Temperature      | 250 ms |
| `0x200` | Actuator Command |  Event |
| `0x210` | Actuator Status  | 100 ms |
| `0x700` | Heartbeat        | 100 ms |

Example state machine:

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

The simulator will expose enough telemetry to determine how fuzzing affects application behavior.

---

# Experiment Model

Experiments should follow a repeatable lifecycle:

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

Recovery is part of the measurement.

---

# Experiment Profiles

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

The seed allows generated test inputs to be reproduced.

---

# Observability

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
      ↓
CAN Behavior
      ↓
Application Behavior
      ↓
System Impact
      ↓
Recovery
```

---

# Recovery Testing

A system temporarily entering a degraded state is different from a system remaining permanently unavailable.

Recovery should therefore be explicitly measured.

Potential metrics:

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

| Level | Result                                |
| ----- | ------------------------------------- |
| 0     | No observable impact                  |
| 1     | Temporary degradation                 |
| 2     | Automatic recovery required           |
| 3     | Component reset required              |
| 4     | Manual intervention required          |
| 5     | Persistent unsafe or unexpected state |

---

# Host-Side Tooling

A future Python host application will provide experiment orchestration.

Desired interface:

```bash
canchaos status

canchaos profiles

canchaos capture --duration 60

canchaos run profiles/rpm_mutation.json

canchaos analyze results/CAN-2026-09-06-0042/
```

Initial communication will use USB serial.

Future interfaces may include:

```text
SocketCAN
TCP/IP
WiFi
REST API
```

---

# Linux / SocketCAN Integration

Linux will eventually provide independent CAN monitoring and experiment capture.

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

* Attack generation
* Monitoring
* Target behavior

---

# Aerospace / Embedded Context

CAN is used across automotive, industrial, robotics, aerospace, and other embedded systems.

This project is not intended to model one specific vehicle or platform.

However, the testing methodology is particularly relevant to systems where CAN communication participates in:

* Distributed control
* Sensor communication
* Actuator control
* Subsystem coordination
* Health/status reporting

The repository includes additional documentation exploring relationships to CANaerospace and ARINC 825-style environments.

See:

```text
docs/aerospace-mapping.md
```

---

# Threat Model

The project assumes a test scenario in which a node capable of transmitting onto the CAN network is compromised, malfunctioning, malicious, or intentionally acting as an adversarial test device.

The project is therefore primarily interested in **post-bus-access resilience**.

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

Can the system recover?
```

See:

```text
docs/threat-model.md
```

---

# Roadmap

## v0.1 — CAN Foundation

* [ ] PlatformIO environment
* [ ] Native RA4M1 CAN
* [ ] TX/RX
* [ ] SAFE/ARMED state machine
* [ ] Serial command interface
* [ ] Hardware validation

## v0.2 — Core Fuzzer

* [ ] Random CAN fuzzing
* [ ] Known-ID mutation
* [ ] Mutation strategies
* [ ] Deterministic seeds
* [ ] Experiment duration

## v0.3 — Chaos Engine

* [ ] Timing jitter
* [ ] Bursts
* [ ] Replay
* [ ] Arbitration testing
* [ ] Controlled load generation

## v0.4 — Target ECU

* [ ] Target simulator
* [ ] CAN message map
* [ ] State machine
* [ ] Health telemetry
* [ ] Fault/recovery behavior

## v0.5 — Instrumentation

* [ ] Experiment IDs
* [ ] Structured logging
* [ ] Baseline capture
* [ ] Recovery measurement
* [ ] Result summaries

## v0.6 — Host Controller

* [ ] Python CLI
* [ ] Experiment profiles
* [ ] Automated execution
* [ ] Capture management
* [ ] Result analysis

## v1.0 — CAN Security Test Platform

* [ ] Reproducible experiments
* [ ] Defined test cases
* [ ] Independent monitoring
* [ ] Evidence capture
* [ ] Resilience metrics
* [ ] Recovery analysis
* [ ] Complete documentation

---

# Future Ideas

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

The immediate priority is building a reliable, observable, and reproducible CAN security testing foundation.

---

# Responsible Use

CAN Chaos Fuzzer is intended for:

* Security research
* Embedded systems testing
* Hardware-in-the-loop laboratories
* Authorized penetration testing
* Security verification
* Resilience testing
* Education

CAN fuzzing can disrupt communications, trigger unexpected behavior, or render connected systems unavailable.

**Do not connect this tool to vehicles, aircraft, industrial equipment, medical devices, production networks, or other operational systems without explicit authorization and appropriate safety controls.**

Development and testing should be performed on isolated laboratory CAN networks using hardware you own or are explicitly authorized to test.

---

# License

See `LICENSE` for project licensing information.

---

# Project Philosophy

Traditional verification often asks:

> **Does the system behave correctly when given the expected input?**

Security testing adds another question:

> **What happens when we intentionally violate those expectations?**

CAN Chaos Fuzzer is being built to explore that boundary.

**Break the assumptions. Observe the behavior. Measure the recovery. Reproduce the result.**
