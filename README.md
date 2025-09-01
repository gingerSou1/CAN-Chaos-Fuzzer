# CAN Chaos Fuzzer (Arduino UNO R4 WiFi + SN65HVD230)

*A red‑team–oriented CAN fuzzing and chaos‑injection toolkit for embedded/avionics‑style networks using Arduino UNO R4 WiFi (RA4M1 CAN controller) + SN65HVD230 transceiver.*

> ⚠️ **For authorized lab testing only.** Do not connect to production vehicles, aircraft, or any real system without explicit written permission. Bench‑only, isolated bus with proper termination.

---

## ✈️ Aerospace/Defense Framing
This project prototypes adversarial bus conditions (malformed frames, arbitration abuse, timing jitter) on a contained **CAN** network to emulate attack/fault scenarios analogous to **CANaerospace / ARINC‑825‑style** concerns. The goal is **chaotic testing**: probe subsystem resilience and recovery behavior under hostile traffic using low‑cost hardware before high‑assurance validation.

---

## 🔧 Hardware
- **Arduino UNO R4 WiFi (ABX00087)** — Renesas **RA4M1** with integrated CAN controller
- **SN65HVD230** CAN transceiver (3.3V preferred)
- Breadboard & jumpers
- Two **120Ω** resistors for end‑of‑line termination (bench bus)
- Optional: USB‑CAN adapter (for PC capture) or a 2nd microcontroller node

### Wiring (UNO R4 WiFi ↔ SN65HVD230)
| UNO R4 WiFi | SN65HVD230 |
|-------------|------------|
| 3V3         | VCC (most SN65 boards are 3.3V only) |
| GND         | GND |
| D2 (CAN TX) | D (TXD) |
| D3 (CAN RX) | R (RXD) |
| —           | Rs (tie to GND for high‑speed; add resistor for slope control) |
| —           | Vref (usually NC) |
| CANH        | CANH (bus) |
| CANL        | CANL (bus) |

> Ensure **120Ω** termination across CANH–CANL at **both ends** of the bench bus.

---

## 📦 Repo Layout
```
can-chaos-fuzzer/
├─ README.md
├─ .gitignore
├─ LICENSE
├─ /src
│  ├─ can_fuzzer.ino          # main sketch + serial menu
│  └─ profiles.h              # fuzz profile struct (extensible)
├─ /docs
│  ├─ threat-model.md         # adversary, scope, assumptions
│  └─ aerospace-mapping.md    # how this maps to CANaerospace/ARINC‑825
├─ /tools
│  └─ capture_notes.md        # how to capture baseline PCAP/logs
├─ /hardware
│  └─ wiring-diagram.md       # quick notes / placeholders for diagram
└─ /examples
   └─ profile_samples.json    # example profiles (future SPIFFS/SD)
```

---

## 🧪 Quick Start
1. **Wire** UNO R4 WiFi ↔ SN65HVD230 as above. Build a **bench‑only** CAN bus with proper termination.  
2. Install Arduino libraries:  
   - **Arduino CAN** (Renesas/UNO R4 compatible) — search for `Arduino_CAN` or `Arduino CAN` in Library Manager.  
3. Open `src/can_fuzzer.ino`, select **Arduino UNO R4 WiFi**, and upload.  
4. Open Serial Monitor @ **115200** baud.  
5. Start in **SAFE** mode (default). Choose a profile and **arm** explicitly:
   - `h` to show menu  
   - `a` to ARM (enable transmit)  
   - `1` random fuzz, `2` mutate selected IDs, `3` DoS flood (guarded), `s` SAFE/disarm

---

## 🔴 Fuzz Profiles
- **Random** — Random IDs (11‑bit), random payloads (0–8 bytes), rate‑limited.  
- **Mutate** — Choose from `TARGET_IDS` and mutate payloads with optional bit‑flip bias.  
- **DoS Flood** — High‑priority ID saturation with adjustable interval (explicit opt‑in).

Extend via `profiles.h` and/or load JSON profiles in future (SPIFFS/SD).

---

## ✅ Bench Validation Checklist
- Termination checked (120Ω at both ends)  
- Baseline receive‑only capture saved (no fuzz)  
- Fuzz profile + rates documented  
- Observed effects, lockups, error counters  
- Recovery behavior (power cycle, bus‑off, auto‑recovery)  
- Aerospace mapping notes recorded (`/docs/aerospace-mapping.md`)

---

## ⚠️ Safety & Ethics
This is a **research/education** tool. Use only on **isolated lab rigs** you own or have written authorization to test. Never operate near live control systems. Follow organizational policy and law.

---

## 🗺️ Roadmap
- WiFi dashboard (start/stop, rate sliders, live logs)
- JSON‑defined profiles (SD/SPIFFS)
- Frame capture to SD card
- Anomaly counters (RX/TX error, bus‑off tracking)
- Targeted ECUs with condition‑based triggers

---

## 📜 License
MIT — see `LICENSE`.
