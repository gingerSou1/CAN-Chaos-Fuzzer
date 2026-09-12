# Experiment Design

Milestone 1 has one experiment mode: a bounded known-frame transmission demo.

Purpose:

- Prove the PlatformIO toolchain.
- Prove UNO R4 native CAN initialization at 500 kbps.
- Prove the fuzzer can transmit standard CAN frames.
- Prove a sniffer can observe those frames on the isolated bench bus.
- Prove unsafe starts are rejected before transmission.

Acceptance demonstration:

1. Flash the fuzzer firmware.
2. Flash or connect the sniffer.
3. Confirm `status` reports `STATE: SAFE`.
4. Send `start` and confirm `ERROR: DEVICE NOT ARMED OR UNSAFE CONFIG`.
5. Send `arm` and confirm `STATE ARMED`.
6. Send `start 5000 1000`.
7. Confirm the sniffer observes ID `0x123` and payloads beginning with `CA FE 00 01`.
8. Send `stop` during a longer run and confirm no new application writes are scheduled. This does not establish quantitative STOP latency or retract a queued controller frame.

Milestone 2A adds deterministic known-ID payload fuzzing. See the [Milestone 2A specification](milestone-2a-deterministic-fuzzing.md) for implemented strategies, bounds and counters; the known-frame mode above remains supported.

## Historical design sketches and future experiments

The following design sketches were moved from the early README. They preserve research ideas, examples and possible interfaces, not implementation commitments. Milestone 2A now implements six fixed-ID payload strategies; its specification is authoritative. Other IDs, intervals below 1000 ms, profiles and commands illustrated below are future concepts and are not accepted by the current firmware.

### Known-ID Payload Mutation

The first fuzzing primitive was known-ID payload mutation. The current implementation fixes ID 0x123 and DLC 8; these earlier 0x120 examples illustrate future target-specific payloads.

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

Milestone 2A implements seeded payload reproducibility. Profile-based orchestration below remains a future concept.

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

## Direction and deferred scope

The project began with CAN communication, then safe experiment control, then
seeded payload fuzzing. Its direction has evolved toward building a working
embedded/product system and testing it through BUILD -> VERIFY -> BREAK ->
HARDEN -> RETEST. A complete cyber-physical system was not the original scope.

Current foundation: native CAN, explicit safety states, bounded known-frame and
payload experiments, independent observation and reproducible host tests.
Near-term work continues the fuzzing core. Later work may explore timing, replay,
bursts, ordering and load, then a target controller/system, physical sensors,
actuators and displays, a Wi-Fi experiment console, threat modeling/security
assessment, hardening and deterministic security regression. This is a direction,
not a schedule or an instruction to build scaffolding now.

Earlier README ideas retained for future consideration:

- Byte mutation, increment/decrement, configurable IDs/DLC, deterministic profiles.
- Experiment IDs, per-frame evidence, structured logs, capture baselines, result
  summaries, recovery windows and recovery measurements.
- A target CAN message map, application state machine, health telemetry and
  explicit fault/recovery behavior (the simulator sketch above is one option).
- Python CLI/profile orchestration, automated runs, capture management and analysis.
- CAN FD, DBC parsing, ISO-TP, UDS, protocol-aware/coverage-guided fuzzing,
  SocketCAN, multiple targets/fuzzing nodes, hardware-in-the-loop testing,
  anomaly detection and CI-connected bench validation.
- Possible Ethernet, serial, Modbus or ARINC 429 exploration only when justified
  by a future target; no generic transport framework exists today.

A future release would need defined test cases, independent evidence capture,
resilience metrics, recovery analysis, reproducible builds and complete supporting
documentation. Speculative examples above are not current CLI or target contracts.

Quantitative STOP latency remains a separate follow-up measurement. UART blocking,
queued frames and independent delivery evidence are discussed in the
[developer guide](developer-guide.md) and [Milestone 1C record](milestone-1c-validation.md).
