/*
 * RhythmRide — Arduino Uno Controller
 * =====================================
 * Inputs:
 *   - Throttle pedal pot → A0
 *   - Brake pedal pot    → A1
 *   - Steering pot       → A2
 *   - Button 1           → D2
 *   - Button 2           → D4
 *
 * Sends JSON over USB Serial at 115200 baud.
 */

// ── Pins ──────────────────────────────────────────────────
#define PIN_THROTTLE A0
#define PIN_BRAKE    A1
#define PIN_STEER    A2
#define PIN_BTN1     2
#define PIN_BTN2     4

// ── Smoothing / sensitivity ───────────────────────────────
const float EMA            = 0.15f;
const float DELTA_THROTTLE = 0.015f;
const float DELTA_BRAKE    = 0.004f;
const float DELTA_STEER    = 0.006f;

// ── Stored values ─────────────────────────────────────────
float throttleSmooth = 0.0f;
float brakeSmooth    = 0.0f;
float steerSmooth    = 0.0f;

float lastThrottle = -1.0f;
float lastBrake    = -1.0f;
float lastSteer    = -1.0f;

int lastBtn1 = HIGH;
int lastBtn2 = HIGH;

unsigned long lastReadMs = 0;

// ── Send value as JSON ────────────────────────────────────
void sendValue(const char* type, float v) {
  char num[8];
  dtostrf(v, 4, 2, num);

  char* p = num;
  while (*p == ' ') p++;

  Serial.print("{\"t\":\"");
  Serial.print(type);
  Serial.print("\",\"v\":");
  Serial.print(p);
  Serial.println("}");
}

void setup() {
  Serial.begin(115200);

  pinMode(PIN_BTN1, INPUT_PULLUP);
  pinMode(PIN_BTN2, INPUT_PULLUP);

  Serial.println("{\"t\":\"hello\",\"role\":\"uno\"}");
}

void loop() {
  unsigned long now = millis();

  // Read analog inputs 25 times per second
  if (now - lastReadMs >= 40) {
    lastReadMs = now;

    float throttleRaw = analogRead(PIN_THROTTLE) / 1023.0f;
    float brakeRaw    = analogRead(PIN_BRAKE)    / 1023.0f;
    float steerRaw    = analogRead(PIN_STEER)    / 1023.0f;

    throttleSmooth = EMA * throttleRaw + (1.0f - EMA) * throttleSmooth;
    brakeSmooth    = EMA * brakeRaw    + (1.0f - EMA) * brakeSmooth;
    steerSmooth    = EMA * steerRaw    + (1.0f - EMA) * steerSmooth;

    if (fabsf(throttleSmooth - lastThrottle) >= DELTA_THROTTLE) {
      sendValue("throttle", throttleSmooth);
      lastThrottle = throttleSmooth;
    }

    if (fabsf(brakeSmooth - lastBrake) >= DELTA_BRAKE) {
      sendValue("brake", brakeSmooth);
      lastBrake = brakeSmooth;
    }

    if (fabsf(steerSmooth - lastSteer) >= DELTA_STEER) {
      sendValue("steer", steerSmooth);
      lastSteer = steerSmooth;
    }
  }

  // Button 1 → stomp / kick
  int b1 = digitalRead(PIN_BTN1);
  if (b1 != lastBtn1) {
    lastBtn1 = b1;

    if (b1 == LOW) {
      Serial.println("{\"t\":\"stomp\",\"m\":3.00}");
    }
  }

  // Button 2 → pad / snare
  int b2 = digitalRead(PIN_BTN2);
  if (b2 != lastBtn2) {
    lastBtn2 = b2;

    if (b2 == LOW) {
      Serial.println("{\"t\":\"btn\",\"i\":1,\"s\":1}");
    } else {
      Serial.println("{\"t\":\"btn\",\"i\":1,\"s\":0}");
    }
  }

  // Ignore messages from the web app
  while (Serial.available()) {
    Serial.read();
  }
}
