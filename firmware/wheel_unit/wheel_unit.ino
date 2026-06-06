/*
 * RhythmRide — Wheel Unit (standard ESP32, NOT ESP32-S3)
 * =========================================================
 * Board: "ESP32 Dev Module" in Arduino IDE
 * Flash: 4MB, Default partition scheme
 *
 * CRITICAL PIN NOTES for standard ESP32:
 *   GPIO6-11  = internal SPI flash → NEVER USE (instant crash)
 *   GPIO4,2,0,12,13,14,15,25,26,27 = ADC2 (unusable for analog while WiFi on)
 *   ADC1 safe pins: GPIO32,33,34,35,36(VP),39(VN)
 *   GPIO34,35,36,39 = INPUT ONLY (no pullup/output)
 *
 * WIRING (steering pot — 3 wires):
 *   Pot GND   → ESP32 GND
 *   Pot WIPER → GPIO34
 *   Pot VCC   → ESP32 3.3V
 *
 * BUTTONS (wire each pin → button → GND, INPUT_PULLUP):
 *   Btn 0-7: GPIO4, GPIO5, GPIO13, GPIO14, GPIO15, GPIO16, GPIO17, GPIO18
 *   Paddle L: GPIO19
 *   Paddle R: GPIO23
 *
 * LIBRARIES — install via Arduino IDE Library Manager:
 *   1. arduinoWebSockets  (Markus Sattler)
 *   2. ArduinoJson v6     (Benoit Blanchon)
 *
 * FIRST TIME SETUP:
 *   1. Install ESP32 board package: File > Preferences > add
 *      https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
 *   2. Tools > Board > ESP32 Arduino > "ESP32 Dev Module"
 *   3. Tools > Upload Speed > 115200 if 921600 fails
 *   4. Open Serial Monitor at 115200 baud to see debug output
 */

#include <WiFi.h>
#include <WebSocketsServer.h>
#include <WebServer.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>

// ─── WiFi credentials ────────────────────────────────────────────────────────
const char* AP_SSID = "RhythmRide";
const char* AP_PASS = "beatdrop";

// ─── PINS (standard ESP32 safe) ──────────────────────────────────────────────
#define PIN_STEER       34    // ADC1 — steering pot wiper (INPUT ONLY pin)
const int BUTTONS[8] = {4, 5, 13, 14, 15, 16, 17, 18};
#define PIN_PADDLE_L    19
#define PIN_PADDLE_R    23

// ─── Servers ─────────────────────────────────────────────────────────────────
WebSocketsServer wsServer(81);
WebServer        httpServer(80);

// ─── State ───────────────────────────────────────────────────────────────────
float steerSmooth   = 0.5f;
float steerLastSent = -1.0f;
uint32_t tSteer = 0, tBtn = 0, tHb = 0;

struct Btn { bool cur, raw, sent; uint32_t t; };
Btn btn[8], paddleL, paddleR;

// ─────────────────────────────────────────────────────────────────────────────
void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println("\n=============================");
    Serial.println(" RhythmRide WHEEL UNIT v2.0");
    Serial.println("=============================");

    analogReadResolution(12);

    // Init button state
    for (int i = 0; i < 8; i++) {
        pinMode(BUTTONS[i], INPUT_PULLUP);
        btn[i] = {false, false, false, 0};
    }
    pinMode(PIN_PADDLE_L, INPUT_PULLUP);
    pinMode(PIN_PADDLE_R, INPUT_PULLUP);
    paddleL = paddleR = {false, false, false, 0};

    // Read initial steering value
    steerSmooth = analogRead(PIN_STEER) / 4095.0f;

    // WiFi AP
    Serial.println("[WiFi] Starting access point...");
    WiFi.mode(WIFI_AP);
    if (WiFi.softAP(AP_SSID, AP_PASS)) {
        Serial.print("[WiFi] AP OK  SSID='RhythmRide'  IP=");
        Serial.println(WiFi.softAPIP());
    } else {
        Serial.println("[WiFi] ERROR: AP failed to start!");
    }

    // SPIFFS (serves PWA files)
    if (SPIFFS.begin(true)) {
        Serial.println("[SPIFFS] Mounted.");
        File root = SPIFFS.open("/");
        File f = root.openNextFile();
        if (!f) Serial.println("[SPIFFS] Empty — upload index.html via SPIFFS uploader");
        while (f) { Serial.print("[SPIFFS] "); Serial.println(f.name()); f = root.openNextFile(); }
    } else {
        Serial.println("[SPIFFS] Mount failed.");
    }

    // HTTP — serves PWA
    httpServer.on("/", []() {
        if (SPIFFS.exists("/index.html")) {
            File f = SPIFFS.open("/index.html", "r");
            httpServer.streamFile(f, "text/html");
            f.close();
        } else {
            // Fallback: redirect to a data URL that just opens WebSocket
            httpServer.send(200, "text/html",
                "<html><head><meta name='viewport' content='width=device-width'></head>"
                "<body style='background:#000;color:#0ff;font-family:monospace;text-align:center;padding:40px'>"
                "<h1>RhythmRide</h1>"
                "<p>Hardware connected!</p>"
                "<p>Upload index.html via SPIFFS for full app.</p>"
                "<p id=ws>Connecting WebSocket...</p>"
                "<script>"
                "var ws=new WebSocket('ws://192.168.4.1:81');"
                "ws.onopen=function(){document.getElementById('ws').textContent='WS CONNECTED';};"
                "ws.onmessage=function(e){console.log(e.data);};"
                "</script></body></html>");
        }
    });
    httpServer.onNotFound([]() {
        String uri = httpServer.uri();
        if (SPIFFS.exists(uri)) {
            String mime = "text/plain";
            if (uri.endsWith(".html")) mime = "text/html";
            else if (uri.endsWith(".js")) mime = "application/javascript";
            else if (uri.endsWith(".css")) mime = "text/css";
            else if (uri.endsWith(".json")) mime = "application/json";
            File f = SPIFFS.open(uri, "r");
            httpServer.streamFile(f, mime);
            f.close();
        } else {
            httpServer.send(404, "text/plain", "Not found: " + uri);
        }
    });
    httpServer.begin();
    Serial.println("[HTTP] Server on port 80.");

    // WebSocket
    wsServer.begin();
    wsServer.onEvent(wsEvent);
    Serial.println("[WS] Server on port 81.");
    Serial.println("\nReady. Phone: connect WiFi 'RhythmRide', open http://192.168.4.1");
}

// ─────────────────────────────────────────────────────────────────────────────
void loop() {
    wsServer.loop();
    httpServer.handleClient();
    readSteering();
    readButtons();

    if (millis() - tHb > 1000) {
        tHb = millis();
        char hb[40];
        snprintf(hb, sizeof(hb), "{\"t\":\"hb\",\"ms\":%lu}", millis());
        wsServer.broadcastTXT(hb);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
void wsEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t len) {
    if (type == WStype_CONNECTED) {
        Serial.printf("[WS] Client #%d connected from %s\n", num,
                      wsServer.remoteIP(num).toString().c_str());
        wsServer.sendTXT(num, "{\"t\":\"hello\",\"role\":\"wheel\"}");

    } else if (type == WStype_DISCONNECTED) {
        Serial.printf("[WS] Client #%d disconnected\n", num);

    } else if (type == WStype_TEXT) {
        // Relay pedal-unit messages to all clients (including phone)
        const char* msg = (const char*)payload;
        if (strstr(msg, "\"stomp\"") || strstr(msg, "\"jump\"") ||
            strstr(msg, "\"throttle\"") || strstr(msg, "\"brake\"") ||
            strstr(msg, "\"stomps\"") || strstr(msg, "\"hello\"") ||
            strstr(msg, "\"bpm\"") || strstr(msg, "\"score\"")) {
            wsServer.broadcastTXT(payload, len);
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
void readSteering() {
    if (millis() - tSteer < 20) return;
    tSteer = millis();

    int raw = analogRead(PIN_STEER);
    float v = raw / 4095.0f;
    steerSmooth = 0.15f * v + 0.85f * steerSmooth;

    float delta = steerSmooth - steerLastSent;
    if (delta < 0) delta = -delta;
    if (delta > 0.008f) {
        steerLastSent = steerSmooth;
        char buf[40];
        snprintf(buf, sizeof(buf), "{\"t\":\"steer\",\"v\":%.3f}", steerSmooth);
        wsServer.broadcastTXT(buf);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
void debounce(Btn& b, bool rawVal, uint8_t idx, bool isPaddle) {
    uint32_t now = millis();
    if (rawVal != b.raw) { b.raw = rawVal; b.t = now; }
    if ((now - b.t) >= 50 && b.raw != b.cur) b.cur = b.raw;
    if (b.cur != b.sent) {
        b.sent = b.cur;
        char buf[48];
        if (isPaddle)
            snprintf(buf, sizeof(buf), "{\"t\":\"paddle\",\"i\":%d,\"s\":%d}", idx, b.cur ? 1 : 0);
        else
            snprintf(buf, sizeof(buf), "{\"t\":\"btn\",\"i\":%d,\"s\":%d}", idx, b.cur ? 1 : 0);
        wsServer.broadcastTXT(buf);
        Serial.println(buf);
    }
}

void readButtons() {
    if (millis() - tBtn < 30) return;
    tBtn = millis();
    for (int i = 0; i < 8; i++)
        debounce(btn[i], digitalRead(BUTTONS[i]) == LOW, i, false);
    debounce(paddleL, digitalRead(PIN_PADDLE_L) == LOW, 0, true);
    debounce(paddleR, digitalRead(PIN_PADDLE_R) == LOW, 1, true);
}
