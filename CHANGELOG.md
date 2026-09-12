# Changelog

## Unreleased

- Add a bounded interactive serial console with a `CAN>` prompt, character echo,
  Backspace/Delete editing, CR/LF/CRLF handling and a `menu` alias for `help`.
  Prompts and echo share the existing output queue; dropped console fragments
  contribute to `LOG_DROPPED_LINES`. Scripted clients must account for echo and
  prompts, and CR now terminates a command instead of acting as token whitespace.
- Refocus README on current capabilities and project direction; retain detailed
  experiment sketches in `docs/experiment-design.md` and update architecture/CLI docs.
- Add 13 console regressions while retaining the 32 foundation/fuzz tests.

- Add Milestone 2A deterministic known-ID payload fuzzing with explicit xorshift32
  seed semantics and random, bitflip, zero, ff, boundary and walkingbit strategies.
- Add `fuzz start <strategy> <seed> <count> <interval_ms>`, bounded experiment
  scheduling and per-run reproducibility status through the existing safety,
  driver and logger boundaries; preserve the original known-frame demo.
- Add 14 host regressions (32 total) and document deterministic ordering, duration
  limits and future fuzz hardware acceptance, which has not been performed.

- Complete Milestone 1C hardware acceptance: record the healthy baseline, deliberate
  CAN write-failure fault (`-60003`), latched fault after reconnect, and SAFE recovery
  by reboot. Asynchronous controller-error handling remains host-test validated;
  it was not observed during this bench fault. See `docs/milestone-1c-validation.md`.

- Add coding, file-header, API-documentation, security reporting and release-tag
  conventions, editor/formatter settings and a pull request template.
- Document public safety/control contracts and label project-owned source licenses.
- Use the portable compiler's executable aliases directly instead of generating
  command-shell wrappers for host builds.

- Establish and pin the UNO R4 WiFi PlatformIO/Arduino build baseline; add a
  separately filtered sniffer environment.
- Verify Arduino_CAN 1.6.0 core write semantics (1 is success), expose error state
  and accepted-write counters, and preserve extended RX format.
- Harden command boundaries, whitespace, integer overflow and argument counts.
- Gate writes by safety state, latch runtime faults, cancel experiments on loss
  of permission and consistently reset non-faulted counters.
- Bound serial/RX/log work and restrict the known-frame demo to low-rate intervals.
- Add native control/safety tests and document remaining hardware acceptance.

- Convert repository to a PlatformIO-based Milestone 1 firmware layout.
- Add CAN driver abstraction for the UNO R4 WiFi native CAN controller.
- Add SAFE/ARMED/RUNNING/FAULT safety state handling.
- Add USB serial command interface for CAN bring-up and known-frame TX testing.
- Update the sniffer sketch to use the UNO R4 `Arduino_CAN` API.
