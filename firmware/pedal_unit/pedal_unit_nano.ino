/*
 * RhythmRide — Pedal Unit (Arduino Nano — NO WiFi needed)
 * =========================================================
 * Board: "Arduino Nano" | Processor: ATmega328P (Old Bootloader if upload fails)
 *
 * Outputs line-delimited JSON to Serial (115200 baud).
 * Browser on laptop reads via WebSerial API (Chrome/Edge only).
 *
 * WIRING:
 *   Throttle pot wiper → A0
 *   Brake    pot wiper → A1
 *   Pot GND  → GND    Pot VCC → 5V
 *   MPU-6050 SDA → A4   SCL → A5   VCC → 3.3V (or 5V)   GND → GND
 *   WS2812  data → D6   (via 300Ω resistor)   5V rail → separate 5V supply
 *   SSD1306 OLED → A4/A5 (same I2C bus as MPU)   addr 0x3C
 *   Extra button → D2 (tap tempo / extra trigger)
 *
 * LIBRARIES (Library Manager):
 *   FastLED
 *   MPU6050_light  (rfetick)
 *   Adafruit SSD1306
 *   Adafruit GFX Library
 *
 * PROTOCOL (one JSON per line, 115200 baud):
 *   {"t":"hello","role":"pedal","ver":1}
 *   {"t":"throttle","v":0.45}
 *   {"t":"brake","v":0.10}
 *   {"t":"stomp","m":2.8}
 *   {"t":"jump","m":3.1}
 *   {"t":"stomps","n":10}
 *   {"t":"btn","i":8,"s":1}   (i=8 = extra D2 button)
 *
 * Receives (from browser via Serial):
 *   {"t":"bpm","v":120}
 *   {"t":"score","v":1500}
 *   {"t":"hit"}
 */

#include <Wire.h>
#include <MPU6050_light.h>
#include <FastLED.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ─── Pin definitions ─────────────────────────────────────────────────────────
#define PIN_THROTTLE  A0
#define PIN_BRAKE     A1
#define PIN_LED_DATA   6
#define PIN_EXTRA_BTN  2   // extra button for tap tempo / trigger

// ─── LED ─────────────────────────────────────────────────────────────────────
#define NUM_LEDS 30
CRGB leds[NUM_LEDS];

// ─── OLED ─────────────────────────────────────────────────────────────────────
Adafruit_SSD1306 display(128, 64, &Wire, -1);

// ─── MPU ─────────────────────────────────────────────────────────────────────
MPU6050 mpu(Wire);

// ─── State ───────────────────────────────────────────────────────────────────
float throttleSmooth = 0.0f, brakeSmooth = 0.0f;
float lastThrottle   = -1.0f, lastBrake   = -1.0f;

bool  inAir          = false;
int   totalStomps    = 0;
int   currentBpm     = 120;
int   currentScore   = 0;

unsigned long tPedal = 0, tImu = 0, tOled = 0, tBeat = 0;
unsigned long lastStomp = 0, lastJump = 0;

// LED flash
enum LedFx { FX_IDLE, FX_STOMP, FX_JUMP, FX_HIT };
LedFx ledFx = FX_IDLE;
unsigned long ledFxEnd = 0;
float breathPhase = 0.0f;

// Extra button debounce
bool btnRaw = false, btnCur = false, btnSent = false;
unsigned long btnT = 0;

// Serial receive buffer
char rxBuf[64];
uint8_t rxLen = 0;

// ─────────────────────────────────────────────────────────────────────────────
void setup() {
    Serial.begin(115200);
    delay(200);

    // Extra button
    pinMode(PIN_EXTRA_BTN, INPUT_PULLUP);

    // ADC
    // Nano uses 10-bit ADC (0-1023)

    // I2C
    Wire.begin();

    // OLED
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println(F("# OLED failed"));
    } else {
        display.clearDisplay();
        display.setTextColor(SSD1306_WHITE);
        display.setTextSize(1);
        display.setCursor(0, 0);
        display.println(F("RhythmRide"));
        display.println(F("Booting..."));
        display.display();
    }

    // MPU-6050
    byte err = mpu.begin();
    if (err) {
        Serial.print(F("# MPU error ")); Serial.println(err);
    } else {
        display.clearDisplay();
        display.setCursor(0, 0);
        display.println(F("IMU calibrate"));
        display.println(F("Keep still 3s"));
        display.display();
        mpu.calcOffsets();
    }

    // FastLED
    FastLED.addLeds<WS2812B, PIN_LED_DATA, GRB>(leds, NUM_LEDS);
    FastLED.setBrightness(60);
    fill_solid(leds, NUM_LEDS, CRGB::Black);
    FastLED.show();

    // Announce
    Serial.println(F("{\"t\":\"hello\",\"role\":\"pedal\",\"ver\":1}"));

    tBeat = millis();
}

// ─────────────────────────────────────────────────────────────────────────────
void loop() {
    readSerial();
    readPedals();
    readImu();
    readExtraBtn();
    updateLeds();
    updateOled();
}

// ─── Serial receive (commands from browser) ───────────────────────────────────
void readSerial() {
    while (Serial.available()) {
        char c = Serial.read();
        if (c == '\n' || c == '\r') {
            if (rxLen > 0) {
                rxBuf[rxLen] = 0;
                parseCommand(rxBuf);
                rxLen = 0;
            }
        } else if (rxLen < 63) {
            rxBuf[rxLen++] = c;
        }
    }
}

void parseCommand(const char* s) {
    // Minimal parse — check for key substrings
    if (strstr(s, "\"bpm\"")) {
        const char* p = strstr(s, "\"v\":");
        if (p) currentBpm = atoi(p + 4);
    } else if (strstr(s, "\"score\"")) {
        const char* p = strstr(s, "\"v\":");
        if (p) currentScore = atoi(p + 4);
    } else if (strstr(s, "\"hit\"")) {
        setLedFx(FX_HIT);
    }
}

// ─── Pedals ───────────────────────────────────────────────────────────────────
void readPedals() {
    if (millis() - tPedal < 50) return;
    tPedal = millis();

    // Nano: 10-bit ADC (0-1023)
    float t = smooth(analogRead(PIN_THROTTLE) / 1023.0f, throttleSmooth);
    float b = smooth(analogRead(PIN_BRAKE)    / 1023.0f, brakeSmooth);
    throttleSmooth = t; brakeSmooth = b;

    char buf[40];
    if (fabsf(t - lastThrottle) >= 0.02f) {
        lastThrottle = t;
        dtostrf(t, 4, 2, buf); // Arduino-safe float→str
        Serial.print(F("{\"t\":\"throttle\",\"v\":"));
        Serial.print(buf); Serial.println(F("}"));
    }
    if (fabsf(b - lastBrake) >= 0.02f) {
        lastBrake = b;
        dtostrf(b, 4, 2, buf);
        Serial.print(F("{\"t\":\"brake\",\"v\":"));
        Serial.print(buf); Serial.println(F("}"));
    }
}

float smooth(float newVal, float prev) {
    return 0.1f * newVal + 0.9f * prev;
}

// ─── IMU stomp/jump ──────────────────────────────────────────────────────────
void readImu() {
    if (millis() - tImu < 10) return;
    tImu = millis();

    mpu.update();
    float ax = mpu.getAccX(), ay = mpu.getAccY(), az = mpu.getAccZ();
    float mag = sqrt(ax*ax + ay*ay + az*az);

    if (mag < 0.4f) inAir = true;

    // STOMP
    if (!inAir && mag > 2.2f && (millis() - lastStomp) > 300) {
        lastStomp = millis();
        totalStomps++;
        setLedFx(FX_STOMP);

        char buf[40];
        dtostrf(mag, 4, 1, buf);
        Serial.print(F("{\"t\":\"stomp\",\"m\":"));
        Serial.print(buf); Serial.println(F("}"));

        if (totalStomps % 10 == 0) {
            Serial.print(F("{\"t\":\"stomps\",\"n\":"));
            Serial.print(totalStomps); Serial.println(F("}"));
        }
    }

    // JUMP landing
    if (inAir && mag > 1.8f && (millis() - lastJump) > 300) {
        lastJump = millis();
        inAir = false;
        setLedFx(FX_JUMP);

        char buf[40];
        dtostrf(mag, 4, 1, buf);
        Serial.print(F("{\"t\":\"jump\",\"m\":"));
        Serial.print(buf); Serial.println(F("}"));
    }
}

// ─── Extra button (D2) ────────────────────────────────────────────────────────
void readExtraBtn() {
    bool raw = (digitalRead(PIN_EXTRA_BTN) == LOW);
    if (raw != btnRaw) { btnRaw = raw; btnT = millis(); }
    if ((millis() - btnT) >= 50 && btnRaw != btnCur) btnCur = btnRaw;
    if (btnCur != btnSent) {
        btnSent = btnCur;
        Serial.print(F("{\"t\":\"btn\",\"i\":8,\"s\":"));
        Serial.print(btnCur ? 1 : 0); Serial.println(F("}"));
    }
}

// ─── LED ─────────────────────────────────────────────────────────────────────
void setLedFx(LedFx fx) {
    ledFx = fx;
    unsigned long now = millis();
    switch (fx) {
        case FX_STOMP: fill_solid(leds, NUM_LEDS, CRGB::White);   FastLED.show(); ledFxEnd = now + 60; break;
        case FX_JUMP:  fill_solid(leds, NUM_LEDS, CRGB::Magenta); FastLED.show(); ledFxEnd = now + 100; break;
        case FX_HIT:   fill_solid(leds, NUM_LEDS, CRGB::Green);   FastLED.show(); ledFxEnd = now + 80; break;
        default: break;
    }
}

void updateLeds() {
    unsigned long now = millis();
    if (ledFx != FX_IDLE && now >= ledFxEnd) ledFx = FX_IDLE;
    if (ledFx != FX_IDLE) return;

    // Breathing idle (cyan)
    breathPhase += 0.04f;
    if (breathPhase > 6.283f) breathPhase -= 6.283f;
    uint8_t bright = (uint8_t)(20 + 90 * (1.0f + sin(breathPhase)));

    // Beat pulse
    unsigned long bp = (currentBpm > 0) ? (60000UL / currentBpm) : 500;
    if (now - tBeat >= bp) tBeat = now;
    if (now - tBeat < 50) bright = min(255, (int)bright + 100);

    fill_solid(leds, NUM_LEDS, CHSV(128, 255, bright));
    FastLED.show();
}

// ─── OLED ─────────────────────────────────────────────────────────────────────
void updateOled() {
    if (millis() - tOled < 500) return;
    tOled = millis();

    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 0); display.println(F("** RhythmRide **"));
    display.setCursor(0, 12); display.print(F("BPM: ")); display.println(currentBpm);
    display.setCursor(0, 22); display.print(F("Score: ")); display.println(currentScore);
    display.setCursor(0, 32); display.print(F("Stomps: ")); display.println(totalStomps);
    float kcal = totalStomps * 0.12f;
    char kbuf[8]; dtostrf(kcal, 4, 1, kbuf);
    display.setCursor(0, 42); display.print(F("kcal: ")); display.println(kbuf);
    display.display();
}
