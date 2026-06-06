/*
 * RhythmRide — Nano Brake Pedal
 *
 * Wiring:
 *   Pot GND   → GND
 *   Pot wiper → A0
 *   Pot VCC   → 5V
 *   Nano TX   → 10kΩ → ESP32-S3 B GPIO18 (RX2)
 *              junction → 20kΩ → GND
 *
 * Sends: {"t":"brake","v":0.65}\n  at 9600 baud
 */

#define POT_PIN     A0
#define MSG_TYPE    "brake"
#define SEND_INTERVAL_MS  50
#define EMA_ALPHA   0.1f
#define SEND_DELTA  0.02f

static float smooth    = 0.0f;
static float lastSent  = -1.0f;
static unsigned long lastSend = 0;

void setup() {
    Serial.begin(9600);
}

void loop() {
    unsigned long now = millis();
    if ((now - lastSend) < SEND_INTERVAL_MS) return;
    lastSend = now;

    float raw  = analogRead(POT_PIN) / 1023.0f;
    smooth     = EMA_ALPHA * raw + (1.0f - EMA_ALPHA) * smooth;

    if (fabsf(smooth - lastSent) >= SEND_DELTA) {
        Serial.print("{\"t\":\"");
        Serial.print(MSG_TYPE);
        Serial.print("\",\"v\":");
        Serial.print(smooth, 2);
        Serial.println("}");
        lastSent = smooth;
    }
}
