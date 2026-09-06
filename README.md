# CAN Chaos Fuzzer

A controlled CAN bus security testing platform for isolated laboratory environments.

This project targets the Arduino UNO R4 WiFi using the board's native Renesas RA4M1 CAN controller and an external SN65HVD230 or compatible 3.3 V CAN transceiver.

Use only on isolated bench hardware you own or are explicitly authorized to test. Do not connect this tool to production vehicles, aircraft, or live safety-critical systems.

## Current Milestone

Milestone 1 is intentionally narrow:

- PlatformIO + Arduino framework project layout
- UNO R4 WiFi native CAN initialization at 500 kbps
- CAN driver abstraction
- SAFE, ARMED, RUNNING, and FAULT control states
- USB serial command interface
- bounded known-frame TX demo for bench validation
- existing sniffer sketch for observing frames

Fuzzing strategies, replay, load generation, and the target ECU simulator are later milestones.

## Build

```bash
pio run
```

The PlatformIO environment is `uno_r4_wifi`.

## Hardware

- Arduino UNO R4 WiFi
- SN65HVD230 or compatible 3.3 V CAN transceiver
- second sniffer node or USB-CAN monitor
- two 120 ohm termination resistors
- breadboard and jumpers

See `hardware/BOM.md` and `hardware/wiring.md`.

## Serial Commands

Open the PlatformIO serial monitor at 115200 baud:

```bash
pio device monitor
```

Commands:

```text
status
arm
disarm
start [duration_ms] [interval_ms]
stop
stats
reset
help
```

The fuzzer boots SAFE. `start` is rejected until `arm` succeeds.

Example Milestone 1 demonstration:

```text
status
start
arm
start 5000 1000
stop
```

During the known-frame demo, the fuzzer transmits standard CAN ID `0x123` with payloads beginning `CA FE 00 01`.

## Repository Layout

```text
CAN-Chaos-Fuzzer/
|-- platformio.ini
|-- include/
|-- src/
|-- docs/
|-- hardware/
|-- examples/
|-- tools/
`-- README.md
```

## References

- `docs/architecture.md`
- `docs/protocol.md`
- `docs/experiment-design.md`
- `docs/threat-model.md`
- `docs/aerospace-mapping.md`

## License

MIT. See `LICENSE`.
