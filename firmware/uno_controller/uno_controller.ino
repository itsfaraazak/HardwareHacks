/*
 * RhythmRide — Arduino Uno Controller
 * =====================================
 * Parts: 2 pedal pots (throttle + brake) + 2 push buttons.
 *
 * Talks to the web app over USB (Web Serial, Chrome/Edge).
 * Sends newline-terminated JSON at 115200 baud.
 *
 * WIRES:
 *   PEDAL CABLE (4 wires from Thrustmaster pedal connector):
 *     VCC wire (red/orange)   → 5V  (or + red rail if using breadboard)
 *     GND wire (black/brown)  → GND (or – blue rail)
 *     throttle wiper          → A0
 *     brake wiper             → A1
 *   BUTTON 1 — STOMP:    one leg → D2, other leg → GND
 *   BUTTON 2 — SNARE:    one leg → D4, other leg → GND
 *
 * WHAT IT SENDS:
 *   {"t":"throttle","v":0.42}  ← throttle pedal 0.00–1.00
 *   {"t":"brake","v":0.10}     ← brake pedal 0.00–1.00
 *   {"t":"stomp","m":3.00}     ← button 1 pressed
 *   {"t":"btn","i":1,"s":1}    ← button 2 pressed/released
 */

// ── Pins ──────────────────────────────────────────────────
#define PIN_THROTTLE A0
#define PIN_BRAKE    A1
#define PIN_STEER    A2    // steering wheel pot wiper → teal wire
#define PIN_BTN1     2     // stomp
#define PIN_BTN2     4     // snare

// ── How smooth / how often ────────────────────────────────
const float EMA           = 0.15f;  // smoothing (higher = snappier)
const float DELTA_THROTTLE= 0.015f; // throttle: send if moved this much
const float DELTA_BRAKE   = 0.004f; // brake: very sensitive (stiff spring)

// ── Memory of last values ─────────────────────────────────
float throttleSmooth = 0,  brakeSmooth = 0,  steerSmooth = 0;
float lastThrottle   = -1, lastBrake   = -1, lastSteer   = -1;
int   lastBtn1 = HIGH,     lastBtn2    = HIGH;
unsigned long lastPedalMs = 0;

// ── Send {"t":type,"v":number} — built by hand because the Uno's
//    printf can't print floats ──────────────────────────────
void sendValue(const char* type, float v) {
    char num[8];
    dtostrf(v, 4, 2, num);          // float → text, 2 decimals
    char* p = num;
    while (*p == ' ') p++;          // trim leading spaces
    Serial.print("{\"t\":\"");
    Serial.print(type);
    Serial.print("\",\"v\":");
    Serial.print(p);
    Serial.println("}");
}

void setup() {
    Serial.begin(115200);
    pinMode(PIN_BTN1, INPUT_PULLUP);   // no resistor needed
    pinMode(PIN_BTN2, INPUT_PULLUP);
    Serial.println("{\"t\":\"hello\",\"role\":\"uno\"}");
}

void loop() {
    // ── Pedals → throttle + brake (25 times a second) ────
    unsigned long now = millis();
    if (now - lastPedalMs >= 40) {
        lastPedalMs = now;

        float t = EMA * (analogRead(PIN_THROTTLE) / 1023.0f) + (1.0f - EMA) * throttleSmooth;
        float b = EMA * (analogRead(PIN_BRAKE)    / 1023.0f) + (1.0f - EMA) * brakeSmooth;
        float s = EMA * (analogRead(PIN_STEER)    / 1023.0f) + (1.0f - EMA) * steerSmooth;
        throttleSmooth = t; brakeSmooth = b; steerSmooth = s;

        if (fabsf(t - lastThrottle) >= DELTA_THROTTLE) { sendValue("throttle", t); lastThrottle = t; }
        if (fabsf(b - lastBrake)    >= DELTA_BRAKE)    { sendValue("brake",    b); lastBrake    = b; }
        if (fabsf(s - lastSteer)    >= DELTA_BRAKE)    { sendValue("steer",    s); lastSteer    = s; }
    }

    // ── Button 1 → stomp (kick drum) ──────────────────────
    int b1 = digitalRead(PIN_BTN1);
    if (b1 != lastBtn1) {
        lastBtn1 = b1;
        if (b1 == LOW) Serial.println("{\"t\":\"stomp\",\"m\":3.00}");
    }

    // ── Button 2 → pad (snare) ────────────────────────────
    int b2 = digitalRead(PIN_BTN2);
    if (b2 != lastBtn2) {
        lastBtn2 = b2;
        if (b2 == LOW) Serial.println("{\"t\":\"btn\",\"i\":1,\"s\":1}");
        else           Serial.println("{\"t\":\"btn\",\"i\":1,\"s\":0}");
    }

    // ── Ignore anything the web app sends back ────────────
    while (Serial.available()) Serial.read();
}
