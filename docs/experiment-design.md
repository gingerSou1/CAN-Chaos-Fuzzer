# Experiment Design

Milestone 1 has one experiment mode: a bounded known-frame transmission demo.

Purpose:

- Prove the PlatformIO toolchain.
- Prove UNO R4 native CAN initialization at 500 kbps.
- Prove the fuzzer can transmit standard CAN frames.
- Prove a sniffer can observe those frames on the isolated bench bus.
- Prove unsafe starts are rejected before transmission.

Acceptance demonstration:

1. Flash the fuzzer firmware.
2. Flash or connect the sniffer.
3. Confirm `status` reports `STATE: SAFE`.
4. Send `start` and confirm `ERROR: DEVICE NOT ARMED OR UNSAFE CONFIG`.
5. Send `arm` and confirm `STATE ARMED`.
6. Send `start 5000 1000`.
7. Confirm the sniffer observes ID `0x123` and payloads beginning with `CA FE 00 01`.
8. Send `stop` during a longer run and confirm transmission stops immediately.

Fuzzing, replay, load generation, and recovery windows start after this foundation is stable.
