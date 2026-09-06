# Wiring Diagram Notes

The canonical Milestone 1 wiring instructions are in `hardware/wiring.md`.

Summary:

- UNO R4 WiFi 3V3 -> SN65HVD230 VCC
- UNO R4 WiFi GND -> SN65HVD230 GND
- UNO R4 WiFi D2 / CAN TX -> SN65HVD230 TXD / D
- UNO R4 WiFi D3 / CAN RX -> SN65HVD230 RXD / R
- SN65HVD230 RS -> GND for high-speed mode
- CANH/CANL connected to an isolated bench bus with 120 ohm termination at both ends

Add a rendered wiring diagram before publishing hardware screenshots.
