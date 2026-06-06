# Pedal Unit Wiring (Arduino Nano)

Arduino Nano has no WiFi — outputs JSON to Serial (115200 baud).
Browser reads via WebSerial API (Chrome/Edge). Click "🔌 USB Serial" in the app.

## Pedal potentiometers (from Thrustmaster pedal unit via DE-9 cable)
```
Throttle pot wiper ──► A0
Brake    pot wiper ──► A1
Pot GND            ──► GND
Pot VCC            ──► 5V
```
Nano ADC: 10-bit (0–1023). Normalised → 0.0–1.0.

## DE-9 pedal connector pinout (Thrustmaster)
Measure with multimeter — colours vary by revision.
| DE-9 Pin | Signal   | Connect to |
|----------|----------|------------|
| varies   | GND      | Nano GND   |
| varies   | VCC      | Nano 5V    |
| varies   | Throttle | Nano A0    |
| varies   | Brake    | Nano A1    |

Probe: set multimeter to DC voltage, put + on each pin with GND as ref.
Pressing pedal changes voltage 0–5V → that's the signal pin.

## MPU-6050 (stomp/jump detection — from Elegoo kit)
```
MPU VCC ──► 3.3V  (or 5V if module has regulator)
MPU GND ──► GND
MPU SDA ──► A4    (Nano I2C default)
MPU SCL ──► A5    (Nano I2C default)
MPU AD0 ──► GND   (sets I2C addr 0x68)
MPU INT ──► (unused)
```

## WS2812B LED strip (visual beat feedback)
```
LED strip 5V  ──► 5V rail (NOT Nano 5V pin — use separate supply or USB)
LED strip GND ──► shared GND
LED strip DIN ──► 300Ω ──► D6
```
Add 1000µF capacitor across 5V/GND of LED strip to prevent brown-out.

## SSD1306 OLED 128×64 (score display)
```
OLED VCC ──► 3.3V
OLED GND ──► GND
OLED SDA ──► A4   (shared I2C bus with MPU)
OLED SCL ──► A5
```
I2C address: 0x3C (most common). If OLED doesn't appear, scan with I2C scanner sketch.

## Extra trigger button (D2)
```
Button ──► D2 ──► GND  (INPUT_PULLUP)
```
Sends `{"t":"btn","i":8,"s":1}` — maps to Pad 8 in app (tap tempo / trigger).

## TT Motor + Motor Driver (rhythm wheel effect — optional)
```
Motor Driver IN1 ──► D8
Motor Driver IN2 ──► D9
Motor Driver IN3 ──► D10
Motor Driver IN4 ──► D11
Motor Driver ENA ──► D5  (PWM)
Motor Driver ENB ──► D3  (PWM)
Motor Driver VCC ──► 7-12V (or 4× AA batteries = 6V)
Motor Driver GND ──► shared GND
Motor A ──► Motor driver OUT1/OUT2
Motor B ──► Motor driver OUT3/OUT4
```
Motors spin 100ms every beat at current BPM → pedal board "ticks" at tempo.

## Power
- Nano: USB from laptop (needed for WebSerial)
- LEDs + motors: separate 5V supply or 4× AA batteries

## Libraries (Library Manager)
- FastLED
- MPU6050_light (rfetick)
- Adafruit SSD1306
- Adafruit GFX Library
