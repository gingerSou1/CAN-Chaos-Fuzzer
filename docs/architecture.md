# Software Architecture

Milestone 1 establishes the firmware structure without implementing fuzz strategies.

```text
Command Interface
        |
Experiment Manager
        |
Safety Manager ---- CAN Driver
                         |
                 UNO R4 native CAN
```

## Components

- `CanDriver`: wraps the UNO R4 `Arduino_CAN` API and exposes standard frame send/receive operations.
- `SafetyManager`: owns BOOT, SAFE, ARMED, RUNNING, and FAULT transitions.
- `CommandInterface`: parses USB serial commands without dynamic allocation.
- `ExperimentManager`: runs the bounded known-frame TX demo used for CAN bring-up.
- `Logger`: prints status, errors, and observed CAN frames.

Milestone 1B retains these modules. `CanDriver` now holds a reference to the
`SafetyManager` so all application sends enforce the state gate, and propagates
latched controller errors into FAULT. `ExperimentManager` treats loss of transmit
permission as cancellation. `BufferedLogOutput` adds a fixed-size whole-line
queue inside Logger; each cooperative loop limits serial input, RX and log work.
No strategy engine or hardware-specific pin changes are introduced.

Fuzzing strategies, replay, load generation, and the target ECU simulator belong to later milestones.
