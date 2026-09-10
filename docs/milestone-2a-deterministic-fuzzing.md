# Milestone 2A: Deterministic Known-ID Payload Fuzzing

Milestone 1 is COMPLETE at baseline `v0.1.0-can-foundation`. Milestone 2 is
IN PROGRESS. Milestone 2A adds deterministic payload generation and host-validated
orchestration; physical validation of these new fuzz runs is still pending.

## Objective and architecture

Generate reproducible eight-byte payload sequences for standard Classical CAN
ID `0x123`, DLC 8, at the existing low-rate safety limits.

`CommandInterface -> ExperimentManager -> FuzzEngine -> CanDriver` is the data
path. `FuzzEngine` is pure C++: fixed storage, no Arduino dependency, no CAN API,
no clocks, no allocation. It copies a base payload and produces eight bytes.
`ExperimentManager` owns the engine, constructs the CAN frame, schedules it and
accounts for its outcome. `CanDriver` and `SafetyManager` remain the authoritative
transmission and safety boundaries. There is no new transport framework.

The original `start [duration_ms] [interval_ms]` known-frame experiment retains
its sequence-counter behavior. Only the explicit `fuzz start` command selects
mutation. Fuzz frames use the fixed base:

```text
ID: 0x123  format: STD  DLC: 8
CA FE 00 01 00 00 00 00
```

Each generated payload starts from a fresh copy of that base. Fuzzing can change
all eight bytes, including the prefix. It never mutates the ID or DLC. A new
accepted fuzz start copies the base again, resets the PRNG/strategy position and
clears per-run progress. Rejected starts leave the current run unchanged.

## Deterministic PRNG and reproducibility contract

The implementation uses xorshift32 with a `uint32_t` state. Initialization is the
supplied unsigned seed, except seed 0 maps to `0x6D2B79F5` (1831565813). Each draw
updates the state in this exact order, with unsigned 32-bit shifts and XOR:

```cpp
state ^= state << 13;
state ^= state >> 17;
state ^= state << 5;
return state;
```

The first draw applies all three operations to the initialized state. No clock,
hardware entropy, Arduino random function or standard-library random distribution
is involved. This is a reproducibility tool, not a cryptographic PRNG. Seed 0
intentionally produces the same sequence as seed 1831565813.

For the same implementation, strategy, seed, base payload and configuration, each
generated frame index has identical ID, DLC and payload across repeated runs and
host/embedded builds. RANDOM seed 1 starts with `21 01 C5 4F D1 D0 1A B2`.
Timing metadata, wall-clock scheduling, controller acceptance and physical
delivery are outside the payload determinism contract. STOP, FAULT or timeout
may truncate the generated/accepted sequence. Resubmitting an accepted start
restarts at index 0; there is no continuation from a previous experiment.

## Strategy definitions

CLI names are lowercase; payload byte 0 is the leftmost displayed byte.

| Strategy | Exact output ordering |
| --- | --- |
| `random` | Draw once per byte, bytes 0 through 7. Use the low eight bits of each successive PRNG word: eight draws per frame. |
| `bitflip` | Draw once per frame; select `word & 63`. Flip exactly one bit of the fresh base: byte `bit / 8`, mask `1 << (bit % 8)`. Bit 0 is byte 0's least significant bit. |
| `zero` | Replace all eight bytes with `00`. |
| `ff` | Replace all eight bytes with `FF`. |
| `boundary` | Fill all eight bytes with one value per frame: `00`, `01`, `7F`, `80`, `FE`, `FF`, then repeat every six frames. Starts at `00`. |
| `walkingbit` | Zero all bytes, then set one bit. Start at byte 7 mask `01`, then `02` through `80`; continue at byte 6 mask `01`, down through byte 0 mask `80`. Repeat every 64 frames. |

Only `random` and `bitflip` consume PRNG words. The other four strategies accept
and report the seed but are seed-independent. Additional strategies can be added
through the strategy enum, parsing/name mapping and engine switch, with explicit
ordering tests. Optional BYTE_MUTATE/INCREMENT/DECREMENT are deferred.

## CLI and limits

```text
arm
fuzz start <strategy> <seed> <count> <interval_ms>
status
stop
```

Examples (each start requires ARMED with no active experiment):

```text
fuzz start random 1337 10 1000
fuzz start bitflip 1337 20 1000
fuzz start boundary 1 12 1000
fuzz start walkingbit 1 64 1000
```

All numeric arguments are required unsigned decimal uint32 values. Signs, hex,
fractional values, malformed input, overflow, unknown strategies, missing/extra
arguments and invalid subcommands are rejected. Existing whitespace, line-length,
one-command-per-poll and 32-input-byte budgets are unchanged.

Count must be nonzero; interval must be 1000 through 300000 ms. To retain the
existing bounded-duration policy, `count <= 300000 / interval_ms` is required.
Thus at 1000 ms the maximum count is 300, and the run's duration budget is exactly
`count * interval_ms`. Validation uses division before multiplication to avoid
overflow. Seed `4294967295` is valid; `4294967296` is rejected.

## Lifecycle, scheduling and safety

Boot remains SAFE. ARM alone generates and transmits nothing. A valid fuzz start
uses `SafetyManager::start()` to enter RUNNING; it does not write a frame itself.
The first update is immediately eligible, then at most one send attempt occurs
per configured interval. Unsigned elapsed-time arithmetic handles millis wrap.
Delayed loops never cause catch-up bursts and no blocking delay is introduced.

For an eligible frame, the engine advances once and the manager retains the
generated payload until accepted. A driver rate-limit rejection does not advance
the sequence again: the same payload is retried on the next eligible update.
The driver's minimum TX interval remains authoritative, including across
STOP/START and reset. A failed hardware write is never retried: it latches FAULT
and cancels the experiment through the existing path.

Reaching the requested **accepted** count completes the run and returns ARMED.
The deadline is checked before sending; if reached before the count is accepted,
the run stops and returns ARMED without incrementing completed. For example,
an immediate restart can encounter the existing driver's rate limit, or a delayed
loop can miss opportunities. Neither condition extends the duration budget.
Compare requested/accepted counts and the stopped counter to identify a partial
run. Timeout and explicit STOP share the existing stopped outcome counter.

STOP and DISARM cancel through the existing manager. Runtime write failure or
asynchronous controller error causes the existing latched FAULT and cancellation,
before any further application-level transmission. Reconnect cannot clear FAULT;
reboot creates fresh SAFE/IDLE state without restarting. `reset` remains rejected
in FAULT; outside FAULT it stops first, resets counters and clears fuzz metadata.

No hardware cancellation of an already accepted/queued frame is promised.
Controller acceptance is not proof of on-bus delivery. The independent Minima
sniffer remains the source of physical delivery evidence.

## Statistics and observability

`status`/`stats` retain the existing fields and add `EXPERIMENT_MODE` (`KNOWN` or
`FUZZ`). Mode identifies the selected/last experiment even after it becomes IDLE.
In FUZZ mode they also show:

| Field | Meaning |
| --- | --- |
| `FUZZ_STRATEGY`, `FUZZ_SEED` | Selected strategy and original user seed, including 0. |
| `FUZZ_REQUESTED`, `FUZZ_INTERVAL_MS` | Reproduction configuration; base/ID/DLC are fixed above. |
| `FUZZ_GENERATED` | Per-run unique payloads generated, including a pending or failed frame. |
| `FUZZ_TX_ACCEPTED` | Per-run accepted writes, not confirmed deliveries. |
| `FUZZ_LAST_INDEX` | Zero-based last generated index; omitted until generation begins. |
| `FUZZ_LAST_GENERATED` | Last generated frame's software generation timestamp, ID, format, DLC and compact hexadecimal payload; omitted before generation. |

Existing `EXPERIMENTS_STARTED/COMPLETED/STOPPED/FAULTED` count both experiment
types, with one outcome per run. `REJECTED_STARTS` counts manager-level state or
configuration rejections; syntax/numeric parsing failures do not increment it,
consistent with the original CLI. `KNOWN_FRAMES_ACCEPTED` remains exclusive to
the original demo. Global `TX_ACCEPTED` includes both modes. Fuzz progress resets
on every successful fuzz start; global counters persist until reset/reboot.
No duplicate fuzz outcome counters are added.

Status uses the existing fixed-size bounded logger; it does not print directly
to Serial. No automatic per-frame logging or capture buffer is added. Only the
latest generated payload is retained, and output can drop whole lines under
backpressure (`LOG_DROPPED_LINES`). Query again after draining. Bounded logging
takes priority over exhaustive evidence; reproducibility uses the reported
configuration and documented algorithm, with an independent capture for delivery.
The pinned UART's potential synchronous wait remains a Milestone 1 limitation.

## Host acceptance and software validation

The 18 Milestone 1 tests are retained. Fourteen new Unity regressions cover:

- RANDOM golden bytes, same-seed repetition, different-seed divergence, seed 0
  aliasing, and engine restart.
- ZERO/FF constants, repeated BOUNDARY cycles, two WALKING_BIT cycles, BIT_FLIP
  exact selected bits and base-copy ownership.
- Repeated experiments for every strategy, fixed ID/DLC, acceptance completion
  and separation from known-frame counters.
- SAFE rejection, explicit ARM/START, active-start rejection, authoritative
  driver rate limiting without skipping generated data, STOP/DISARM/reset,
  unchanged known-demo payload sequence, timeout and wraparound/no catch-up.
- Write-failure FAULT, asynchronous error before generation, blocked subsequent
  transmission/restart after recovery, and fresh SAFE state on reconstructed boot.
- Invalid strategies, syntax, numeric input/overflow, zero count, unsafe intervals,
  excessive count/duration, seed maximum, status contents and logging backpressure.

Software validation on 2026-09-09:

- `pio test -e host`: **32 tests passed, 0 failures**.
- `pio run -e uno_r4_wifi -t clean`: PASS; subsequent WiFi build PASS.
- `pio run -e uno_r4_wifi -e sniffer`: PASS for both targets, including the final
  active-run reset correction. Sniffer explicitly targets `uno_r4_minima`.
- Changed C++ files: `clang-format --dry-run --Werror` PASS.

Tests exercise production control/engine code with host boundary fakes; they do
not establish physical CAN behavior. No firmware upload or hardware test was
performed. The final whitespace/status checks are recorded in the review report.

## Future hardware validation procedure

Use the existing isolated two-node 500 kbps bench, WiFi transmitter and independent
Minima sniffer. This is a procedure for later review, not a record of execution.

1. Record the firmware revision, build environment, strategy, original seed,
   count, interval and fixed base; clear the sniffer capture between runs.
2. Verify SAFE boot, rejected fuzz start in SAFE and no application frames from
   ARM alone. Repeat the original `start 5000 1000` known-frame check.
3. Run `fuzz start random 1 10 1000` from ARMED. Compare ID/DLC/payloads with host
   generation; first payload must be `2101C54FD1D01AB2`. Save independent capture
   and final status; distinguish accepted from received counts.
4. Repeat the same configuration after a fresh explicit start, allowing the
   driver's minimum spacing. Compare ordered payloads; change seed and compare.
5. Capture ZERO, FF, 12 BOUNDARY frames, 64 WALKING_BIT frames and seeded BIT_FLIP
   frames against the ordering above.
6. Check STOP/DISARM during a run, completion to ARMED/IDLE and reset without
   automatic restart. Quantitative STOP latency remains unmeasured until separately
   instrumented; already queued frames are not evidence of new scheduling.
7. In a separately controlled deliberate loss-of-ACK test, confirm FAULT, IDLE,
   a faulted outcome and no further new application writes. Record whether write
   failure or asynchronous controller error was actually observed. Reconnect must
   preserve FAULT; reboot with the bus restored must return SAFE/IDLE.

## Limitations and deferred scope

Only the six strategies, fixed base, standard ID `0x123` and DLC 8 are implemented.
No ID fuzzing, timing chaos, flooding, CAN FD, replay, physical fault generation,
UI, trainer/scenario, sensors or generic multi-protocol scaffolding is included.
Seeds do not guarantee different outputs for every possible pair, and periodic
strategies deliberately repeat. Host determinism and successful builds do not
replace physical validation. No new claim about controller callbacks, queued-frame
abortability or absolute STOP latency is made.
