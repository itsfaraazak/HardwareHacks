# Wheel Unit Wiring (Standard ESP32)

## Critical ESP32 pin rules
- **GPIO6–11 = internal flash → NEVER use** (instant crash)
- **ADC2 pins (GPIO0,2,4,12–15,25–27) = dead while WiFi active** → use ADC1 only
- **ADC1 safe pins: GPIO32, 33, 34, 35, 36(VP), 39(VN)**
- GPIO34, 35, 36, 39 are **INPUT ONLY** (no pullup, no output)

## Steering potentiometer (from Thrustmaster wheel)
```
Pot GND   ──► ESP32 GND
Pot WIPER ──► ESP32 GPIO34  (ADC1 — DO NOT use GPIO4)
Pot VCC   ──► ESP32 3.3V
```
Reads 0–4095 → normalised 0.0–1.0. Broadcasts `{"t":"steer","v":0.65}` when delta > 0.008.

## Face buttons (8× from wheel PCB, active LOW)
```
Button 1 ──► GPIO4   + resistor or direct, other end to GND
Button 2 ──► GPIO5
Button 3 ──► GPIO13
Button 4 ──► GPIO14
Button 5 ──► GPIO15
Button 6 ──► GPIO16
Button 7 ──► GPIO17
Button 8 ──► GPIO18
```
All use INPUT_PULLUP. Press = LOW.

## Paddles
```
Paddle Left  ──► GPIO19  (INPUT_PULLUP)
Paddle Right ──► GPIO23  (INPUT_PULLUP)
```

## Vibration motor (optional)
```
GPIO21 ──► 1kΩ ──► NPN transistor base (e.g. 2N2222)
Transistor emitter ──► GND
Transistor collector ──► motor –
Motor + ──► 5V rail
```
Comment out `#define VIBRO_ENABLED` in firmware if not wired.

## Power
- USB from laptop (programming + power)
- Or 5V powerbank via USB-C

## Arduino IDE settings
```
Board:               ESP32 Dev Module
Upload Speed:        115200 (use if 921600 fails)
Flash Size:          4MB
Partition Scheme:    Default 4MB with spiffs
USB CDC On Boot:     Disabled (standard ESP32)
```

## Libraries (Library Manager)
- arduinoWebSockets (Markus Sattler)
- ArduinoJson v6 (Benoit Blanchon)
