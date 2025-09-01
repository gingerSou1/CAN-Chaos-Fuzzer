# Sniffer Node (UNO R4 WiFi + SN65HVD230)

Minimal receive-only sketch for a second node so you can observe CAN traffic from the fuzzer.

## Wiring (same as fuzzer)
- UNO R4 WiFi **3.3V** → SN65HVD230 **VCC**
- UNO R4 WiFi **GND** → SN65HVD230 **GND**
- UNO R4 WiFi **D2 (CAN TX)** → SN65 **TXD (D)**
- UNO R4 WiFi **D3 (CAN RX)** → SN65 **RXD (R)**
- SN65 **RS** → **GND** (high-speed mode)
- Bus ends terminated with **120Ω** across CANH–CANL.

Open Serial Monitor at **115200** baud to see frames.
