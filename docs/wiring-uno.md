# RhythmRide — Wiring (Total Beginner Guide)

Never touched an Arduino before? Perfect. Read this top to bottom.

---

## What you have

- **The Uno** — the blue circuit board. The brain.
- **The black strips with little holes** along its edges = **sockets**. You poke wire ends into them. Each hole has a printed label next to it (`5V`, `GND`, `A0`, `2`, `4`…).
- **Jumper wires** — colored wires with a metal pin on each end. They carry the signal. (Use the ones with a stiff metal **pin** on the end — they push into the holes.)
- **The knob** — round part with 3 metal legs. Twist it = the instrument's pitch.
- **2 push buttons** — little square clickers. These are your drum pads.

That's all. **No breadboard. No soldering. No resistors.**

---

## The whole build = 7 wires

You plug wires straight into the Uno's labeled holes. Nothing else.

```
        ARDUINO UNO  (looking down at it)
   ┌──────────────────────────────────────┐
   │  digital holes (top edge)             │
   │   ...  [GND] 13 12 11 10 9 8          │
   │              7 6 5 [4] 3 [2] 1 0      │
   │                      │       │        │
   │                   BUTTON2 BUTTON1     │
   │                                       │
   │                                       │
   │   power holes        analog holes     │
   │   ...[5V][GND][GND]  [A0] A1 A2 ...    │
   │       │    │    │     │                │
   │      KNOB KNOB BTN   KNOB              │
   │      left right legs  mid              │
   └──────────────────────────────────────┘
```

### The 7 connections

| # | Wire from this part | …goes into this Uno hole |
|---|---------------------|--------------------------|
| 1 | Knob — **left** leg   | `5V`   |
| 2 | Knob — **middle** leg | `A0`   |
| 3 | Knob — **right** leg  | `GND`  (one of the two by 5V) |
| 4 | Button 1 — one leg    | `2`    |
| 5 | Button 1 — other leg  | `GND`  (the second one by 5V) |
| 6 | Button 2 — one leg    | `4`    |
| 7 | Button 2 — other leg  | `GND`  (the one near hole `13`) |

> The Uno has **three** `GND` holes. Use a different one for each part — that's why all three are listed.

---

## How to actually plug a wire in

1. Take a jumper wire. Look at the metal pin on the end.
2. Push that pin into the labeled hole. It should grip snugly.
3. Push the other end onto the part's leg.
   - **Knob legs** and **button legs** are thin metal — slide the wire's socket/pin over them. Wiggle gently until snug.

That's it. Repeat 7 times.

### Button legs — which two?

A push button has 4 legs (one in each corner). Pick **two legs that are diagonally across from each other** (far corners). Those two turn on when you press. (If a button seems "always on," move one wire to a different corner.)

### Knob legs — which is which?

Hold the knob so the 3 legs face you in a row. **Left** and **right** are the outer two (5V and GND — swappable, doesn't matter which). **Middle** is the wiper → `A0`.

---

## Turn it on

1. Plug the Uno into your laptop with the **USB cable** (the square-ish USB-B end into the Uno).
2. A light on the Uno turns on. It's already running — we flashed it for you.

---

## Connect it to the music app

1. Open **Google Chrome** or **Microsoft Edge**. (Firefox/Safari won't work — they can't talk to USB.)
2. Open the app: the `pwa/index.html` file.
3. Click the **🔌 Connect Uno** button.
4. A little window pops up. Pick **"Arduino Uno"** from the list → click **Connect**.
5. Done. The dot turns green.

---

## Play

| You do this | App does this |
|-------------|---------------|
| **Twist the knob** | Sweeps the pitch / filter up and down |
| **Press Button 1** | Kick drum + screen flash (a "stomp") |
| **Press Button 2** | Snare / second sound |

No hardware handy? The app also works with the keyboard: **SPACE** = stomp, **A** = snare, **← →** = pitch.

---

## If something's wrong

| Problem | Try this |
|---------|----------|
| Knob does nothing | Check middle leg is in `A0`, outer legs in `5V` and `GND`. |
| Button always firing | Move one of its wires to a different corner leg. |
| "Connect Uno" shows nothing | Use Chrome/Edge. Unplug any other USB gadgets. Re-plug the Uno. |
| Pitch goes the wrong way | Swap the knob's `5V` and `GND` wires. |
| Nothing happens at all | Is the green dot in the app on? If not, click 🔌 Connect Uno again. |

---

## Want more later? (optional, needs the breadboard)

Once comfortable, you can add: a 2nd & 3rd knob (brake + throttle), more buttons, LEDs, a buzzer, and the LCD screen. That's the bigger build — ask and we'll wire it up. For now, **1 knob + 2 buttons is enough to play.**
