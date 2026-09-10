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
| `fuzz start <strategy> <seed> <count> <interval_ms>` | Start deterministic known-ID payload fuzzing from ARMED; all arguments required |
| `stop` | Immediately stop the active experiment and return to ARMED |
| `stats` | Alias for current status output |
| `reset` | Stop, clear driver/experiment/log-drop counters, and return to SAFE; rejected in FAULT |

## Required Safety Behavior

- `start` is rejected unless the device is ARMED.
- The device boots SAFE.
- `stop` cancels the active experiment when parsed and returns to ARMED. It does
  not retract an already accepted controller frame. DISARM cancels and enters SAFE.
- Known-frame experiments default to 30000 ms duration, with a maximum of 300000 ms.
- Known-frame interval defaults to 1000 ms; all experiment intervals must be between 1000 and 300000 ms inclusive.
- Fuzz runs require nonzero count and `count <= 300000 / interval_ms`. They complete
  when count writes are accepted, or stop at the `count * interval_ms` duration limit.
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

## Deterministic payload fuzzing (Milestone 2A)

Strategies: `random`, `bitflip`, `zero`, `ff`, `boundary`, `walkingbit`.
Seed is decimal uint32, including 0; seed 0 maps to `0x6D2B79F5` internally.
Example: `arm`, then `fuzz start random 1337 10 1000`. The existing plain `start`
command retains its known-frame sequence behavior. STOP/DISARM/FAULT semantics
and the authoritative driver rate limit apply to both modes.

Status adds `EXPERIMENT_MODE` and, in FUZZ mode, strategy, seed, requested count,
interval, generated/accepted counts and the last generated index/frame. These
per-run fields remain available after termination until reset or another start.
Existing experiment outcome counters cover both modes; known-frame counters stay
exclusive to the original demo. Parse failures do not increment `REJECTED_STARTS`.
Logging remains bounded and may drop lines. See the [Milestone 2A specification](milestone-2a-deterministic-fuzzing.md)
for exact PRNG/strategy ordering, retry/timeout semantics and physical validation.
