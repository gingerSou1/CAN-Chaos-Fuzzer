#include <Arduino.h>
#include <CAN.h>   // Install Arduino CAN (UNO R4 WiFi / Renesas compatible)

// ----------------- Safety & State -----------------
static bool ARMED = false;           // Transmit disabled by default
static uint8_t activeProfile = 0;    // 0=off, 1=random, 2=mutate, 3=flood

// ----------------- Timing (rate limits) -----------------
static unsigned long lastTx = 0;
static unsigned long txIntervalMs = 20;     // ~50 msg/sec default
static unsigned long floodIntervalMs = 2;   // aggressive when enabled

// ----------------- Targets for mutate profile -----------------
static const uint32_t TARGET_IDS[] = {0x100, 0x200, 0x321};
static const size_t   TARGET_COUNT = sizeof(TARGET_IDS)/sizeof(TARGET_IDS[0]);

// ----------------- Helpers -----------------
static inline uint8_t randByte() { return (uint8_t)random(0,256); }

static void printMenu() {
  Serial.println(F("\n=== CAN Chaos Fuzzer (UNO R4 WiFi + SN65HVD230) ==="));
  Serial.println(F("[a] Arm (enable transmission)"));
  Serial.println(F("[s] Safe (disarm / stop)"));
  Serial.println(F("[1] Profile: Random IDs & payloads"));
  Serial.println(F("[2] Profile: Targeted ID mutation"));
  Serial.println(F("[3] Profile: DoS flood (aggressive)"));
  Serial.println(F("[r] Set rate (ms between tx)"));
  Serial.println(F("[f] Set flood rate (ms between tx)"));
  Serial.println(F("[i] Info / status"));
  Serial.println(F("[h] Help / menu"));
}

static void handleSerial() {
  if (!Serial.available()) return;
  char c = Serial.read();
  switch (c) {
    case 'a':
      ARMED = true; Serial.println(F(">> ARMED: transmission enabled")); break;
    case 's':
      ARMED = false; activeProfile = 0; Serial.println(F(">> SAFE: transmission disabled")); break;
    case '1':
      activeProfile = 1; Serial.println(F(">> Profile: Random")); break;
    case '2':
      activeProfile = 2; Serial.println(F(">> Profile: Mutate (targeted IDs)")); break;
    case '3':
      activeProfile = 3; Serial.println(F(">> Profile: DoS flood (use responsibly)")); break;
    case 'r': {
      Serial.println(F("Enter tx interval ms (e.g., 20):"));
      while (!Serial.available()) {}
      unsigned long v = Serial.parseInt();
      if (v > 0) txIntervalMs = v;
      Serial.print(F(">> txIntervalMs=")); Serial.println(txIntervalMs);
      break;
    }
    case 'f': {
      Serial.println(F("Enter flood interval ms (e.g., 2):"));
      while (!Serial.available()) {}
      unsigned long v = Serial.parseInt();
      if (v > 0) floodIntervalMs = v;
      Serial.print(F(">> floodIntervalMs=")); Serial.println(floodIntervalMs);
      break;
    }
    case 'i':
      Serial.print(F("ARMED=")); Serial.print(ARMED);
      Serial.print(F(" profile=")); Serial.print(activeProfile);
      Serial.print(F(" txIntervalMs=")); Serial.print(txIntervalMs);
      Serial.print(F(" floodIntervalMs=")); Serial.println(floodIntervalMs);
      break;
    case 'h':
    default:
      printMenu(); break;
  }
}

static void sendFrame(uint32_t id, const uint8_t* payload, uint8_t len, bool extended=false) {
  if (extended) CAN.beginExtendedPacket(id);
  else CAN.beginPacket(id);
  for (uint8_t i=0;i<len;i++) CAN.write(payload[i]);
  CAN.endPacket();
}

static void randomFrame() {
  uint32_t id = (uint32_t)random(0x000, 0x7FF); // 11-bit ID
  uint8_t len = (uint8_t)random(0, 9);
  uint8_t data[8];
  for (uint8_t i=0;i<len;i++) data[i] = randByte();
  sendFrame(id, data, len, false);
}

static void mutateFrame() {
  uint32_t base = TARGET_IDS[random(0, TARGET_COUNT)];
  uint8_t len = 8;
  uint8_t data[8];
  for (uint8_t i=0;i<len;i++) data[i] = randByte();
  // occasional extreme bias
  if (random(0,100) < 25) data[random(0,8)] = (random(0,2) ? 0x00 : 0xFF);
  sendFrame(base, data, len, false);
}

static void floodFrame() {
  uint32_t id = 0x001; // High-priority (wins arbitration)
  uint8_t data[8] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
  sendFrame(id, data, 8, false);
}

void setup() {
  Serial.begin(115200);
  delay(300);
  randomSeed(analogRead(A0));
  Serial.println(F("Initializing CAN @ 500 kbps..."));
  if (!CAN.begin(500E3)) {
    Serial.println(F("Starting CAN failed!"));
    while (1) { delay(1000); }
  }
  Serial.println(F("CAN init OK."));
  printMenu();
  Serial.println(F("STARTUP: SAFE (disarmed). Press [a] to arm, then choose a profile."));
}

void loop() {
  handleSerial();

  // Passive receive/logging when SAFE (optional basic RX check)
  if (!ARMED || activeProfile == 0) {
    int packetSize = CAN.parsePacket();
    if (packetSize) {
      // You can read and optionally log here
      while (CAN.available()) (void)CAN.read();
    }
    delay(5);
    return;
  }

  unsigned long now = millis();
  if (activeProfile == 1 && (now - lastTx) >= txIntervalMs) {
    randomFrame(); lastTx = now;
  } else if (activeProfile == 2 && (now - lastTx) >= txIntervalMs) {
    mutateFrame(); lastTx = now;
  } else if (activeProfile == 3 && (now - lastTx) >= floodIntervalMs) {
    floodFrame(); lastTx = now;
  }
}
