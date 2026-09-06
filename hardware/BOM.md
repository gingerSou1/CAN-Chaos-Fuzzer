# Bill of Materials

Milestone 1 uses a narrow bench setup for CAN bring-up.

| Item | Quantity | Notes |
| --- | ---: | --- |
| Arduino UNO R4 WiFi | 1 | Fuzzer node, using the native RA4M1 CAN controller |
| SN65HVD230 CAN transceiver | 1 | 3.3 V CAN transceiver for the fuzzer |
| CAN sniffer node | 1 | Existing `examples/sniffer.ino` on a second UNO R4-compatible node, or a USB-CAN adapter |
| CAN transceiver for sniffer | 1 | Required if the sniffer node is another microcontroller |
| 120 ohm termination resistor | 2 | One across CANH/CANL at each end of the isolated bench bus |
| Breadboard and jumpers | As needed | Keep wiring short for initial bring-up |

Do not connect this hardware to production vehicles, aircraft, or active safety-critical systems.
