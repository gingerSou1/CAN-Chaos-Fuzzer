# Developer Guide

## Build

Install PlatformIO, then run:

```bash
pio run
```

The active environment is `uno_r4_wifi`.

## Firmware Layout

- Public headers are in `include/`.
- Firmware implementation lives in `src/`.
- `src/main.cpp` is the PlatformIO entry point.

## Current Scope

Milestone 1 is limited to CAN bring-up and safety/control behavior. Do not add fuzz strategies until:

- the project builds reliably,
- the fuzzer initializes CAN at 500 kbps,
- the sniffer observes known frames,
- SAFE boot and unauthorized start rejection are demonstrated.
