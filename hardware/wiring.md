# Wiring

Milestone 1 targets the Arduino UNO R4 WiFi with an SN65HVD230 or compatible 3.3 V CAN transceiver.

| UNO R4 WiFi | SN65HVD230 |
| --- | --- |
| 3V3 | VCC |
| GND | GND |
| D2 / CAN TX | D / TXD |
| D3 / CAN RX | R / RXD |
| GND | Rs, for high-speed mode |
| No connection | Vref, unless the module documentation requires otherwise |

Connect CANH and CANL from the transceiver to the isolated bench bus. Install one 120 ohm resistor across CANH and CANL at each physical end of the bus.

Use the same transceiver wiring for the second microcontroller when running `examples/sniffer.ino`.
