# RhythmRide — Hackathon Context Log
> KCL Guys Campus Hackathon · Hardware Hacks
> Generated from conversation — paste this at the top of any new chat to restore full context.

---

## Project name
**RhythmRide** — a full-body musical instrument / exergame built from a repurposed Thrustmaster 360 Modena PS1/PS2 racing wheel.

---

## Elevator pitch (use this with judges)
> "RhythmRide is a physical music instrument you play with your whole body. You hop on mobile pedals to the beat, steer a racing wheel to control pitch and filters, and hit buttons to trigger samples — all while wearing a 3D-printed phone VR headset that overlays a Guitar Hero / OSU note highway on your real surroundings. There's a creative free-jam mode too, like a Teenage Engineering KO2. Tap an NFC sticker to start. It's music, exercise, and music education in one."

---

## Hardware inventory (what you have)

| Item | Role |
|---|---|
| Thrustmaster 360 Modena (PS1/PS2) | Steering wheel input, ~20kΩ linear pot + 8 face buttons + 2 paddles + D-pad + vibration motors |
| Pedal unit | Throttle + brake, linear pots, spring return, connects via DE-9 (9-pin) cable |
| 2× ESP32 (Elgoo starter kit) | ESP32 A = wheel unit, ESP32 B = pedal unit |
| Elgoo most complete starter kit | Breadboard, jumpers, resistors, LEDs, WS2812 strip, MPU-6050, OLED, buzzers, etc. |
| Android/iPhone | PWA host, camera passthrough VR display, audio output |
| 3D printers (KCL classroom) | VR phone mount (Cardboard-style), pedal board enclosure, caster mounts |
| NFC stickers | NDEF URL record → launches PWA |
| KCL Guys classroom tools | Drill, MDF/plywood offcuts, caster wheels, soldering iron |

---

## Hardware teardown — Thrustmaster 360 Modena

### Steering wheel unit PCB
- **Steering potentiometer:** ~20kΩ linear (B-type), 3 wires: GND / wiper (signal) / VCC
- **Face buttons + paddles:** digital switches, active LOW, wired in matrix
- **Vibration motors:** 2× unbalanced DC motors, ~7.5V supply pin on PS port
- **D-pad + analog toggle:** digital matrix
- **Internal MCU / black-blob IC:** reads pot + buttons, speaks PS SPI protocol

### Pedal unit
- Throttle pot + brake pot, each linear, spring return
- Connected to wheel via **DE-9 (9-pin D-sub)** cable
- Pins carry: VCC, GND, throttle wiper, brake wiper

### PS1/PS2 controller port — 9-pin pinout

| Pin | Name | Direction | Function |
|---|---|---|---|
| 1 | DATA (MISO) | ← ctrl | Button + analog values |
| 2 | CMD (MOSI) | → host | Poll + config commands |
| 3 | MOTOR (7.5V) | → ctrl | Vibration motor supply |
| 4 | GND | — | Ground |
| 5 | VCC (3.3V) | → ctrl | Logic power |
| 6 | ATT / CS | → ctrl | Chip-select, active LOW |
| 7 | CLK (SCK) | → ctrl | Clock ~250kHz |
| 8 | N/C | — | Unused |
| 9 | ACK | ← ctrl | Handshake after each byte |

Protocol: SPI-like, LSB first, ~250–500 kHz. Controller ID for analog wheel = 0x73.

---

## Chosen architecture: Route B (MCU bypass)

Bypass the wheel's internal IC entirely. Wire components direct to ESP32.

### ESP32 A — Wheel unit wiring

| Component | ESP32 pin |
|---|---|
| Steering pot GND | GND |
| Steering pot wiper | GPIO34 (ADC) |
| Steering pot VCC | 3.3V |
| Button 1–8 | GPIO 4,5,12,13,14,15,16,17 (INPUT_PULLUP) |
| Paddle L / R | GPIO 18, 19 (INPUT_PULLUP) |
| Vibration motor (optional) | GPIO 25 → L298N/MOSFET driver |

### ESP32 B — Pedal unit wiring

| Component | ESP32 pin |
|---|---|
| Throttle pot wiper | GPIO34 (ADC) |
| Brake pot wiper | GPIO35 (ADC) |
| Shared GND | GND |
| Shared VCC | 3.3V |
| MPU-6050 SDA | GPIO21 |
| MPU-6050 SCL | GPIO22 |
| MPU-6050 VCC | 3.3V |
| MPU-6050 GND | GND |
| WS2812 LED strip data | GPIO13 |

---

## MPU-6050 on ESP32 B — jump/stomp detection

### Wiring
```
MPU-6050 VCC  →  ESP32 3.3V
MPU-6050 GND  →  ESP32 GND
MPU-6050 SDA  →  ESP32 GPIO21
MPU-6050 SCL  →  ESP32 GPIO22
MPU-6050 AD0  →  GND  (sets I2C address 0x68)
```

### Library
Arduino IDE → Library Manager → install **"MPU6050_light"** by rfetick (lightweight, no DMP needed).

### Detection logic — magnitude threshold method
```cpp
#include <Wire.h>
#include <MPU6050_light.h>

MPU6050 mpu(Wire);

// ── Tune these for your floor/shoe combination ──
#define STOMP_THRESHOLD   2.2f   // g above 1g gravity baseline
#define JUMP_THRESHOLD    1.8f   // g (airborne = <0.3g, land spike = >2.5g)
#define DEBOUNCE_MS       300    // ignore re-triggers within this window

unsigned long lastEventMs = 0;
bool inAir = false;

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);
  mpu.begin();
  mpu.calcOffsets();  // ~3 sec calibration, keep sensor still
}

void loop() {
  mpu.update();
  float ax = mpu.getAccX();
  float ay = mpu.getAccY();
  float az = mpu.getAccZ();
  float mag = sqrt(ax*ax + ay*ay + az*az);  // total g magnitude

  unsigned long now = millis();

  // Jump detection: goes light (airborne) then spikes on landing
  if (mag < 0.4f) inAir = true;

  if (inAir && mag > JUMP_THRESHOLD && (now - lastEventMs > DEBOUNCE_MS)) {
    lastEventMs = now;
    inAir = false;
    triggerEvent("JUMP", mag);
  }

  // Stomp detection: sharp spike without prior airborne phase
  if (!inAir && mag > STOMP_THRESHOLD && (now - lastEventMs > DEBOUNCE_MS)) {
    lastEventMs = now;
    triggerEvent("STOMP", mag);
  }
}

void triggerEvent(const char* type, float mag) {
  Serial.printf("EVENT:%s mag=%.2f\n", type, mag);
  // → send OSC packet here (see comms section)
}
```

### Threshold calibration guide
| Activity | Typical magnitude (g) |
|---|---|
| Resting flat | ~1.0 |
| Gentle walk step | 1.3–1.6 |
| Hard stomp | 2.0–3.5 |
| Jump airborne | 0.1–0.4 |
| Jump landing spike | 2.5–4.0 |

Start with `STOMP_THRESHOLD = 2.2` and `JUMP_THRESHOLD = 1.8`. Serial-print magnitudes and tune live.

---

## Comms layer — ESP32 → Phone

### Protocol: OSC over UDP (WiFi)
- ESP32 A creates a **WiFi soft access point**: `WiFi.softAP("RhythmRide", "beatdrop")`
- ESP32 B connects to same AP as station
- Both send UDP OSC packets to phone IP (phone connects to same AP)
- Phone PWA receives via **WebSocket bridge** (small Node.js script on phone, or use ESP32 web server)

### OSC message schema
```
/wheel/steer     ,f    0.0–1.0   (pot normalised)
/wheel/btn       ,ii   <button_index> <0|1>
/pedal/throttle  ,f    0.0–1.0
/pedal/brake     ,f    0.0–1.0
/pedal/stomp     ,f    <magnitude>
/pedal/jump      ,f    <magnitude>
```

### Arduino library for OSC on ESP32
Library Manager → **"OSCBundle"** (CNMAT) or use raw UDP with hand-rolled OSC (4-byte aligned strings + float).

---

## Phone PWA architecture

**Stack:** Vanilla JS + Tone.js + Web Audio API. No framework needed for hackathon speed.
Hosted: ESP32 A's built-in web server (SPIFFS), or `npx serve` on a laptop, opened via NFC URL.

### Modes
| Mode | Description |
|---|---|
| VR game mode | `getUserMedia` camera passthrough, canvas note highway overlay, device gyro for head look, Guitar Hero / OSU timing logic |
| Creative / jam mode | KO2-style 8-pad grid (face buttons), steering = pitch bend / filter cutoff, pedals = volume/expression, loop record + overdub |

### Sound engine (Tone.js)
```js
import * as Tone from 'tone';

// Sample pads — map to 8 buttons
const sampler = new Tone.Sampler({ C3: 'kick.mp3', D3: 'snare.mp3', ... }).toDestination();

// Steering wheel → filter
const filter = new Tone.Filter(800, 'lowpass').toDestination();
const synth = new Tone.Synth().connect(filter);

// Vocaloid-style vowel synth (formant)
const f1 = new Tone.BiquadFilter(800, 'bandpass');  // "ah"
const f2 = new Tone.BiquadFilter(1200, 'bandpass'); // "ee"
const osc = new Tone.Oscillator('sawtooth').fan(f1, f2);
// Steering wheel crossfades f1.frequency between 300–800 Hz

// Receive from OSC bridge
ws.onmessage = ({data}) => {
  const {address, args} = parseOSC(data);
  if (address === '/wheel/steer') filter.frequency.value = 200 + args[0] * 3000;
  if (address === '/pedal/stomp') sampler.triggerAttack('C3');
  if (address === '/pedal/jump')  triggerJumpEffect();
};
```

---

## NFC activation

1. Get NFC sticker (NTAG213 or NTAG215 — both work)
2. Use **NFC Tools** app (Android/iOS) to write NDEF record: type = URL, value = `http://192.168.4.1` (ESP32 AP web server IP)
3. Stick to wheel base or pedal board
4. Phone tap → browser opens → PWA loads → system live
5. No app install. No QR code. Magic.

---

## 3D print: VR phone mount

- Print a Google Cardboard-compatible shell in PLA, 0.3mm layer height, ~40 min
- Standard lens spacing: 63mm IPD
- Phone slot width: measure your phone + 2mm clearance
- Search Thingiverse: **"cardboard VR headset"** — dozens of remixable files
- Add a velcro head strap from the KCL workshop

---

## Physical pedal board build

1. Cut ~400×300mm MDF or plywood from KCL offcuts
2. Screw 4 caster wheels underneath (workshop stock)
3. Cable-tie pedal assembly to top surface
4. Velcro ESP32 B underneath
5. USB power bank powers ESP32 B
6. Pedal DE-9 cable runs into ESP32 B breadboard
7. WS2812 LED strip along front edge (Elgoo kit)
8. Total build time: ~45 minutes

---

## WS2812 LED beat sync (ESP32 B)

Library: **FastLED**
```cpp
#include <FastLED.h>
#define NUM_LEDS 30
#define LED_PIN  13
CRGB leds[NUM_LEDS];

// On stomp event:
void flashStomp() {
  fill_solid(leds, NUM_LEDS, CRGB::White);
  FastLED.show();
  delay(80);
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  FastLED.show();
}
```

---

## Fitness / exercise angle (judge talking point)

- Count stomps per session
- Estimate kcal: ~0.1–0.15 kcal per vigorous stomp
- Display on audience screen / PWA
- Pitch: "Music + exercise + music education in one device"
- Future: physio mode (slower tempo → music therapy)

---

## Demo script (2 minutes for judges)

1. **[0:00]** Hand judge the NFC sticker. "Tap this." Phone opens app.
2. **[0:15]** Put phone in VR mount. "Look at the note highway."
3. **[0:30]** Demonstrate stomp → kick drum + LED flash.
4. **[0:45]** Turn steering wheel → audible filter sweep + vocaloid vowel shift.
5. **[1:00]** Hit paddle button → melodic sample fires.
6. **[1:15]** "Now watch the fitness score." Stomp 5 times fast.
7. **[1:30]** Remove VR mount. "This is creative mode." Free jam on buttons + wheel.
8. **[2:00]** "Built in one day, from a £10 car boot PS2 steering wheel."

---

## Open sub-tasks (each can be a separate chat)

Paste this entire file at the top of a new chat, then ask your specific question.

| Sub-task | Prompt to use |
|---|---|
| ESP32 A firmware (wheel) | "Using the RhythmRide context below, write complete ESP32 A Arduino firmware that reads the steering pot on GPIO34, 8 buttons, 2 paddles with INPUT_PULLUP, and sends OSC UDP packets over WiFi softAP" |
| ESP32 B firmware (pedals + IMU) | "Using the RhythmRide context below, write complete ESP32 B firmware: read 2 pedal ADC channels, MPU-6050 jump/stomp detection, WS2812 LED flash on events, send OSC UDP" |
| PWA sound engine | "Using the RhythmRide context below, build the complete PWA index.html with Tone.js sound engine that receives OSC via WebSocket, maps wheel/pedal/stomp messages to audio" |
| VR camera overlay (game mode) | "Using the RhythmRide context below, build the VR game mode: camera passthrough, scrolling note highway canvas overlay, gyroscope head-look, hit detection from stomp OSC events" |
| Vocaloid formant synth | "Using the RhythmRide context below, implement the vocaloid-style formant synthesiser in Web Audio API where steering wheel value crossfades F1/F2 formant frequencies" |
| OSC WebSocket bridge | "Using the RhythmRide context below, write a minimal Node.js script that bridges UDP OSC packets from the ESP32s to WebSocket for the phone PWA" |
| NFC setup guide | "Using the RhythmRide context below, give step-by-step instructions to write an NDEF URL record to an NFC sticker using NFC Tools app pointing to the ESP32 web server" |
| Audience display | "Using the RhythmRide context below, build a second-screen audience visualiser PWA page that shows live score, stomp count, fitness kcal, and a waveform visualiser" |
| 3D print VR mount | "Using the RhythmRide context below, give me exact parameters and a Thingiverse search strategy to find and customise a VR phone mount for my phone model" |
| Pedal board physical build | "Using the RhythmRide context below, give me a step-by-step build guide for the mobile pedal board with caster wheels and LED strip" |

---

*End of context log — version 1.0*
