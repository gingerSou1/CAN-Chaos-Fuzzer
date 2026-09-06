# Sniffer Node

`sniffer.ino` is a minimal receive-only sketch for a second UNO R4 WiFi node so you can observe CAN traffic from the fuzzer.

## Wiring

- UNO R4 WiFi 3.3V -> SN65HVD230 VCC
- UNO R4 WiFi GND -> SN65HVD230 GND
- UNO R4 WiFi D2 / CAN TX -> SN65HVD230 TXD / D
- UNO R4 WiFi D3 / CAN RX -> SN65HVD230 RXD / R
- SN65HVD230 RS -> GND for high-speed mode
- Bus ends terminated with 120 ohm across CANH/CANL

Open Serial Monitor at 115200 baud. During the Milestone 1 known-frame demo, expect standard ID `0x123` and payloads beginning with `CA FE 00 01`.
