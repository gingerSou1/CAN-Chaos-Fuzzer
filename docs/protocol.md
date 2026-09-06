# Serial Protocol

Milestone 1 uses newline-terminated ASCII commands over USB serial at 115200 baud.

Commands are lowercase. Spaces, tabs, carriage returns, vertical tabs and form
feeds delimit tokens; LF terminates a command (CRLF is supported). Empty lines are
ignored. The limit is 79 bytes before LF, including whitespace/CR. Overlong lines
and lines with invalid/non-ASCII control characters are rejected and discarded
through LF. Extra arguments are rejected. Numeric arguments must be unsigned
decimal uint32 values without a sign; overflow is rejected before range checks.

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
| `reset` | Stop, clear driver/experiment/log-drop counters, and return to SAFE; rejected in FAULT |

## Required Safety Behavior

- `start` is rejected unless the device is ARMED.
- The device boots SAFE.
- `stop` cancels the active experiment when parsed and returns to ARMED. It does
  not retract an already accepted controller frame. DISARM cancels and enters SAFE.
- Experiments are bounded by duration. The default is 30000 ms and the maximum is 300000 ms.
- Interval defaults to 1000 ms and must be between 1000 and 300000 ms inclusive.
- FAULT prohibits new application transmissions and remains latched until reboot.
- Status reports controller initialization/error state, accepted writes, RX,
  write errors, latched controller event code, experiment outcomes and dropped log
  lines. ONLINE means initialized with no error observed, not verified bus health.
- Output is buffered and may drop whole lines under load. Command execution does
  not depend on delivery of its response; query status again after output drains.

## Known-Frame Demo

`start` sends standard CAN ID `0x123` at the configured interval. Payload format:

```text
CA FE 00 01 [sequence: uint32 big-endian]
```

This frame is intentionally fixed so the sniffer can verify bus wiring, bitrate, and TX/RX before fuzzing behavior is added.
