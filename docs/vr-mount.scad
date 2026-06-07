// RhythmRide — Phone Visor Mount (no lenses, ~15 min print)
// ============================================================
// Headband clip + phone cradle. Phone at ~28cm from eyes — readable without lenses.
// Print settings: PLA, 0.3mm layer, 20% infill, no supports.
// Two parts: clip + cradle. Hot-glue together after printing.
//
// FASTEST ALTERNATIVE (no print): tape phone to front of baseball cap brim,
// rubber band around head. 2 minutes, same effect.

phone_w = 76;   // phone width mm (add 2mm for case)
phone_h = 10;   // phone thickness mm
wall    = 3;
clip_w  = 32;
clip_h  = 18;   // fits 10-15mm elastic headband
clip_t  = 4;

// PART 1 — Headband clip (8 min print)
// Thread elastic/headband through the slot, knot to secure
module headband_clip() {
    difference() {
        cube([clip_w + 2*clip_t, clip_h + 2*clip_t, wall*2]);
        translate([clip_t, clip_t, -0.1])
            cube([clip_w, clip_h, wall*2 + 0.2]);
    }
    // Retaining lips so elastic doesn't slip
    translate([0, 0, wall*2])
        cube([clip_t, clip_h + 2*clip_t, wall]);
    translate([clip_w + clip_t, 0, wall*2])
        cube([clip_t, clip_h + 2*clip_t, wall]);
}

// PART 2 — Phone cradle (8 min print)
// Phone slides in from top in landscape orientation
module phone_cradle() {
    cw = phone_w + 4;
    ct = phone_h + 2;
    ch = 28;
    difference() {
        cube([cw + 2*wall, ch + wall, ct + wall*2]);
        translate([wall, wall, wall])
            cube([cw, ch + 1, ct]);
        // Open slot at top for easy insert/remove
        translate([wall + 12, ch - 1, -0.1])
            cube([cw - 24, wall + 2, ct + wall*2 + 0.2]);
    }
    // Bottom shelf
    translate([wall, 0, wall])
        cube([cw, wall, ct]);
}

// Render both parts side by side
headband_clip();
translate([0, 55, 0]) phone_cradle();

// Assembly:
// 1. Print both parts (can run simultaneously on 2 printers)
// 2. Hot-glue cradle onto clip at ~15° forward angle
// 3. Thread elastic headband through clip slot
// 4. Slide phone in landscape — screen faces you at ~28cm
// 5. Wear on forehead — AR game is fully visible without lenses
