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

Fuzzing strategies, replay, load generation, and the target ECU simulator belong to later milestones.
