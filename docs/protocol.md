# Serial Protocol

Milestone 1 uses newline-terminated ASCII commands over USB serial at 115200 baud.

## Commands

| Command | Description |
| --- | --- |
| `help` | Print available commands |
| `status` | Print safety state, CAN status, active experiment status, and counters |
| `arm` | Transition from SAFE to ARMED |
| `disarm` | Stop any active experiment and return to SAFE |
| `start [duration_ms] [interval_ms]` | Start the known-frame TX demo from ARMED |
| `stop` | Immediately stop the active experiment and return to ARMED |
| `stats` | Alias for current status output |
| `reset` | Stop, clear experiment counters, and return control state to SAFE |

## Required Safety Behavior

- `start` is rejected unless the device is ARMED.
- The device boots SAFE.
- `stop` preempts the active experiment immediately.
- Experiments are bounded by duration. The default is 30000 ms and the maximum is 300000 ms.

## Known-Frame Demo

`start` sends standard CAN ID `0x123` at the configured interval. Payload format:

```text
CA FE 00 01 [sequence: uint32 big-endian]
```

This frame is intentionally fixed so the sniffer can verify bus wiring, bitrate, and TX/RX before fuzzing behavior is added.
