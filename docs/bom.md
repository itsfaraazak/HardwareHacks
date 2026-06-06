# Bill of Materials — RhythmRide

## Already owned (free)
| Item | Source | Role |
|------|--------|------|
| Thrustmaster 360 Modena PS1/PS2 | Already have | Steering wheel + pedals + buttons |
| Phone (Android/iPhone) | Already have | VR display + AR overlay |
| 3D printers | KCL classroom | VR mount, enclosures |
| MDF/plywood offcuts | KCL workshop | Pedal board base |
| Caster wheels | KCL workshop | Mobile pedal platform |
| Soldering iron | KCL workshop | Assembly |
| Velcro | KCL workshop | Head strap, cable management |
| Laptop | Already have | WebSerial bridge + audience display |

## Hackathon store (Hardbucks)

### Core (must-buy)
| Item | Qty | HB each | Total HB | Role |
|------|-----|---------|----------|------|
| Arduino Nano | 1 | 80 | 80 | Pedal unit MCU (serial output, no WiFi needed) |
| Breadboard | 2 | 25 | 50 | Prototyping |
| Sensor Module (MPU-6050) | 1 | 30 | 30 | Stomp/jump detection |
| Jumper wire | lots | free | 0 | Wiring |
| Tape | — | free | 0 | Cable management |
| **Core subtotal** | | | **160 HB** | |

### Enhanced (buy if budget allows)
| Item | Qty | HB each | Total HB | Role |
|------|-----|---------|----------|------|
| OLED Display (SSD1306) | 1 | 100 | 100 | Score/BPM/fitness on pedal board |
| Motor (TT yellow) | 2 | 100 | 200 | Rhythm wheel — ticks at BPM |
| Motor Driver | 1 | 35 | 35 | Drives TT motors |
| Buck Converter | 1 | 15 | 15 | Regulate battery voltage to motors |
| Battery (AA) | 4 | 15 | 60 | Power for motors + Nano |
| Battery Holder | 1 | 25 | 25 | 4× AA holder |
| Cable Tie | 5 | 5 | 25 | Board assembly |
| **Enhanced subtotal** | | | **460 HB** | |

### Upgrade (if ESP32 WiFi wanted)
| Item | Qty | HB each | Total HB | Role |
|------|-----|---------|----------|------|
| ESPRESSIS ESP32-S3 | 2 | 100 | 200 | Replaces Nano — adds WiFi, no laptop bridge needed |
| **Upgrade subtotal** | | | **200 HB** | |

## Budget scenarios
| Scenario | HB | What you get |
|----------|-----|-------------|
| Core only | 160 | Pedal stomps + buttons via laptop USB |
| Core + OLED | 260 | + score display on pedal board |
| Core + Full enhancement | 620 | + OLED + motors + battery |
| Full with ESP32-S3 | 660 | + WiFi direct (no laptop bridge) |

## NFC stickers
Get NTAG213 or NTAG215 stickers from workshop/store.
Write URL via NFC Tools app (free, iOS + Android).

## 3D print: VR phone mount
- Search Thingiverse: "cardboard VR headset"
- Material: PLA, 0.3mm layers, ~40min
- Standard IPD: 63mm
- Phone slot: measure phone width + 2mm clearance
- Add velcro head strap from KCL workshop
