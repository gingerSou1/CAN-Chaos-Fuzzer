# Milestone 1C bench validation

Date: 2026-09-07. Transmitter: UNO R4 WiFi, COM11. Receiver: UNO R4 Minima, COM9. CAN: 500 kbps.

Milestone remains incomplete pending deliberate runtime CAN fault and recovery bench validation.

| Check | Result | Evidence |
| --- | --- | --- |
| Known-frame delivery | PASS | Five frames received, ID `0x123`, payload prefix `CA FE 00 01`, sequence 0–4; zero TX errors. |
| Start rejected while SAFE | PASS | `start 5000 1000` returned `ERROR: DEVICE NOT ARMED OR UNSAFE CONFIG`; no frames captured. |
| STOP during active run | PASS on healthy bus | Ten-second run reported RUNNING/ACTIVE with 7682 ms remaining. Three frames received before STOP. STOP reported ARMED/IDLE; TX remained 8 cumulatively and no further frames arrived during a three-second observation. Explicit disarm then returned SAFE. |
| Reboot from SAFE | PASS after user reset | COM11 reconnected and a status-only query reported SAFE, CAN ONLINE at 500000, EXPERIMENT IDLE, TX 0, RX 0, and TX_ERRORS 0. TX had been 8 before reset. No arm/start commands were sent after reset. |
| Reboot from ARMED | PASS after user reset | Confirmed ARMED with CAN ONLINE and EXPERIMENT IDLE before reset. After reset, a status-only query reported SAFE, CAN ONLINE, EXPERIMENT IDLE, and zero counters. |
| Reboot from RUNNING | PASS after user reset | Started `start 30000 1000`; confirmed RUNNING/ACTIVE with 28692 ms remaining and two frames received by the Minima (sequence 0–1). Reset was performed while the experiment was active. After reset, two status-only queries reported SAFE, CAN ONLINE, EXPERIMENT IDLE, and zero counters. No Minima frames arrived during a five-second observation after clearing its input buffer. No arm/start commands were sent after reset. Reset timing was user-operated, not independently instrumented. |
| Disconnect during transmission enters FAULT and cancels | PENDING HARDWARE TEST | Runtime fault handling is covered by passing host regression tests. Updated firmware has not yet been validated with a controlled loss-of-ACK or bus-disconnect condition. |
| Recovery after deliberate fault | PENDING HARDWARE TEST | Host tests verify FAULT remains latched after simulated controller recovery. Physical reconnect and reboot behavior after a deliberate CAN fault still require bench validation. |

## Implementation findings

`CanDriver::pollHealth()` checks the Arduino CAN controller error state and latches observed controller errors into both the CAN driver error state and the safety FAULT state. A failed hardware write also causes a fault. Local policy rejection, invalid frames, and transmit rate limiting do not themselves represent controller faults.

`CanDriver` holds a reference to `SafetyManager`, so transmit permission is enforced at the CAN write boundary. `send()` polls controller health before transmission and requires the system to be RUNNING, the controller to be ONLINE, the frame to be a valid Classical CAN standard frame, and the minimum transmit interval to have elapsed.

The main loop polls CAN health before processing commands. If Safety is in FAULT, the active experiment is stopped before further command processing. `ExperimentManager::update()` also checks CAN health and cancels an active experiment when transmission is no longer permitted.

FAULT is latched for the lifetime of the running firmware instance. Restoring the CAN bus does not automatically return the application to ARMED or RUNNING. A hardware reboot creates fresh application state and is expected to boot SAFE if CAN initialization succeeds.

`controllerErrors` records observed controller errors. `lastControllerError()` retains the most recently latched controller error code. `TX_ERRORS` records failed or rejected transmit attempts according to the current driver accounting. `TX` counts writes accepted by the Arduino CAN API and is not proof of physical on-bus delivery.

FAULT prevents future application-level transmissions. A frame already accepted or queued by the CAN controller is not guaranteed to be retractable. This implementation does not claim hardware-level cancellation of an already-pending controller transmission. STOP and FAULT prevent new application scheduling.

The earlier unsuccessful wiring test yielded one accepted write, four errors with code `-60003` (`transmit still in progress`), and no receiver frames. That was not a controlled disconnect-during-transmission test and is not counted as Milestone 1C fault-injection evidence.

## Code validation

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

The existing bench PASS results above were collected using the previously flashed firmware. The newly software-validated firmware has not yet completed deliberate runtime fault bench validation.

The next bench step is to repeat a short healthy communication check and then perform a controlled loss-of-ACK or disconnect test.

During the deliberate fault test, distinguish an already-pending controller transmission from resumed application scheduling. The acceptance requirement is that no new application-level transmissions are requested after FAULT and that restoring the bus does not automatically clear FAULT or resume the experiment.

Bench fault-injection validation remains incomplete until the deliberate disconnect/recovery test is performed.