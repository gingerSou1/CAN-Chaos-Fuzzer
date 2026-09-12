# CAN Chaos Fuzzer

CAN Chaos Fuzzer is an experimental embedded security test platform built around
native CAN on the Arduino UNO R4. It began as a CAN fuzzing experiment and has
progressed through safe experiment control to deterministic payload fuzzing.

The project is evolving toward a complete embedded/IoT system that can be built,
verified, security-tested, hardened and retested. That target system is future
work; today's implementation is a two-node CAN bench with a fuzzer and an
independent sniffer.

## Project Philosophy

**Build it. Verify it. Break it. Harden it. Retest it.**

This is a personal engineering and product-security research project motivated
by an interest in hardware, IoT, automotive and embedded systems. The approach
is to engineer a working system first, then test its assumptions, observe failure
and recovery, apply mitigations, and turn findings into reproducible regressions.

## Current Capabilities

- Native Renesas RA4M1 CAN, using the Arduino framework and external transceivers.
- Classical CAN at 500 kbps; experiments use standard ID `0x123`, DLC 8.
- SAFE / ARMED / RUNNING / FAULT control with driver-enforced transmit permission.
- Interactive, bounded serial CLI with `CAN>` prompt, echo and backspace editing.
- Original known-frame experiment for bring-up and independent delivery checks.
- Deterministic payload strategies: `random`, `bitflip`, `zero`, `ff`, `boundary`
  and `walkingbit`, with explicitly defined seed behavior.
- Bounded runs, at least 1000 ms between accepted writes, STOP and cancellation.
- Runtime CAN write/controller-error handling with latched FAULT.
- Bounded status/log output and generated-versus-accepted frame accounting.
- Independent UNO R4 Minima CAN sniffer, host tests and completed baseline bench
  validation. The new interactive console still needs hardware validation.

## Architecture

```text
PC terminal
    |
USB serial
    |
UNO R4 WiFi -- CAN Chaos Fuzzer
    |
CAN transceiver
    |
Isolated, terminated CAN bus
    |
CAN transceiver
    |
UNO R4 Minima -- Independent sniffer
```

The mutation engine produces payload data; the experiment manager schedules it.
Every application transmission passes through `CanDriver` and the existing
safety checks. See [software architecture](docs/architecture.md).

## Hardware and Software

- Arduino UNO R4 WiFi: fuzzer/transmitter.
- Arduino UNO R4 Minima: independent observer.
- SN65HVD230 CAN transceivers and short CANH/CANL bench wiring.
- A common ground and 120-ohm termination at each physical end of the bus.
- PlatformIO, Arduino framework and C++ firmware; Unity host tests.

The RA4M1 supplies the native CAN controller; an external MCP2515 is not used.
Review [wiring](hardware/wiring.md), [hardware architecture](hardware/architecture.md)
and the [bill of materials](hardware/BOM.md) before connecting the bench.
Pinned toolchain details are in the [developer guide](docs/developer-guide.md).

## Quick Start

Install PlatformIO and open this repository. VS Code with the PlatformIO extension
is one supported workflow; the following commands use its CLI.

```sh
git clone https://github.com/gingerSou1/CAN-Chaos-Fuzzer.git
cd CAN-Chaos-Fuzzer
pio run -e uno_r4_wifi -e sniffer
pio test -e host
```

The sniffer environment explicitly targets `uno_r4_minima`. Host tests require a
native compiler; see [host setup](docs/developer-guide.md#host-tests), including
the existing Windows portable-toolchain workflow.

Upload only to your isolated bench, replacing the port names with your actual
fuzzer and sniffer ports:

```sh
pio run -e uno_r4_wifi -t upload --upload-port FUZZER_PORT
pio run -e sniffer -t upload --upload-port SNIFFER_PORT
pio device monitor -p FUZZER_PORT -b 115200
```

Use a terminal that sends keystrokes as typed, with terminal-side local echo
**off**: the firmware supplies the echo. CR, LF and CRLF are accepted for Enter.
If your terminal buffers a whole line locally, choose its character-at-a-time
mode for interactive editing. Backspace (`0x08`) and Delete (`0x7F`) erase the
last input character. Type `help` or `menu` to redisplay the commands.

Monitor the sniffer separately on its port to verify physical delivery. No
experiment starts automatically when either board is connected.

## Example

A short interactive session after a successful boot:

```text
CAN Chaos Fuzzer
OK: CAN ONLINE 500000
STATE: SAFE
Type 'help' for commands.
CAN> arm
OK: STATE ARMED
CAN> fuzz start random 1337 10 1000
OK: FUZZ EXPERIMENT STARTED
CAN> stop
OK: EXPERIMENT STOPPED
CAN>
```

`status` or `stats` shows current state, CAN errors and experiment progress.
For the original bring-up demo, use `arm` followed by `start 5000 1000`.
The full syntax and input rules are in the [CLI reference](docs/protocol.md).

## Deterministic Fuzzing

The same strategy, seed, base payload and configuration reproduce the same
ordered generated payloads. A new accepted start resets generation to index 0.
The existing known-frame demo remains a separate mode.

Reproducibility concerns generated content, not identical timing or guaranteed
physical delivery. Controller acceptance is not confirmation that a sniffer
received a frame. See [Milestone 2A](docs/milestone-2a-deterministic-fuzzing.md)
for the PRNG, seed-zero rule, strategy ordering, counters and duration limits.

## Safety

Use an isolated bench with hardware you own or are authorized to test.
CAN traffic can disrupt connected systems; this is not a production safety device.

- Successful boot enters SAFE; transmission requires explicit ARM and START.
- `CanDriver` enforces transmit state, frame validation and minimum spacing.
- STOP/DISARM cancel new application scheduling; queued controller frames cannot
  be assumed retractable.
- Runtime CAN failures enter latched FAULT and cancel the active experiment.
- Reconnect does not clear FAULT; reboot returns SAFE if CAN initialization succeeds.
- Serial input, CAN receive work and logging remain bounded. Slow output can drop
  log lines or console fragments; command execution does not wait for display.

Quantitative STOP latency remains unmeasured. See the
[safety and scheduling contracts](docs/developer-guide.md#scheduling-and-safety-limits-milestone-1b)
and [security policy](SECURITY.md) for limitations and reporting guidance.

## Documentation

| Topic | Reference |
| --- | --- |
| Interactive CLI and command syntax | [Serial protocol](docs/protocol.md) |
| Components and repository layout | [Software architecture](docs/architecture.md) |
| Build, host setup and safety contracts | [Developer guide](docs/developer-guide.md) |
| Hardware setup | [Wiring](hardware/wiring.md), [architecture](hardware/architecture.md), [BOM](hardware/BOM.md) |
| Milestone 1 hardware evidence | [Milestone 1C validation](docs/milestone-1c-validation.md) |
| Payload generation and reproducibility | [Milestone 2A specification](docs/milestone-2a-deterministic-fuzzing.md) |
| Experiment methodology and retained future sketches | [Experiment design](docs/experiment-design.md) |
| Security assumptions | [Threat model](docs/threat-model.md) |
| Broader embedded context | [Aerospace mapping](docs/aerospace-mapping.md) |
| Change history and contribution rules | [Changelog](CHANGELOG.md), [Contributing](CONTRIBUTING.md) |

## Roadmap

Current: CAN foundation, safe experiment control and deterministic payload fuzzing.
Next: continue the fuzzing core and evaluate additional chaos capabilities.
Future: build a target embedded system with physical inputs/outputs, then explore
Wi-Fi control, security assessment, mitigations and deterministic regression.

Those directions are incremental research work, not implemented features or a
commitment to a generic framework. Earlier technical sketches are retained in
[experiment design](docs/experiment-design.md#historical-design-sketches-and-future-experiments).

## Status

Milestone 1 is complete (`v0.1.0-can-foundation`). Milestone 2A at `eecea63` is
implemented, host-tested and hardware-validated, as reported by the maintainer.
The interactive console update is separate from that validated baseline and has
not been uploaded or bench-tested as part of this change.

## License

[MIT License](LICENSE).
