# RhythmRide — Cloud Deploy Guide (Option C)

Lets a judge's phone connect to the system **without being on the same WiFi**.

```
[Arduino Uno]
     ↕ USB Serial (115200 baud)
[Laptop]  ←→  bridge/bridge.js (Node.js)
                    ↕ WebSocket (wss://)
         [Cloudflare Worker — Durable Object relay]
                    ↕ WebSocket (wss://)
           [Judge's Phone — PWA on Vercel]
```

---

## 1. Deploy Cloudflare Worker (5 min)

```bash
npm install -g wrangler
wrangler login          # opens browser, sign in with Cloudflare account
cd worker
npm install
wrangler deploy
```

Copy the URL shown, e.g.:
```
https://rhythmride-relay.abc123.workers.dev
```

**Update two placeholders with your real subdomain:**

1. In `pwa/index.html` — find `CLOUD_RELAY_URL` near the top of the `<script>` block:
   ```js
   const CLOUD_RELAY_URL = 'wss://rhythmride-relay.YOUR_SUBDOMAIN.workers.dev/ws?role=phone';
   ```
   Replace `YOUR_SUBDOMAIN` with e.g. `abc123`.

2. In `bridge/config.json`:
   ```json
   { "workerUrl": "wss://rhythmride-relay.abc123.workers.dev/ws" }
   ```

---

## 2. Deploy PWA to Vercel (3 min)

```bash
npm install -g vercel
vercel          # run from repo root, follow prompts
```

Copy the project URL shown, e.g. `https://rhythmride.vercel.app`.

**Write URL to NFC sticker** (or generate a QR code):
- NFC Tools app → Write → URL → `https://rhythmride.vercel.app`
- Stick to wheel base

---

## 3. Run bridge on laptop (during demo)

```bash
cd bridge
npm install
# Edit config.json — fill in your worker URL
npm start
```

Expected output when ready:
```
RhythmRide bridge starting…
[WS] Connecting to wss://rhythmride-relay.abc123.workers.dev/ws?role=bridge …
[SERIAL] Opening /dev/cu.usbmodem1234 at 115200 baud…
[WS] Connected to Cloudflare relay
[SERIAL] Connected to /dev/cu.usbmodem1234
[READY] Bridge is live — Uno data flowing to phone via Cloudflare
```

If the Serial port isn't found automatically, check the port name with:
```bash
# macOS / Linux
ls /dev/cu.usb* /dev/ttyACM* 2>/dev/null
# Windows
wmic path Win32_SerialPort get DeviceID
```

Then set it manually by editing the serial open call in `bridge/bridge.js` or set:
```bash
SERIAL_PATH=/dev/cu.usbmodemXXXX npm start
```

---

## 4. Judge experience

1. Judge taps NFC sticker on the wheel unit
2. Phone opens `https://rhythmride.vercel.app`
3. Cloud WebSocket auto-connects (phone is detected; no button press needed)
4. Judge sees AR note highway — all pedal/wheel data flows live:
   ```
   Uno → USB → laptop bridge → Cloudflare Worker → judge's phone
   ```

---

## Troubleshooting

| Symptom | Fix |
|---|---|
| Bridge says "No port found" | Plug in Uno, check driver (CH340/FTDI), re-run |
| WS connect fails | Check worker URL in config.json — must start with `wss://` |
| Phone shows "Cloud" button but no connection | Confirm CLOUD_RELAY_URL is updated in index.html + Vercel redeployed |
| Worker deploy fails | `wrangler login` again; ensure Durable Objects are enabled on your account |
| "CLOUD_RELAY_URL placeholder" warning | Replace `YOUR_SUBDOMAIN` in index.html before deploying to Vercel |
