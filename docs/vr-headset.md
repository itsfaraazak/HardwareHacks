# VR Headset — No Lenses, Fast Print

No optical lenses available. No problem. At **25–35 cm viewing distance** the phone screen is large enough to fill a significant portion of your field of view without distortion. This gives the "wraparound" immersion needed for the AR note highway demo.

The PWA already defaults to **AR mode** (single full-screen, no split), so no VR optics are required. The headset is just a rigid frame that holds the phone steady at the right distance.

---

## Option A — Print our custom OpenSCAD cradle (< 40 min)

File: `docs/vr-mount.scad`

- Parametric for phone widths 68–80 mm
- PLA, 0.3 mm layers, 20% infill, no supports
- 4 walls + nose bridge + eye gap + elastic-band slots
- No lens holes — phone sits at 30 cm from eyes

**Estimated print time**: 30–38 minutes on a standard FDM printer.

See `docs/vr-mount.scad` for the full script and instructions.

---

## Option B — Thingiverse fast prints (choose one)

All of these print without supports and can be modified to remove lens holders.

| Model | Search term | Print time | Notes |
|---|---|---|---|
| **Mini VR Viewer v2** | `thingiverse "mini VR viewer" no lens` | ~35 min | Compact, snap-fit |
| **Cardboard-style box viewer** | `thingiverse "phone VR box" open` | ~40 min | Box with eye gap cut-out |
| **Periscope viewer** | `thingiverse "phone scope viewer"` | ~25 min | Simplest: just a tube |
| **Google Cardboard remix** | `thingiverse "cardboard VR remix fast"` | ~45 min | Fits pharmacy reading glasses (+1 to +2 dioptre) if you want optional lenses |

**Recommended search on Thingiverse**: `phone VR holder no lens` — filter by "Print time < 1hr".

### If you want to add cheap lenses later
Pharmacy reading glasses (+1 to +2 dioptre, ~£3–5) work as makeshift VR lenses. Pop the lenses out and glue or slot them into any of the above models. Move phone to 15–20 cm for lensed use.

---

## Print settings (for all options)

| Setting | Value |
|---|---|
| Layer height | 0.30 mm |
| Infill | 20% gyroid or grid |
| Supports | None |
| Perimeters / walls | 2 |
| Speed | 60+ mm/s |
| Material | PLA |
| Bed temp | 60°C |
| Nozzle temp | 210°C |

> Thinner walls = faster print. 2 perimeters is enough for a demo prop.

---

## Attaching the phone

No clips needed — elastic bands or velcro work perfectly.

1. **Elastic band method**: loop 2–3 rubber bands over the phone and around the frame. Takes 10 seconds.
2. **Velcro method**: stick a 5 cm velcro strip to the back of the phone case and matching piece inside the cradle slot.
3. **Tape**: a strip of painter's tape over the phone while in the cradle is invisible from the front and holds firm.

---

## Head strap

| Method | Time | Cost |
|---|---|---|
| Velcro roll (20 mm wide) | 2 min | ~£1 |
| Repurposed elastic hairband | 1 min | £0 |
| Rubber exercise band cut to strips | 2 min | £0 |
| Bungee cord loop | 1 min | £0 |

**Velcro method**: cut two 30 cm strips. Hot-glue or zip-tie them to the sides of the cradle frame. Thread around the back of the head. Loop the velcro on itself. Done.

---

## Viewing distance notes

| Distance | Effect |
|---|---|
| 20 cm | Wide apparent FOV (~80°). Text is very large. Good with +1 reading lens. |
| 30 cm | Comfortable naked-eye distance. ~60° apparent FOV. Recommended. |
| 35 cm | Slightly narrower immersion but zero eye strain. Best for long demos. |

The OpenSCAD model (`vr-mount.scad`) sets phone distance to **30 cm** by default. Adjust the `PHONE_DIST` parameter to change it.

---

## Quick demo tip

For a 2-minute hackathon demo you don't even need the headset to be wearable — you can simply hand the phone to the judge and say "hold this up to your face and stomp". The immersion effect still reads clearly.

The headset mount is most impressive when you stomp and the note highway reacts in sync with the kick drum — even at arm's length the visual is compelling.
