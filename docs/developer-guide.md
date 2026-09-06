# Developer Guide

## Build

Install PlatformIO, then run:

```bash
pio run
```

The active environment is `uno_r4_wifi`.

### Reproducible build baseline (Milestone 1A)

The checked-in environments pin Renesas RA platform 1.9.0, Arduino UNO
framework 1.6.0, ARM toolchain package 1.70201.0 (GCC 7.2.1), and bossac
1.10901.0. Arduino_CAN 1.0 is bundled with that framework; do not install an
unrelated CAN library. The baseline was established with PlatformIO Core 6.1.19.

```bash
pio run -e uno_r4_wifi -t clean
pio run -e uno_r4_wifi
pio run -e sniffer
```

`pio run` defaults only to the main firmware. `sniffer` compiles a wrapper around
the existing `examples/sniffer.ino`; its source filter excludes the main firmware.
No file copying or renaming is needed. Build/upload the second board explicitly
with `-e sniffer` and its own upload port; see `examples/README.md`.

### Verified Arduino_CAN contracts

Inspection of the selected framework's
`libraries/Arduino_CAN/src/R7FA4M1_CAN.{h,cpp}` establishes:

- `begin()` returns a boolean initialization result.
- `write()` returns **1** when `R_CAN_Write` returns `FSP_SUCCESS`; failures return
  the negative FSP error. The original `rc == 1` check was correct and is retained.
- Successful writes mean controller acceptance, not confirmed on-bus delivery;
  the wrapper ignores the TX-complete callback. Status therefore says `TX_ACCEPTED`.
- `available()` reports queued RX messages; `read()` dequeues one. The firmware
  checks availability before reading.
- `isError(int&)` exposes a latched callback error/event code. This milestone
  conservatively faults on any reported event or failed hardware write, without
  automatic recovery or retries. RX loss and bus-recovery events are included in
  the library's error latch; `ERROR` does not specifically mean bus-off.
- RX preserves standard/extended ID format. TX remains standard data frames only.
  RTR/CAN FD support is not claimed.

FAULT is latched until board reboot. `reset` resets control/counters only when
not faulted; it does not clear a hardware fault or erase its evidence. Calling
the startup initialization methods again cannot recover a faulted controller.

### Host tests

```bash
pio test -e host
```

The native environment compiles the production control, driver, logger and main
loop sources against small Arduino/Arduino_CAN boundary doubles. Tests execute
without a board and do not transmit CAN traffic. They validate policy and API
usage, not peripheral interrupt behavior or electrical characteristics.

A native GCC-compatible C++17 compiler must be on PATH. On Windows without one,
extract the official [LLVM-MinGW 20250709 release](https://github.com/mstorsjo/llvm-mingw/releases/tag/20250709)
`llvm-mingw-20250709-ucrt-x86_64.zip` into `.pio/host-tools/`, retaining its top-level
directory, then run `pio test -e host`. `tools/host_compiler.py` selects that
compiler when present and links the test runtime statically; it does not download
tools automatically. Native platform 1.2.1 and Unity 2.6.1 are pinned separately
from the firmware.

### Scheduling and safety limits (Milestone 1B)

- At most 32 input bytes and one complete command line are processed per loop.
- At most four CAN RX frames are consumed per loop.
- Log lines enter a fixed-size buffer. At most 32 output bytes are written per
  loop; excess whole lines are dropped and counted. A full log buffer does not
  prevent a command from taking effect, though its response may be dropped.
- The only experiment is the known-frame demo, with a minimum 1000 ms interval,
  bounded duration, and no catch-up bursts. A driver-level one-second spacing
  limit survives STOP/START and counter reset; repeated commands cannot bypass it.
  `TX_RATE_LIMITED` counts software attempts withheld by this limit.
- The CAN driver checks safety at the actual send boundary. SAFE, ARMED and FAULT
  prohibit new application-frame writes.
- Controller faults immediately prohibit sends and make an experiment externally
  inactive; its termination counter is reconciled by stop/update on the same
  cooperative loop. A fault during a send is reconciled before update returns.

The pinned UNO R4 WiFi core defines `NO_USB` and routes `Serial` to its UART
bridge. `UART::write()` synchronously waits for an interrupt and does not override
`availableForWrite()`. The log budget bounds normal output time (about 2.8 ms for
32 bytes at 115200 baud), but cannot guarantee progress if the UART/interrupt
implementation itself hangs. No serial routing, pins, or core internals are changed.

STOP is processed in stream order; a command behind earlier input must wait for
that input to be consumed. Software cancellation prevents subsequent writes; it
does not retract a frame already accepted by the controller. CAN acknowledgments
and error signaling are also distinct from application-frame transmission.
This is not a hardware silent/listen-only or emergency-stop implementation.

### Hardware acceptance still required (not performed in 1A/1B)

On an isolated bench, independently verify:

1. Transceiver supply/logic compatibility, wiring and termination against actual
   module/board documentation; no wiring changes were inferred from software.
2. Boot is SAFE, ARM alone produces no application frames, and START in SAFE is
   rejected. Validate reboot and USB bridge reconnect behavior.
3. `arm`, then `start 5000 1000`, produces the documented ID/DLC/payload/sequence
   and rate on an independent observer. Acceptance counters are not delivery proof.
4. Standard and extended RX frames with the same numeric ID are distinguished.
5. Measure STOP/DISARM/timeout latency with RX traffic and slow/disconnected host
   logging; account for an already accepted CAN frame and UART driver waits.
6. Verify initialization failure, reported controller faults and failed writes
   latch FAULT, cancel the run and block subsequent application writes. Confirm
   the selected core actually delivers the relevant callbacks on hardware.
7. Verify reboot is the only fault recovery path and successful `reset` clears
   displayed counters consistently. No automatic restart should occur on reconnect.

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
