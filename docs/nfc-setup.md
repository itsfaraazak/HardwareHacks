# NFC Launch Setup — RhythmRide

A judge taps the NFC sticker on the steering wheel → their phone opens a browser → the PWA loads → WebSocket auto-connects to the ESP32 → game starts. No app install required.

---

## What you need

| Item | Notes |
|---|---|
| NTAG213 sticker (or NTAG215/216) | Tiny white sticker, ~€0.30 each |
| NFC Tools app | Free — iOS App Store / Google Play |
| ESP32-S3 A powered on | Must be broadcasting the "RhythmRide" WiFi AP |

---

## Step 1 — Write the URL to the sticker

1. Install **NFC Tools** (iOS or Android).
2. Open the app → tap **Write** → **Add a record** → **URL / URI**.
3. Enter exactly: `http://192.168.4.1`
4. Tap **OK** → tap **Write / 4 bytes**.
5. Hold the sticker flat against the back of your phone (near the NFC chip — usually top-centre on Android, top-centre or bottom on iPhone 14+).
6. You'll hear a beep / feel a haptic. Done.

> Tip: NTAG213 holds 144 bytes — more than enough for this URL.

---

## Step 2 — Test the write worked

1. Open NFC Tools → **Read** tab.
2. Tap the sticker.
3. You should see: `Record type: URI / URL` → `http://192.168.4.1`

Alternatively, tap the sticker with your phone while **not** on the RhythmRide WiFi — Android will say "Open in browser" (it may fail to load, which is fine — it proves the URL launched).

---

## Step 3 — Demo flow (what happens when a judge taps)

```
Judge taps sticker
  → Phone NFC reads URL http://192.168.4.1
  → Default browser opens
  → Phone must be on "RhythmRide" WiFi AP (tell judge to connect)
  → PWA loads from ESP32 LittleFS
  → Mobile landing screen appears: "TAP TO START"
  → WebSocket auto-connects to ws://192.168.4.1:81
  → Judge taps "TAP TO START" → AR game mode launches
```

> The WiFi step: the ESP32 broadcasts an open AP called **RhythmRide**. Ask the judge to go to Settings → WiFi → connect to "RhythmRide" before tapping the sticker, or tap the sticker first (it will fail), then connect to WiFi, then tap again.

---

## Sticker placement

- Stick the NFC sticker to the **base of the steering wheel** or on a prominent flat surface.
- Write a small label underneath: **"TAP WITH PHONE"**.
- Keep it away from metal surfaces (metal blocks NFC). Stick on plastic or wood.
- If your wheel has a metal base, use a **ferrite sheet** spacer (1–2 mm, ~£1) between sticker and metal.

---

## Troubleshooting

### iOS (iPhone)

| Problem | Fix |
|---|---|
| Nothing happens when tapping | iPhone requires **background NFC** (iPhone XS+, iOS 13+). If it doesn't auto-open, open NFC Tools → Read → tap sticker → tap the URL it shows. |
| "Safari cannot open page" | Phone is not on the RhythmRide WiFi yet. Connect to WiFi first. |
| Still not working | Use NFC Tools to write the URL → manually enter `http://192.168.4.1` in Safari. |

### Android

| Problem | Fix |
|---|---|
| Nothing happens | Check Settings → Connected devices → NFC is **ON**. |
| "Web page not available" | Connect to RhythmRide WiFi first (it's an offline AP, no internet). |
| Opens wrong app | Android asks which app — choose your browser (Chrome recommended). |

### General

- **Distance**: hold the phone within 2–3 cm of the sticker. Don't swipe — hold still for 0.5 s.
- **Phone case**: thick cases can block NFC. Remove case or hold at an angle.
- **Multiple taps**: tapping again just reloads the page — that's fine.
- **WebSocket not connecting**: verify the ESP32 is powered on and the "RhythmRide" AP is visible in WiFi list.

---

## For the demo (quick checklist)

- [ ] ESP32 A powered on, LED solid (AP active)
- [ ] NFC sticker written with `http://192.168.4.1`
- [ ] Test: tap sticker with your own phone — PWA loads
- [ ] Tell judges: "Connect to 'RhythmRide' WiFi (no password), then tap this sticker"
- [ ] If iOS 16 and below: open NFC Tools and tap Read first, then tap the URL link
