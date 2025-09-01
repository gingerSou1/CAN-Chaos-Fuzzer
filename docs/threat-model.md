# Threat Model

## Scope (Bench-Only)
- Isolated CAN bus with test nodes under your control.
- No connection to production vehicles/aircraft or safety-critical rigs.

## Adversary
- Red-team operator attempting to disrupt or manipulate CAN traffic.
- Capabilities: inject, replay, flood, manipulate timing.

## Goals
- Surface robustness issues: lockups, bus-off, silent data corruption.
- Observe recovery behavior and error counters under stress.

## Assumptions
- Proper termination (120Ω ends), known bitrate, test harness available.
- Legal/authorized lab setting with safety procedures.
