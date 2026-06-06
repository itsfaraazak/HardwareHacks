# RhythmRide — Breadboard Wiring (Total Beginner Guide)

Same 3 parts as before (**1 knob + 2 buttons**), but mounted on the breadboard so nothing wobbles. **The code on the Uno does not change — no reflashing.** A breadboard is just a tidy place to hold wires.

---

## Wait — what is a "pot" / knob?

"Pot" = **potentiometer** = the **twisty knob**. Same thing. It has **3 legs and no labels** — you tell them apart by position. **The middle leg is the only fixed one.**

You have one of these shapes:

```
  SMALL BLUE (twist top):              BIG with a SHAFT (push on a knob cap):
        _____                                  ╓─╖   ← metal shaft
       / ⊙   \                                ╓╜   ╙╖
      |_______|                              ╔╝      ╚╗
       |  |  |                               ║  body  ║
       1  2  3                               ╚╗      ╔╝
       │  │  │                                 ║ ║ ║
     out MID out                               1 2 3
                                             out MID out
```

| Leg | Goes to |
|-----|---------|
| **Middle (2)** — the signal | Uno **`A0`** |
| One outer | **`5V`** (+ rail) |
| Other outer | **`GND`** (– rail) |

Outer two swap freely (pitch backwards? swap them). Middle is always `A0`.

---

## Your actual Arduino Uno — 《circled》holes get a wire

Labels are printed in tiny white text on the board. USB on the left:

```
        DIGITAL pins (along the TOP edge)
 ┌──────────────────────────────────────────────────────────────────┐
 │  SCL SDA AREF GND 13 12 11 10  9  8 ‖  7  6  5 《4》 3 《2》 1  0      │
═╡USB                                       gap     ▲       ▲         │
═╡                                              to BTN2  to BTN1      │
 │ ┌───┐          A R D U I N O   U N O                               │
 │ │PWR│                                                              │
 │ └───┘ IOREF RST 3V3《5V》《GND》GND VIN ‖《A0》A1 A2 A3 A4 A5           │
 │                    ▲    ▲             ▲                            │
 └────────────────────│────│─────────────│────────────────────────────┘
        POWER + ANALOG (BOTTOM edge)       │
                  to + rail  to – rail   to KNOB MIDDLE leg
```

Only **5 holes** are used: `5V`, `GND`, `A0` (bottom edge) and `2`, `4` (top edge, toward the right end). `‖` = the gap in the row.

---

## First: how a breadboard works (the only rule you need)

A breadboard is the white block full of holes. Holes are secretly joined together underneath in a fixed pattern:

```
   ┌───────────────────────────────────────────────┐
 + │ ●─●─●─●─●─●─●─●─●─●─●─●─●─●─●─●─●─●─●─●─●─●─●─●─●  │  ← TOP red rail  (all joined left↔right)
 – │ ●─●─●─●─●─●─●─●─●─●─●─●─●─●─●─●─●─●─●─●─●─●─●─●─●  │  ← TOP blue rail (all joined left↔right)
   │                                                 │
   │     a  b  c  d  e        f  g  h  i  j          │
   │  1  ●  ●  ●  ●  ●        ●  ●  ●  ●  ●           │
   │  2  ●  ●  ●  ●  ●        ●  ●  ●  ●  ●           │
   │  3  ●  ●  ●  ●  ●        ●  ●  ●  ●  ●           │
   │       ↑joined↑   │TRENCH│   ↑joined↑            │
   │  ...            (middle gap)                    │
   │                                                 │
 – │ ●─●─●─●─●─●─●─●─●─●─●─●─●─●─●─●─●─●─●─●─●─●─●─●─●  │  ← BOTTOM blue rail
 + │ ●─●─●─●─●─●─●─●─●─●─●─●─●─●─●─●─●─●─●─●─●─●─●─●─●  │  ← BOTTOM red rail
   └───────────────────────────────────────────────┘
```

**The 3 rules:**
1. **Side rails** (the long `+` red and `–` blue lines): every hole in one rail is joined, all the way across.
2. **Middle holes**: joined in short vertical groups of 5 — `a b c d e` of one row are joined to each other (a column of 5). `f g h i j` likewise. **But the left group and right group are NOT joined** — the trench in the middle splits them.
3. So: poke two wire ends into the **same column** = they're connected.

---

## Step 1 — Power the rails (2 wires)

This feeds power down the whole breadboard so every part can grab it.

| Wire from Uno hole | …to breadboard |
|--------------------|----------------|
| `5V`  | any hole in a **`+` (red) rail** |
| `GND` | any hole in a **`–` (blue) rail** |

Now the entire red rail is "5V" and the entire blue rail is "ground." You only used **one** GND hole on the Uno — the rail shares it with everything.

---

## Step 2 — The knob (3 wires + the part)

Plant the knob (the small blue twist-top, or the big one with a shaft) so its **3 legs sit in 3 different columns**, e.g. rows 5, 6, 7 on the left half. (Middle leg = signal, see drawing above.)

```
        a   b   c   d   e
   5   [L]  ·   ·   ·   ·     ← knob left leg here
   6   [M]  ·   ·   ·   ·     ← knob middle leg here
   7   [R]  ·   ·   ·   ·     ← knob right leg here
```

Then run 3 short wires from column `a`:

| From breadboard (knob's column) | …to |
|--------------------------------|-----|
| Left leg's row (a5)   | `+` red rail |
| Middle leg's row (a6) | Uno `A0` |
| Right leg's row (a7)  | `–` blue rail |

(Left/right are swappable — if pitch runs backwards later, swap these two.)

---

## Step 3 — Button 1 = STOMP (2 wires + the part)

Push the button so it **straddles the middle trench** — its legs land in the left half AND the right half. This locks it in place (the whole reason we use a breadboard).

```
        e │TRENCH│ f
   10   [leg]    [leg]      ← button bridges the gap
```

Wire it:

| From | …to |
|------|-----|
| A column touching one button leg (left side) | Uno `2` |
| A column touching the **other** leg (right side) | `–` blue rail |

---

## Step 4 — Button 2 = SNARE (2 wires + the part)

Same as Step 3, a few rows down so the buttons don't touch.

| From | …to |
|------|-----|
| One leg's column | Uno `4` |
| Other leg's column | `–` blue rail |

---

## Full wire list (9 wires total)

| # | From | To |
|---|------|----|
| 1 | Uno `5V` | `+` red rail |
| 2 | Uno `GND` | `–` blue rail |
| 3 | Knob left leg's column | `+` red rail |
| 4 | Knob middle leg's column | Uno `A0` |
| 5 | Knob right leg's column | `–` blue rail |
| 6 | Button 1 leg | Uno `2` |
| 7 | Button 1 other leg | `–` blue rail |
| 8 | Button 2 leg | Uno `4` |
| 9 | Button 2 other leg | `–` blue rail |

---

## Turn on + connect (unchanged)

1. Uno → laptop via **USB cable**.
2. Open **Chrome** or **Edge** → open `pwa/index.html`.
3. Click **🔌 Connect Uno** → pick **Arduino Uno** → Connect. Dot turns green.

## Play

| Action | Sound |
|--------|-------|
| Twist knob | pitch / filter sweep |
| Press Button 1 | kick (stomp) + screen flash |
| Press Button 2 | snare |

---

## If something's wrong

| Problem | Fix |
|---------|-----|
| Knob dead | Middle leg's column must wire to `A0`; outer columns to `+` and `–` rails. |
| Button always firing | Its two wires are on the same side of the trench — move one so the button bridges the gap. |
| Button never fires | The two wires touch legs that are already joined. Use legs on **opposite** sides of the trench. |
| Pitch backwards | Swap the knob's `+` and `–` wires. |
| Nothing at all | Green dot on in the app? 5V + GND rail wires in? Re-click 🔌 Connect Uno. |

---

## Optional add-ons (now that you have a breadboard)

These need a quick **reflash** of the Uno — say the word and I'll do it:

- **2 LEDs** (flash on stomp / on note hit) — breadboard makes the resistor easy.
- **Buzzer** — beeps on each hit.
- **2 more knobs** — throttle + brake.
- **2 more buttons** — extra drum pads.
- **LCD screen** — shows BPM + score.

For now, **1 knob + 2 buttons plays great.**
