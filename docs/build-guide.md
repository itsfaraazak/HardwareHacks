# RhythmRide Build Guide

## Phase 0 — Software demo (NOW, no hardware) — 5 min

1. In terminal:
   ```bash
   cd pwa && python3 -m http.server 8080
   ```
2. Open Chrome: `http://localhost:8080`
3. Click **JAM MODE**. Press SPACE to stomp, A–H for pads, ← → to steer.
4. Click **VR GAME MODE**. Allow camera. Notes fall. SPACE to hit them.

Done. This is a working demo right now.

---

## Phase 1 — Pedal board physical build — 45 min

### Materials
- ~400×300mm MDF or plywood (KCL offcut)
- 4× caster wheels (KCL workshop)
- Screws + drill
- Cable ties
- Blue tack or velcro

### Steps
1. Cut board to ~400×300mm if needed
2. Screw 4 caster wheels underneath — one per corner
3. Place pedal assembly on top, cable-tie to board
4. Velcro Arduino Nano breadboard under the board
5. Run DE-9 pedal cable into breadboard area
6. USB powerbank → Nano (or USB cable to laptop)
7. (Optional) WS2812 LED strip along front edge, face outward

---

## Phase 2 — Arduino Nano firmware — 30 min

### Setup
1. Install Arduino IDE: https://www.arduino.cc/en/software
2. Library Manager → install:
   - FastLED
   - MPU6050_light (by rfetick)
   - Adafruit SSD1306
   - Adafruit GFX Library
3. Open `firmware/pedal_unit/pedal_unit_nano.ino`
4. Board: Tools → Board → Arduino Nano
5. Processor: ATmega328P (Old Bootloader) — try this if upload fails
6. Port: select the COM/tty port for your Nano

### Wire per docs/wiring-pedal.md, then upload
7. Open Serial Monitor at 115200 baud
8. Should see: `{"t":"hello","role":"pedal","ver":1}`
9. Stomp on MPU-6050 → should see: `{"t":"stomp","m":2.8}`

### Connect to PWA
10. Open `http://localhost:8080` in Chrome
11. Click **🔌 USB Serial** → select the Nano port
12. Status shows "Connected: serial"
13. Stomp → kick drum fires in browser

---

## Phase 3 — Steering wheel wiring — 30 min

### Thrustmaster teardown
1. Remove 4 screws on back of wheel unit
2. Inside: find the steering potentiometer (3 wires: GND, wiper, VCC)
3. Find the face button matrix (8 buttons → ground when pressed)
4. Find the paddle switches (2 wires each: GND + signal)

### Wire to ESP32 (if using WiFi mode)
Per docs/wiring-wheel.md. Key pins:
- Steering pot wiper → GPIO34
- Buttons → GPIO4,5,13,14,15,16,17,18
- Paddles → GPIO19, GPIO23

### Flash firmware
1. Library Manager → install arduinoWebSockets + ArduinoJson v6
2. Open `firmware/wheel_unit/wheel_unit.ino`
3. Board: ESP32 Dev Module
4. Upload
5. Serial Monitor: should show "RhythmRide Wheel Unit v2.0" and AP IP 192.168.4.1

---

## Phase 4 — VR phone mount — 40 min

1. Search Thingiverse: "cardboard VR headset" → pick one matching your phone width
2. Print in PLA, 0.3mm layers, 20% infill
3. Estimated time: 35–50 min
4. Attach velcro head strap (workshop roll)
5. Test: put phone in, camera should be aligned with lenses

---

## Phase 5 — NFC sticker — 5 min

**For WiFi (ESP32) mode:**
1. Install "NFC Tools" app (free)
2. Write → URL → `http://192.168.4.1`
3. Stick to wheel base

**For laptop mode (no ESP32):**
1. Find laptop IP: `ipconfig getifaddr en0`
2. Write URL: `http://192.168.4.X:8080`
3. Ensure phone and laptop on same WiFi network

---

## Demo flow check

- [ ] PWA loads on phone from NFC tap
- [ ] Camera passthrough works in VR mode
- [ ] Stomp detected (Nano serial or phone accelerometer)
- [ ] Stomp triggers kick drum sound
- [ ] LED strip flashes on stomp
- [ ] OLED shows stomp count + kcal
- [ ] Steering pot changes filter cutoff (audible sweep)
- [ ] Buttons trigger pad sounds
- [ ] Audience display shows live score on laptop screen
- [ ] Fitness score increments with each stomp

---

## Troubleshooting

| Problem | Fix |
|---------|-----|
| Nano not detected | Try "Old Bootloader" processor, different USB cable |
| MPU calibration hangs | Keep sensor completely still for 3s at boot |
| OLED shows nothing | Check I2C address — run I2C scanner sketch |
| WebSerial button missing | Use Chrome or Edge (Firefox doesn't support WebSerial) |
| No sound | Click/tap the "TAP TO START AUDIO" gate first |
| LED flicker | Add 1000µF cap on LED 5V rail |
| Motor doesn't spin | Check ENA/ENB pins are HIGH (PWM signal reaching driver) |
