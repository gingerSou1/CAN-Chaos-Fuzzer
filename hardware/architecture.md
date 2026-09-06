# Hardware Architecture

Milestone 1 validates a two-node isolated CAN bench:

```text
Development PC
  |
  | USB serial
  |
UNO R4 WiFi fuzzer
  |
SN65HVD230 transceiver
  |
CANH/CANL isolated bench bus
  |
Sniffer node or USB-CAN monitor
```

The fuzzer starts in SAFE on every boot. It only transmits after the operator sends `arm` followed by `start`.

The target ECU simulator is intentionally deferred until the fuzzer foundation is stable.
