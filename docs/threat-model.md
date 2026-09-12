# Threat Model

## Scope

- Isolated CAN bench networks with test nodes under the operator's control.
- No connection to production vehicles, aircraft, or safety-critical rigs.
- Milestone 1 is limited to CAN bring-up, serial control, SAFE boot behavior, and known-frame transmission.

## Adversary Model

The eventual fuzzer represents an authorized red-team or resilience-test operator attempting to disrupt or manipulate CAN traffic in a controlled lab.

Expected later capabilities include:

- injection,
- replay,
- load generation,
- arbitration pressure,
- payload mutation,
- timing manipulation.

## Goals

- Surface robustness issues such as lockups, bus-off behavior, silent data corruption, and unsafe state handling.
- Observe recovery behavior and error counters under controlled stress.
- Produce repeatable experiments with bounded duration and explicit operator control.

## Assumptions

The earlier README framed this work as post-bus-access resilience: a node may be
malfunctioning, compromised or deliberately adversarial in the isolated lab.
Questions include trust in CAN IDs, payload and timing assumptions, valid messages
in invalid states, priority abuse, stale replay, availability, detection and recovery.
Milestone 2A now supplies fixed-ID payload mutation; replay, load and arbitration
capabilities above remain future work. The project models no particular production
vehicle, aircraft or industrial installation.

- Proper termination: 120 ohm at both ends of the bench bus.
- Known bitrate: 500 kbps for Milestone 1.
- Legal and authorized lab setting with safety procedures.
- Fuzzer boots SAFE and cannot transmit until explicitly armed and started.
