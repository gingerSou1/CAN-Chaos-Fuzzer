# Milestone 1C bench validation

Date: 2026-09-07. Transmitter: UNO R4 WiFi, COM11. Receiver: UNO R4 Minima, COM9. CAN: 500 kbps. Two-node physical CAN bench.

**Milestone 1C: COMPLETE.** The following record incorporates the operator-reported current-firmware hardware validation.

| Check | Result | Evidence |
| --- | --- | --- |
| Known-frame delivery | PASS | Five frames received, ID `0x123`, payload prefix `CA FE 00 01`, sequence 0–4; zero TX errors. |
| Start rejected while SAFE | PASS | `start 5000 1000` returned `ERROR: DEVICE NOT ARMED OR UNSAFE CONFIG`; no frames captured. |
| STOP during active run | PASS on healthy bus | Ten-second run reported RUNNING/ACTIVE with 7682 ms remaining. Three frames received before STOP. STOP reported ARMED/IDLE; TX remained 8 cumulatively and no further frames arrived during a three-second observation. Explicit disarm then returned SAFE. |
| Reboot from SAFE | PASS after user reset | COM11 reconnected and a status-only query reported SAFE, CAN ONLINE at 500000, EXPERIMENT IDLE, TX 0, RX 0, and TX_ERRORS 0. TX had been 8 before reset. No arm/start commands were sent after reset. |
| Reboot from ARMED | PASS after user reset | Confirmed ARMED with CAN ONLINE and EXPERIMENT IDLE before reset. After reset, a status-only query reported SAFE, CAN ONLINE, EXPERIMENT IDLE, and zero counters. |
| Reboot from RUNNING | PASS after user reset | Started `start 30000 1000`; confirmed RUNNING/ACTIVE with 28692 ms remaining and two frames received by the Minima (sequence 0–1). Reset was performed while the experiment was active. After reset, two status-only queries reported SAFE, CAN ONLINE, EXPERIMENT IDLE, and zero counters. No Minima frames arrived during a five-second observation after clearing its input buffer. No arm/start commands were sent after reset. Reset timing was user-operated, not independently instrumented. |
| Disconnect during transmission enters FAULT and cancels | PASS | Disconnect while RUNNING caused a CAN write failure (-60003), FAULT, CAN ERROR, experiment cancellation, and EXPERIMENTS_FAULTED: 1. |
| Recovery after deliberate fault | PASS | Reconnect preserved FAULT/ERROR and IDLE with TX_ACCEPTED unchanged at 49. Reboot restored SAFE, CAN ONLINE, IDLE, and zero counters without automatic restart. |

## Current-firmware hardware observations

Healthy baseline with current firmware:

- 10-second experiment
- Minima received all 10 frames
- ID 0x123
- sequence 0-9
- TX_ACCEPTED: 10
- TX_ERRORS: 0
- CONTROLLER_ERRORS: 0
- EXPERIMENTS_STARTED: 1
- EXPERIMENTS_COMPLETED: 1
- EXPERIMENTS_FAULTED: 0
- final state ARMED/IDLE

Controlled fault:

- Started a 30-second experiment at 1000 ms interval.
- Several frames were successfully received before the Minima-side CAN connection was physically disconnected while RUNNING.
- Loss of the receiving/ACK node caused the transmitter to enter FAULT.
- The hardware-observed fault was detected through `CAN.write()` returning `-60003`; asynchronous controller-error counters remained zero.

Observed fault state:

```text
STATE: FAULT
CAN: ERROR
BITRATE: 500000
EXPERIMENT: IDLE
REMAINING_MS: 0
TX_ACCEPTED: 49
RX: 0
TX_ERRORS: 1
TX_RATE_LIMITED: 0
LAST_WRITE_RESULT: -60003
CONTROLLER_ERRORS: 0
LAST_CONTROLLER_ERROR: 0
EXPERIMENTS_STARTED: 3
EXPERIMENTS_COMPLETED: 2
EXPERIMENTS_STOPPED: 0
EXPERIMENTS_FAULTED: 1
REJECTED_STARTS: 0
KNOWN_FRAMES_ACCEPTED: 49
LOG_DROPPED_LINES: 0
```

This demonstrates:

- runtime transmission failure caused FAULT
- active experiment was cancelled
- experiment did not complete normally
- FAULT was latched
- CAN state reported ERROR

While still faulted, attempted:
`start 5000 1000`

Result:
`ERROR: DEVICE NOT ARMED OR UNSAFE CONFIG`

Afterward:

- REJECTED_STARTS incremented to 1
- STATE remained FAULT
- EXPERIMENT remained IDLE
- TX_ACCEPTED remained 49

The physical CAN connection was then restored WITHOUT rebooting.

Observed after reconnect:

```text
STATE: FAULT
CAN: ERROR
EXPERIMENT: IDLE
TX_ACCEPTED: 49
TX_ERRORS: 1
LAST_WRITE_RESULT: -60003
EXPERIMENTS_FAULTED: 1
REJECTED_STARTS: 1
```

No automatic experiment restart occurred and no new application-level transmissions were observed. Restoring the physical bus did not clear the latched application fault.

Finally, with the CAN bus physically restored, the UNO R4 WiFi was rebooted.

Observed after reboot:

```text
STATE: SAFE
CAN: ONLINE
BITRATE: 500000
EXPERIMENT: IDLE
REMAINING_MS: 0
TX_ACCEPTED: 0
RX: 0
TX_ERRORS: 0
TX_RATE_LIMITED: 0
LAST_WRITE_RESULT: 0
CONTROLLER_ERRORS: 0
LAST_CONTROLLER_ERROR: 0
EXPERIMENTS_STARTED: 0
EXPERIMENTS_COMPLETED: 0
EXPERIMENTS_STOPPED: 0
EXPERIMENTS_FAULTED: 0
REJECTED_STARTS: 0
KNOWN_FRAMES_ACCEPTED: 0
LOG_DROPPED_LINES: 0
```

Recovery was FAULT -> reboot -> SAFE. CAN reinitialized ONLINE, the experiment remained IDLE, statistics reset, and no automatic experiment restart occurred.

The fault snapshot contains cumulative session counters: 49 accepted writes does not mean 49 frames were physically delivered during the faulted experiment. The asynchronous `CAN.isError()` path was not observed during this hardware fault; its handling remains host-test validated. ONLINE means initialized with no error observed, not independently verified bus health.

## Implementation findings

`CanDriver::pollHealth()` checks the Arduino CAN controller error state and latches observed controller errors into both the CAN driver error state and the safety FAULT state. A failed hardware write also causes a fault. Local policy rejection, invalid frames, and transmit rate limiting do not themselves represent controller faults.

`CanDriver` holds a reference to `SafetyManager`, so transmit permission is enforced at the CAN write boundary. `send()` polls controller health before transmission and requires the system to be RUNNING, the controller to be ONLINE, the frame to be a valid Classical CAN standard frame, and the minimum transmit interval to have elapsed.

The main loop polls CAN health before processing commands. If Safety is in FAULT, the active experiment is stopped before further command processing. `ExperimentManager::update()` also checks CAN health and cancels an active experiment when transmission is no longer permitted.

FAULT is latched for the lifetime of the running firmware instance. Restoring the CAN bus does not automatically return the application to ARMED or RUNNING. A hardware reboot creates fresh application state; the recovery bench test confirmed SAFE and CAN ONLINE after successful initialization.

`controllerErrors` records observed controller errors. `lastControllerError()` retains the most recently latched controller error code. `TX_ERRORS` records failed or rejected transmit attempts according to the current driver accounting. `TX_ACCEPTED` counts writes accepted by the Arduino CAN API and is not proof of physical on-bus delivery.

FAULT prevents future application-level transmissions. A frame already accepted or queued by the CAN controller is not guaranteed to be retractable. This implementation does not claim hardware-level cancellation of an already-pending controller transmission. STOP and FAULT prevent new application scheduling.

The earlier unsuccessful wiring test yielded one accepted write, four errors with code `-60003` (`transmit still in progress`), and no receiver frames. That was not a controlled disconnect-during-transmission test and is not counted as Milestone 1C fault-injection evidence.

The earlier accidental Minima reset was not the deliberate fault test and is not counted as fault evidence. STOP behavior passed on the healthy bus; quantitative STOP latency was not measured in the supplied evidence.

## Code validation

Previously completed software validation reported for this closeout (not rerun for this documentation-only update):

- `pio test -e host -v`: PASS — 18 tests, 0 failures.
- `pio run -e uno_r4_wifi`: PASS.
- `pio run -e sniffer`: PASS, explicitly targeting `uno_r4_minima`.
- Host regression coverage includes runtime write failure during RUNNING, asynchronous controller error handling, experiment cancellation, blocked transmission after FAULT, fault latching, initialization failure, transmit rate limiting, bounded loop behavior, and related safety-state transitions.

The current PlatformIO configuration explicitly targets the UNO R4 Minima for the sniffer environment:

```ini
[env:sniffer]
extends = env:uno_r4_wifi
board = uno_r4_minima
build_src_filter = -<*> +<sniffer_main.cpp>
```

The earlier bench PASS results are retained alongside the completed current-firmware baseline and deliberate disconnect/recovery validation.

The acceptance criterion is that no new application-level transmissions are scheduled or accepted after FAULT, and restoring the bus does not clear FAULT or resume the experiment. An already accepted/queued controller frame cannot be assumed abortable. The reported observations satisfy this criterion and complete Milestone 1C.
