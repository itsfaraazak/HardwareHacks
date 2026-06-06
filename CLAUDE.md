# RhythmRide — CLAUDE.md

## Project
Full-body musical instrument / exergame for KCL HardwareHacks26 hackathon.
Play music by stomping pedals, steering a wheel, hitting buttons — while wearing a phone VR headset with AR note highway overlay.

## Elevator pitch
"RhythmRide is a physical music instrument you play with your whole body. Hop on mobile pedals to the beat, steer a racing wheel to control pitch and filters, hit buttons to trigger samples — all while wearing a 3D-printed phone VR headset that overlays a Guitar Hero note highway on your real surroundings. Creative jam mode too, like Teenage Engineering KO2. Tap an NFC sticker to start. Music + exercise + education in one."

## Repo structure
```
firmware/
  wheel_unit/wheel_unit.ino   ESP32-S3 A: WiFi AP + WS server + steering + buttons
  pedal_unit/pedal_unit.ino   ESP32-S3 B: WS client + pedals + IMU + LEDs + OLED + motors
pwa/
  index.html                  Single-file PWA: landing + VR game + jam + audience modes
  manifest.json               PWA manifest
  sw.js                       Service worker (offline cache)
  data/                       Files to upload to ESP32-S3 A LittleFS
docs/
  wiring-wheel.md             Wheel unit wiring diagram
  wiring-pedal.md             Pedal unit wiring diagram
  bom.md                      Bill of materials + Hardbucks budget
  build-guide.md              Physical build instructions
  demo-script.md              2-minute demo script for judges
```

## Hardware
| Part | Role | Cost (HB) |
|---|---|---|
| 2× ESP32-S3 | Main MCUs | 200 |
| 2× Sensor Module (MPU-6050) | Stomp/jump detection | 60 |
| OLED Display (SSD1306) | Score/BPM display on pedal unit | 100 |
| 2× TT Motor + Motor Driver | Rhythm wheel tick effect | 235 |
| Arduino Nano | Optional: secondary LED controller | 80 |
| 4× Battery (AA) + Holder | Power for pedal unit | 85 |
| Buck Converter | 12V→5V for motors | 15 |
| 2× Breadboard | Prototyping | 50 |
| Buttons (if needed beyond wheel) | Extra triggers | 15 each |
| **Thrustmaster 360 Modena** | Steering wheel (already owned) | 0 |
| **Phone + 3D printed VR mount** | AR display | 0 |

## Architecture
```
[Phone browser] ←WiFi→ [ESP32-S3 A: 192.168.4.1]
                              ↑ WebSocket relay
                        [ESP32-S3 B: pedal unit]
```

- ESP32-S3 A: creates WiFi AP "RhythmRide", runs WS server port 81, serves PWA on port 80
- ESP32-S3 B: connects to AP as station, connects to WS server as client, sends sensor JSON
- Phone: connects to AP, opens http://192.168.4.1 (via NFC sticker), connects WS to :81

## WebSocket message schema (JSON)
```
Device→Phone:
  {"t":"steer","v":0.65}       steering 0-1
  {"t":"btn","i":2,"s":1}      button index 0-7, state 0|1
  {"t":"paddle","i":0,"s":1}   paddle 0=L 1=R
  {"t":"throttle","v":0.3}     throttle pedal 0-1
  {"t":"brake","v":0.0}        brake pedal 0-1
  {"t":"stomp","m":2.8}        stomp G magnitude
  {"t":"jump","m":3.2}         jump landing magnitude
  {"t":"stomps","n":42}        cumulative stomps
  {"t":"hb","ms":12345}        heartbeat

Phone→Device (broadcast):
  {"t":"bpm","v":120}          tempo for motor sync
  {"t":"score","v":1500}       score for OLED
  {"t":"hit"}                  successful note hit → green flash
```

## Arduino libraries (install via Library Manager)
- arduinoWebSockets (Markus Sattler)
- ArduinoJson v6
- FastLED
- MPU6050_light (rfetick)
- Adafruit_SSD1306
- Adafruit_GFX_Library
- ESP32 board package (Espressif Systems)

## Arduino IDE board settings for ESP32-S3
- Board: "ESP32S3 Dev Module"
- USB CDC On Boot: Enabled (for Serial over USB)
- Flash Size: 4MB
- Partition Scheme: Default 4MB with spiffs
- Upload Speed: 921600

## LittleFS upload (PWA files to ESP32-S3 A)
1. Install Arduino ESP32 LittleFS uploader plugin
2. Put files in `firmware/wheel_unit/data/` folder
3. Arduino IDE → Tools → ESP32 LittleFS Data Upload
4. Files served at http://192.168.4.1/filename

## NFC setup
1. Get NTAG213 sticker
2. NFC Tools app → Write → URL → `http://192.168.4.1`
3. Stick to wheel base
4. Phone tap → browser opens → PWA loads → live

## 3D print: VR mount
- Search Thingiverse: "cardboard VR headset"
- Standard params: 63mm IPD, phone width + 2mm clearance
- PLA, 0.3mm layers, ~40min print
- Add velcro head strap

## Demo script (2 minutes)
1. [0:00] Hand judge NFC sticker. "Tap this." Phone opens app.
2. [0:15] Put phone in VR mount. "Look at the note highway."
3. [0:30] Stomp → kick drum + LED flash. "That's a stomp trigger."
4. [0:45] Turn steering wheel → audible filter sweep + vowel shift.
5. [1:00] Hit paddle → melodic sample fires.
6. [1:15] "Watch the fitness score." Stomp 5 times fast. Show kcal counter.
7. [1:30] Remove VR mount. "Creative mode." Free jam buttons + wheel.
8. [1:50] "Built in one day from a PS2 steering wheel."

## Ideas to make it even better (pitch to judges)
- RFID card = song unlock. Tap card to wheel → different note highway song loads
- Multiplayer: two pedal units, competitive score on audience screen
- Music education mode: notes labeled with pitch names, slowed tempo, tutorial overlaid
- Vocaloid voice synthesis: steer through vowels A→E→I→O→U, stomp triggers syllable
- Physio mode: 60 BPM, large hit windows, for rehabilitation
- Fitness leaderboard: QR code on audience screen → phone checks in
- Live FFT waveform on audience display reacts to player actions
- Motor speed = BPM feedback loop → pedal board "ticks" at song tempo physically
