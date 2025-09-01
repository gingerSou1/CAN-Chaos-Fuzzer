# Aerospace/Defense Mapping

Although this project targets classic CAN, the **testing mindset** and many failure modes are relevant to avionics-style networks:

- **CANaerospace / ARINC-825**: deterministic messaging, ID discipline, timing windows.
- **Concerns mirrored**: arbitration abuse (low ID floods), malformed payload handling, timing jitter sensitivity.
- **Validation themes**: error confinement behavior, bus-off recovery, critical vs. non-critical traffic prioritization.

Use findings to inform higher-assurance lab tests and requirements hardening.
