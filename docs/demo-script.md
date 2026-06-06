# RhythmRide — 2-Minute Demo Script

## Setup (before judges arrive)
- PWA running on laptop: `cd pwa && python3 -m http.server 8080`
- Audience display open on laptop: `http://localhost:8080#audience`
- Pedal board plugged in + on floor with wheels
- VR mount + phone ready
- NFC sticker on wheel base

---

## The script

**[0:00] — The hook**
> "This is RhythmRide. A musical instrument you play with your whole body."

Hand judge the NFC sticker. "Tap this to your phone."

→ Phone browser opens the app automatically.

---

**[0:15] — VR game mode**
> "Put the phone in here." *(Hand VR mount)*

> "Look around — the note highway follows your head movement."

*(Phone gyroscope shifts the note highway as they tilt their head)*

---

**[0:30] — First interaction: stomp**
> "Now stomp on the pedal."

→ Kick drum fires. LED strip flashes white. OLED increments. Audience screen updates.

> "That's a stomp trigger. The sensor detects landing G-force, not just a button press."

---

**[0:45] — Steering: filter + vowel**
> "Turn the wheel."

→ Audible filter sweep. Formant vowel changes A → E → I → O → U on screen.

> "The steering pot maps to audio filter cutoff AND vowel synthesis. You're literally shaping sound by steering."

---

**[1:00] — Button / paddle**
> "Hit any button." *(Point to face buttons)*

→ Melodic sample fires.

> "And the paddle—" *(trigger left paddle)* "—triggers the chord synth."

---

**[1:15] — Fitness angle**
> "Watch the fitness score." *(Point to audience display)*

Stomp 5 times fast.

> "0.12 kcal per stomp. That's real cardio. We counted. 500 stomps is a proper workout while playing music."

---

**[1:30] — Creative mode (no VR)**
> "Remove the headset. This is creative mode."

*(Switch to Jam mode)*

Free-jam: hit pads, sweep steering, throttle pedal changes reverb.

> "Like a Teenage Engineering pocket operator, but full-body."

---

**[1:50] — Wrap**
> "Built in one day. From a £10 PS2 steering wheel, a 3D-printed headset, and an Arduino."

> "Music education, exercise, and creative expression — one instrument."

---

## Judging criteria talking points

**Innovation:** Novel combination of physical exertion + music performance. Steering wheel repurposed as a synthesizer controller.

**Technical depth:** Sensor fusion (accelerometer G-force for stomp detection), formant vowel synthesis (F1/F2 bandpass filters), AR camera passthrough with note highway, hardware-agnostic architecture (WebSerial, Web Bluetooth, WiFi, phone sensors — hot-swappable).

**Impact:** Fitness + music education simultaneously. Physio rehabilitation angle. No music knowledge required — intuitive physical gestures map to musical output.

**Execution:** Working demo with camera passthrough, real audio synthesis, live fitness tracking, audience display — all in a 24-hour hackathon.

---

## Backup plan (if hardware fails)

PWA works entirely without hardware:
- SPACE = stomp
- A–H = pads
- ← → = steering

Hand the laptop to the judge. The demo runs entirely in the browser.
