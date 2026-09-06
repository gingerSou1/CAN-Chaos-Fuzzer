# Changelog

## Unreleased

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
