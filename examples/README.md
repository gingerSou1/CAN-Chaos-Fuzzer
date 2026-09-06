# Sniffer Node

`sniffer.ino` is a minimal receive-only sketch for a second UNO R4 WiFi node so you can observe CAN traffic from the fuzzer.

Build it independently with PlatformIO:

```bash
pio run -e sniffer
```

To upload, explicitly select the **second board's** port:

```bash
pio run -e sniffer -t upload --upload-port COM_REPLACE_ME
pio device monitor -p COM_REPLACE_ME -b 115200
```

`src/sniffer_main.cpp` includes the existing sketch. Source filters ensure that
each target has only its own setup/loop; plain `pio run` still builds only
`uno_r4_wifi`. The sketch can also be used in an Arduino IDE sketch folder named
`sniffer`, with the matching Renesas UNO core installed.

Receive-only describes application behavior; the sketch does not configure
hardware listen-only mode and may participate in CAN acknowledgments/error signaling.

## Wiring

- UNO R4 WiFi 3.3V -> SN65HVD230 VCC
- UNO R4 WiFi GND -> SN65HVD230 GND
- UNO R4 WiFi D2 / CAN TX -> SN65HVD230 TXD / D
- UNO R4 WiFi D3 / CAN RX -> SN65HVD230 RXD / R
- SN65HVD230 RS -> GND for high-speed mode
- Bus ends terminated with 120 ohm across CANH/CANL

Open Serial Monitor at 115200 baud. During the Milestone 1 known-frame demo, expect standard ID `0x123` and payloads beginning with `CA FE 00 01`.
