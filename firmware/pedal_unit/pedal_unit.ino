/*
 * RhythmRide — Pedal Controller Unit (ESP32-S3, Unit B)
 *
 * BOM:
 *   - 1x ESP32-S3 Dev Module
 *   - 1x MPU-6050 6-axis IMU breakout
 *   - 1x SSD1306 OLED 128x64 (I2C, addr 0x3C)
 *   - 1x WS2812B LED strip, 30 LEDs
 *   - 1x L298N dual H-bridge motor driver
 *   - 2x TT yellow gear motors
 *   - 2x potentiometers (throttle + brake pedals)
 *   - 1x 300Ω resistor (WS2812 data line)
 *   - 1x 1000µF capacitor (WS2812 5V rail)
 *
 * Wiring:
 *   Throttle pot wiper  → GPIO4  (or via Nano 1 TX→GPIO17 Serial1)
 *   Brake pot wiper     → GPIO5  (or via Nano 2 TX→GPIO18 Serial2)
 *   Nano 1 TX (5V→3.3V divider) → GPIO17 (Serial1 RX)
 *   Nano 2 TX (5V→3.3V divider) → GPIO18 (Serial2 RX)
 *   MPU-6050 SDA        → GPIO8
 *   MPU-6050 SCL        → GPIO9
 *   MPU-6050 VCC        → 3.3V
 *   MPU-6050 GND        → GND
 *   MPU-6050 AD0        → GND  (I2C addr 0x68)
 *   WS2812 data         → GPIO10 (via 300Ω resistor)
 *   WS2812 5V           → external 5V rail
 *   OLED SDA            → GPIO8  (shared I2C bus)
 *   OLED SCL            → GPIO9
 *   OLED addr           → 0x3C
 *   L298N IN1           → GPIO11
 *   L298N IN2           → GPIO12
 *   L298N IN3           → GPIO13
 *   L298N IN4           → GPIO14
 *   L298N ENA (PWM)     → GPIO15
 *   L298N ENB (PWM)     → GPIO16
 *
 * Libraries (install via Arduino IDE Library Manager):
 *   FastLED
 *   MPU6050_light  (by rfetick)
 *   Adafruit SSD1306
 *   Adafruit GFX Library
 *   WebSockets  (arduinoWebSockets by Markus Sattler)
 *   ArduinoJson v6
 */

// ─── Library includes ────────────────────────────────────────────────────────
#include <Arduino.h>
#include <WiFi.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <MPU6050_light.h>
#include <FastLED.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ─── WiFi / WebSocket ─────────────────────────────────────────────────────────
static const char* WIFI_SSID     = "RhythmRide";
static const char* WIFI_PASS     = "beatdrop";
static const char* WS_HOST       = "192.168.4.1";
static const uint16_t WS_PORT    = 81;

// ─── Pin definitions ─────────────────────────────────────────────────────────
static const int PIN_THROTTLE    = 4;   // direct pot (unused if using Nanos)
static const int PIN_BRAKE       = 5;   // direct pot (unused if using Nanos)

// Nano serial bridges
static const int PIN_NANO1_RX    = 17;  // Nano 1 TX → voltage divider → here
static const int PIN_NANO2_RX    = 18;  // Nano 2 TX → voltage divider → here
#define USE_NANO_PEDALS true            // set false to use direct pots instead
static const int PIN_I2C_SDA     = 8;
static const int PIN_I2C_SCL     = 9;
static const int PIN_LED_DATA    = 10;
static const int PIN_IN1         = 11;
static const int PIN_IN2         = 12;
static const int PIN_IN3         = 13;
static const int PIN_IN4         = 14;
static const int PIN_ENA         = 15;
static const int PIN_ENB         = 16;

// ─── LED ─────────────────────────────────────────────────────────────────────
static const int NUM_LEDS        = 30;
static CRGB leds[NUM_LEDS];

// ─── OLED ─────────────────────────────────────────────────────────────────────
static const int OLED_WIDTH      = 128;
static const int OLED_HEIGHT     = 64;
static const int OLED_ADDR       = 0x3C;
Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire, -1);

// ─── MPU-6050 ────────────────────────────────────────────────────────────────
MPU6050 mpu(Wire);

// ─── WebSocket client ────────────────────────────────────────────────────────
WebSocketsClient webSocket;

// ─── Application state ───────────────────────────────────────────────────────
// Pedals
static float throttleSmooth = 0.0f;
static float brakeSmooth    = 0.0f;
static float lastThrottle   = -1.0f;
static float lastBrake      = -1.0f;
static const float EMA_ALPHA = 0.1f;
static const float SEND_DELTA = 0.02f;
static unsigned long lastPedalSend = 0;

// IMU / gestures
static bool  inAir              = false;
static unsigned long lastStompTime = 0;
static unsigned long lastJumpTime  = 0;
static unsigned long lastImuRead   = 0;
static const unsigned long IMU_INTERVAL_MS = 10;
static const unsigned long STOMP_DEBOUNCE  = 300;
static const unsigned long JUMP_DEBOUNCE   = 300;
static const float STOMP_THRESHOLD_G = 2.2f;
static const float JUMP_THRESHOLD_G  = 1.8f;
static const float AIR_THRESHOLD_G   = 0.4f;
static int totalStomps = 0;

// Game data from server
static int currentBpm   = 120;
static int currentScore = 0;

// LED mode
enum LedMode { LED_IDLE, LED_STOMP, LED_JUMP, LED_HIT };
static LedMode ledMode             = LED_IDLE;
static unsigned long ledModeUntil  = 0;   // millis() when flash expires
static float breathPhase           = 0.0f;

// BPM beat / motor pulse
static unsigned long lastBeatTime   = 0;
static unsigned long lastMotorOff   = 0;
static bool motorPulsing            = false;

// OLED refresh
static unsigned long lastOledUpdate = 0;

// WiFi reconnect
static unsigned long lastWifiRetry  = 0;

// ─── Forward declarations ─────────────────────────────────────────────────────
void connectWifi();
void webSocketEvent(WStype_t type, uint8_t* payload, size_t length);
void sendJson(const char* jsonStr);
void handlePedals();
void handleNanoSerial();
void handleImu();
void handleLeds();
void handleMotors();
void handleOled();
void triggerFlash(LedMode mode);
void motorsForward(uint8_t speed);
void motorsStop();
float readNormalized(int pin, float& smooth);

// ─── setup() ─────────────────────────────────────────────────────────────────
void setup() {
    Serial.begin(115200);
    delay(200);
    Serial.println("[RhythmRide] Pedal Unit booting...");

    // Motor driver pins
    pinMode(PIN_IN1, OUTPUT);
    pinMode(PIN_IN2, OUTPUT);
    pinMode(PIN_IN3, OUTPUT);
    pinMode(PIN_IN4, OUTPUT);
    pinMode(PIN_ENA, OUTPUT);
    pinMode(PIN_ENB, OUTPUT);
    motorsStop();

    // ADC resolution
    analogReadResolution(12);

    // Nano serial bridges (9600 baud, RX-only)
    Serial1.begin(9600, SERIAL_8N1, PIN_NANO1_RX, -1);
    Serial2.begin(9600, SERIAL_8N1, PIN_NANO2_RX, -1);

    // I2C
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);

    // OLED init
    if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
        Serial.println("[OLED] Init failed — check wiring/address");
    } else {
        display.clearDisplay();
        display.setTextSize(1);
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(0, 0);
        display.println("RhythmRide");
        display.println("Booting...");
        display.display();
    }

    // MPU-6050 init
    byte mpuStatus = mpu.begin();
    if (mpuStatus != 0) {
        Serial.print("[MPU] Init error code: ");
        Serial.println(mpuStatus);
    } else {
        Serial.println("[MPU] Keep still — Calibrating (3s)...");
        display.clearDisplay();
        display.setCursor(0, 0);
        display.println("Calibrating IMU");
        display.println("Keep still...");
        display.display();
        mpu.calcOffsets();   // ~3 second blocking calibration
        Serial.println("[MPU] Calibration done.");
    }

    // FastLED
    FastLED.addLeds<WS2812B, PIN_LED_DATA, GRB>(leds, NUM_LEDS);
    FastLED.setBrightness(80);
    fill_solid(leds, NUM_LEDS, CRGB::Black);
    FastLED.show();

    // WiFi
    connectWifi();

    // WebSocket
    webSocket.begin(WS_HOST, WS_PORT, "/");
    webSocket.onEvent(webSocketEvent);
    webSocket.setReconnectInterval(3000);

    lastBeatTime = millis();
    Serial.println("[RhythmRide] Setup complete.");
}

// ─── loop() ──────────────────────────────────────────────────────────────────
void loop() {
    unsigned long now = millis();

    // WiFi watchdog
    if (WiFi.status() != WL_CONNECTED && (now - lastWifiRetry) >= 5000) {
        Serial.println("[WiFi] Reconnecting...");
        WiFi.disconnect();
        WiFi.begin(WIFI_SSID, WIFI_PASS);
        lastWifiRetry = now;
    }

    webSocket.loop();

    if (USE_NANO_PEDALS) {
        handleNanoSerial();
    } else {
        handlePedals();
    }
    handleImu();
    handleLeds();
    handleMotors();
    handleOled();
}

// ─── WiFi connection ─────────────────────────────────────────────────────────
void connectWifi() {
    Serial.print("[WiFi] Connecting to ");
    Serial.println(WIFI_SSID);
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASS);

    // Block here with 5-second retry loop until connected
    while (WiFi.status() != WL_CONNECTED) {
        unsigned long attempt = millis();
        while (WiFi.status() != WL_CONNECTED && (millis() - attempt) < 5000) {
            delay(200);
            Serial.print(".");
        }
        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("\n[WiFi] Retry...");
            WiFi.disconnect();
            WiFi.begin(WIFI_SSID, WIFI_PASS);
        }
    }
    Serial.print("\n[WiFi] Connected! IP: ");
    Serial.println(WiFi.localIP());
}

// ─── WebSocket event handler ──────────────────────────────────────────────────
void webSocketEvent(WStype_t type, uint8_t* payload, size_t length) {
    switch (type) {
        case WStype_DISCONNECTED:
            Serial.println("[WS] Disconnected");
            break;

        case WStype_CONNECTED:
            Serial.println("[WS] Connected to server");
            // Send hello / role announcement
            sendJson("{\"t\":\"hello\",\"role\":\"pedal\"}");
            break;

        case WStype_TEXT: {
            // Parse incoming JSON
            StaticJsonDocument<256> doc;
            DeserializationError err = deserializeJson(doc, payload, length);
            if (err) {
                Serial.print("[WS] JSON parse error: ");
                Serial.println(err.c_str());
                break;
            }

            const char* msgType = doc["t"] | "";

            if (strcmp(msgType, "bpm") == 0) {
                int newBpm = doc["v"] | 120;
                if (newBpm > 0) {
                    currentBpm = newBpm;
                    Serial.print("[WS] BPM updated: ");
                    Serial.println(currentBpm);
                }

            } else if (strcmp(msgType, "score") == 0) {
                currentScore = doc["v"] | 0;
                Serial.print("[WS] Score updated: ");
                Serial.println(currentScore);

            } else if (strcmp(msgType, "hit") == 0) {
                Serial.println("[WS] HIT received — flash green");
                triggerFlash(LED_HIT);
            }
            break;
        }

        case WStype_ERROR:
            Serial.println("[WS] Error");
            break;

        default:
            break;
    }
}

// ─── Send a JSON string over WebSocket ───────────────────────────────────────
void sendJson(const char* jsonStr) {
    if (webSocket.isConnected()) {
        webSocket.sendTXT(jsonStr);
    }
}

// ─── ADC pedal reading + EMA smoothing ───────────────────────────────────────
float readNormalized(int pin, float& smooth) {
    int raw = analogRead(pin);
    float norm = raw / 4095.0f;
    smooth = EMA_ALPHA * norm + (1.0f - EMA_ALPHA) * smooth;
    return smooth;
}

void handlePedals() {
    unsigned long now = millis();
    if ((now - lastPedalSend) < 50) return;
    lastPedalSend = now;

    float t = readNormalized(PIN_THROTTLE, throttleSmooth);
    float b = readNormalized(PIN_BRAKE, brakeSmooth);

    char buf[64];

    if (fabsf(t - lastThrottle) >= SEND_DELTA) {
        snprintf(buf, sizeof(buf), "{\"t\":\"throttle\",\"v\":%.2f}", t);
        sendJson(buf);
        lastThrottle = t;
    }

    if (fabsf(b - lastBrake) >= SEND_DELTA) {
        snprintf(buf, sizeof(buf), "{\"t\":\"brake\",\"v\":%.2f}", b);
        sendJson(buf);
        lastBrake = b;
    }
}

// ─── Nano serial bridge — forward JSON lines from Nanos over WebSocket ───────
static char nanoBuf1[64];
static uint8_t nanoBuf1Len = 0;
static char nanoBuf2[64];
static uint8_t nanoBuf2Len = 0;

static void drainNanoSerial(HardwareSerial& port, char* buf, uint8_t& len) {
    while (port.available()) {
        char c = port.read();
        if (c == '\n') {
            if (len > 0) {
                buf[len] = '\0';
                sendJson(buf);
                len = 0;
            }
        } else if (len < 63) {
            buf[len++] = c;
        }
    }
}

void handleNanoSerial() {
    drainNanoSerial(Serial1, nanoBuf1, nanoBuf1Len);
    drainNanoSerial(Serial2, nanoBuf2, nanoBuf2Len);
}

// ─── IMU stomp / jump detection ──────────────────────────────────────────────
void handleImu() {
    unsigned long now = millis();
    if ((now - lastImuRead) < IMU_INTERVAL_MS) return;
    lastImuRead = now;

    mpu.update();

    float ax = mpu.getAccX();  // in g
    float ay = mpu.getAccY();
    float az = mpu.getAccZ();
    float mag = sqrtf(ax * ax + ay * ay + az * az);

    // Detect airborne phase (near-weightlessness)
    if (mag < AIR_THRESHOLD_G) {
        inAir = true;
    }

    // STOMP: sharp impact while feet are on the ground
    if (!inAir && mag > STOMP_THRESHOLD_G && (now - lastStompTime) > STOMP_DEBOUNCE) {
        lastStompTime = now;
        totalStomps++;

        char buf[64];
        snprintf(buf, sizeof(buf), "{\"t\":\"stomp\",\"m\":%.1f}", mag);
        sendJson(buf);
        Serial.print("[IMU] STOMP! mag=");
        Serial.println(mag);

        triggerFlash(LED_STOMP);

        // Every 10 stomps send cumulative count
        if (totalStomps % 10 == 0) {
            snprintf(buf, sizeof(buf), "{\"t\":\"stomps\",\"n\":%d}", totalStomps);
            sendJson(buf);
        }
    }

    // JUMP LANDING: was airborne, now impact spike
    if (inAir && mag > JUMP_THRESHOLD_G && (now - lastJumpTime) > JUMP_DEBOUNCE) {
        lastJumpTime = now;
        inAir = false;

        char buf[64];
        snprintf(buf, sizeof(buf), "{\"t\":\"jump\",\"m\":%.1f}", mag);
        sendJson(buf);
        Serial.print("[IMU] JUMP landing! mag=");
        Serial.println(mag);

        triggerFlash(LED_JUMP);
    }
}

// ─── LED management ───────────────────────────────────────────────────────────
void triggerFlash(LedMode mode) {
    ledMode = mode;
    unsigned long now = millis();

    switch (mode) {
        case LED_STOMP:
            fill_solid(leds, NUM_LEDS, CRGB::White);
            FastLED.show();
            ledModeUntil = now + 60;
            break;
        case LED_JUMP:
            fill_solid(leds, NUM_LEDS, CRGB::Magenta);
            FastLED.show();
            ledModeUntil = now + 100;
            break;
        case LED_HIT:
            fill_solid(leds, NUM_LEDS, CRGB::Green);
            FastLED.show();
            ledModeUntil = now + 80;
            break;
        default:
            break;
    }
}

void handleLeds() {
    unsigned long now = millis();

    // Check if flash period has expired
    if (ledMode != LED_IDLE && now >= ledModeUntil) {
        ledMode = LED_IDLE;
    }

    if (ledMode != LED_IDLE) {
        // Flash colours are already written in triggerFlash(); nothing to do until expiry
        return;
    }

    // ── Idle: slow breathing cyan (hue 128) ──────────────────────────────────
    // Advance breathing phase (~0.0015 radians/ms gives ~4s period)
    breathPhase += 0.004f;
    if (breathPhase > TWO_PI) breathPhase -= TWO_PI;

    // Map sine wave to brightness range 20-200
    uint8_t brightness = (uint8_t)(20.0f + 90.0f * (1.0f + sinf(breathPhase)));

    // BPM beat pulse: boost brightness by 100 for 50ms every beat
    unsigned long beatPeriod = (currentBpm > 0) ? (60000UL / currentBpm) : 500UL;
    unsigned long timeSinceBeat = now - lastBeatTime;
    if (timeSinceBeat < 50) {
        brightness = min(255, (int)brightness + 100);
    }

    // Update beat timer
    if (timeSinceBeat >= beatPeriod) {
        lastBeatTime = now;
    }

    fill_solid(leds, NUM_LEDS, CHSV(128, 255, brightness));
    FastLED.show();
}

// ─── Motor rhythm-wheel control ───────────────────────────────────────────────
void motorsForward(uint8_t speed) {
    // Motor A forward
    digitalWrite(PIN_IN1, HIGH);
    digitalWrite(PIN_IN2, LOW);
    analogWrite(PIN_ENA, speed);
    // Motor B forward
    digitalWrite(PIN_IN3, HIGH);
    digitalWrite(PIN_IN4, LOW);
    analogWrite(PIN_ENB, speed);
}

void motorsStop() {
    digitalWrite(PIN_IN1, LOW);
    digitalWrite(PIN_IN2, LOW);
    digitalWrite(PIN_IN3, LOW);
    digitalWrite(PIN_IN4, LOW);
    analogWrite(PIN_ENA, 0);
    analogWrite(PIN_ENB, 0);
}

void handleMotors() {
    unsigned long now = millis();
    unsigned long beatPeriod = (currentBpm > 0) ? (60000UL / currentBpm) : 500UL;
    unsigned long timeSinceBeat = now - lastBeatTime;

    if (motorPulsing) {
        // Turn motors off after 100ms pulse
        if ((now - lastMotorOff) >= 100) {
            motorsStop();
            motorPulsing = false;
        }
    } else {
        // Start a 100ms forward pulse at the beginning of each beat
        if (timeSinceBeat < 10 && (now - lastMotorOff) >= (beatPeriod - 20)) {
            motorsForward(128);
            lastMotorOff = now;
            motorPulsing = true;
        }
    }
}

// ─── OLED display ─────────────────────────────────────────────────────────────
void handleOled() {
    unsigned long now = millis();
    if ((now - lastOledUpdate) < 500) return;
    lastOledUpdate = now;

    float kcal = totalStomps * 0.12f;

    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);

    // Line 1 — header (larger text)
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println("** RhythmRide **");

    // Line 2 — BPM
    display.setCursor(0, 16);
    display.print("BPM:    ");
    display.println(currentBpm);

    // Line 3 — Score
    display.setCursor(0, 26);
    display.print("Score:  ");
    display.println(currentScore);

    // Line 4 — Stomps
    display.setCursor(0, 36);
    display.print("Stomps: ");
    display.println(totalStomps);

    // Line 5 — kcal
    display.setCursor(0, 46);
    display.print("kcal:   ");
    display.println(kcal, 2);

    display.display();
}
