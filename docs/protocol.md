# Serial Protocol

The interactive CLI uses ASCII commands over USB serial at 115200 baud.

## Interactive console

Startup prints the project name, CAN initialization result, actual safety state,
a short help hint and `CAN> ` (including a trailing space, without a newline).
Type `help` or `menu` to display the concise command menu at any safety state.
Neither command changes safety or starts an experiment.

The firmware echoes printable ASCII bytes (`0x20` through `0x7E`) as input is
processed. Disable terminal-side local echo to avoid doubled characters. Use a
terminal mode that sends keystrokes immediately; host-side line buffering delays
firmware editing/echo until the host sends the line. Prompts and echoes are queued
behind earlier output and drained through the normal logger budget.

Both Backspace (`0x08`) and Delete (`0x7F`) remove the last buffered character and
emit `BS SPACE BS` to erase one terminal cell. At an empty buffer they do nothing.
Tabs, vertical tabs and form feeds remain token separators but echo as one space,
so each stored character occupies one editable cell. There is no cursor movement,
history or ANSI escape-sequence support.

CR or LF submits a line. CRLF submits it once; the immediately following LF is
consumed without another response or prompt, even across separate polls. Empty
lines execute nothing and display a fresh prompt. Enter queues a CRLF display
newline, executes at most one command, queues its response, then queues `CAN> `.
Commands after that line wait for a later poll; skipping CRLF's LF does not spend
an extra command slot, but still consumes an input byte from the current budget.

Commands are lowercase. Spaces, tabs, vertical tabs and form feeds delimit tokens.
The buffer holds at most 79 input bytes, including token whitespace but excluding
Enter. Backspace frees a byte while the line is valid. Once overflow or an invalid
byte occurs, the entire line is rejected and remaining input is discarded through
CR or LF; backspace cannot rescue it. Invalid bytes and discarded suffixes are
not echoed. Enter reports the rejection once and restores the prompt. Extra
arguments are rejected. Numeric arguments must be unsigned decimal uint32 values
without a sign; overflow is rejected before range checks.

This changes the older LF-only protocol: CR is now a terminator, not in-line
whitespace. Scripted clients must also account for prompt/echo output. The command
syntax, experiment logic, seeds and safety gates are unchanged.

Input remains capped at 32 bytes and one submitted line per poll, with no blocking
line read or dynamic allocation. Prompt/echo/erase fragments use the existing
2048-byte output queue and 32-output-byte drain budget. A fragment is enqueued
whole or dropped whole; ordinary log lines keep their existing whole-line policy.
`LOG_DROPPED_LINES` now includes dropped console fragments as well as log lines.
Control does not wait for the display: under sustained backpressure, echo or a
prompt can be lost. After output drains, an empty Enter restores a prompt.
Unsolicited RX logs share the terminal and may interrupt the visible input line;
the command buffer remains intact. Automatic line redraw is not implemented.
The existing framework UART synchronous-write limitation still applies.

## Commands

| Command | Description |
| --- | --- |
| `help` | Print available commands |
| `menu` | Alias for `help`; print the same command menu |
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
  Console fragments can also be dropped as described above.

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
