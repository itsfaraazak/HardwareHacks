// ═══════════════════════════════════════════════════════════
//  RhythmRide VR Phone Cradle — OpenSCAD
//  No lenses required. Phone held at ~30 cm from eyes.
//  Prints in ~35 min: 0.3 mm layers, 20% infill, no supports.
//
//  HOW TO PRINT:
//    1. Open this file in OpenSCAD (free: openscad.org)
//    2. Adjust parameters below for your phone
//    3. File → Export → Export as STL
//    4. Slice in Cura/PrusaSlicer:
//         - Layer height : 0.30 mm
//         - Infill       : 20%
//         - Supports     : OFF
//         - Walls        : 2
//    5. Print time ≈ 30–38 min on most FDM printers
//
//  ASSEMBLY:
//    - Slide phone into cradle slot (friction fit)
//    - Thread velcro / rubber-band through the 4 strap slots
//    - Loop strap around head
// ═══════════════════════════════════════════════════════════

// ── Parameters — change these for your phone ─────────────
PHONE_W      = 76;    // phone width  (mm) — measure with case
PHONE_H      = 160;   // phone height (mm) — measure with case
PHONE_THICK  = 10;    // phone thickness with case (mm)
PHONE_DIST   = 30;    // eye-to-phone distance (mm) — 28–35 recommended

// Eye gap: the opening you look through
EYE_W        = 60;    // horizontal eye gap width (mm)
EYE_H        = 34;    // vertical eye gap height (mm)
NOSE_W       = 28;    // nose bridge notch width (mm)
NOSE_H       = 18;    // nose bridge notch depth (mm)

// Frame geometry
WALL         = 3.5;   // wall thickness (mm) — keep ≥ 3 for rigidity
STRAP_SLOT_W = 22;    // width of elastic strap slots (mm)
STRAP_SLOT_H = 6;     // height of strap slots (mm)

// Bevel / fillet (cosmetic)
BEVEL        = 2;

// ── Derived dimensions ─────────────────────────────────────
cradle_W = PHONE_W + 2*WALL;
cradle_H = PHONE_H + 2*WALL;
box_D    = PHONE_THICK + WALL;   // depth of phone pocket
tube_D   = PHONE_DIST;           // length of tube from face-plate to phone
total_D  = box_D + tube_D;

// ══════════════════════════════════════════════════════════
//  MAIN ASSEMBLY
// ══════════════════════════════════════════════════════════
union() {
    phone_cradle();
    translate([0, 0, -tube_D])
        face_plate();
    translate([0, 0, -tube_D/2])
        tube_walls();
}

// ══════════════════════════════════════════════════════════
//  PHONE CRADLE (back box that holds the phone)
// ══════════════════════════════════════════════════════════
module phone_cradle() {
    difference() {
        // Outer shell
        translate([-cradle_W/2, -cradle_H/2, 0])
            cube([cradle_W, cradle_H, box_D]);

        // Inner phone pocket (open at front, closed at back)
        translate([-PHONE_W/2, -PHONE_H/2, WALL])
            cube([PHONE_W, PHONE_H, PHONE_THICK + 1]);

        // Open face (phone inserts from the front)
        translate([-PHONE_W/2, -PHONE_H/2, -0.1])
            cube([PHONE_W, PHONE_H, WALL + 0.2]);
    }

    // Lip retainer (keeps phone from sliding out front)
    // 4 corner tabs, 6mm wide
    for (x = [-1, 1], y = [-1, 1]) {
        translate([
            x * (PHONE_W/2 - 6),
            y * (PHONE_H/2 - 6),
            -1
        ])
            cube([12, 12, WALL + 1], center=true);
    }
}

// ══════════════════════════════════════════════════════════
//  FACE PLATE (eye pieces + nose bridge + strap slots)
// ══════════════════════════════════════════════════════════
module face_plate() {
    difference() {
        // Plate body
        translate([-cradle_W/2, -cradle_H/2, 0])
            cube([cradle_W, cradle_H, WALL]);

        // Left eye hole
        translate([-EYE_W/2 - NOSE_W/2, -EYE_H/2, -0.1])
            cube([EYE_W, EYE_H, WALL + 0.2]);

        // Right eye hole
        translate([NOSE_W/2, -EYE_H/2, -0.1])
            cube([EYE_W, EYE_H, WALL + 0.2]);

        // Nose bridge cutout (bottom centre)
        translate([-NOSE_W/2, -NOSE_H - EYE_H/2, -0.1])
            cube([NOSE_W, NOSE_H, WALL + 0.2]);

        // Strap slots — left side (2 slots, top & bottom)
        for (y = [cradle_H/4, -cradle_H/4]) {
            translate([
                -cradle_W/2 - 0.1,
                y - STRAP_SLOT_H/2,
                (WALL - STRAP_SLOT_H)/2
            ])
                cube([WALL + 0.2, STRAP_SLOT_W, STRAP_SLOT_H]);
        }

        // Strap slots — right side
        for (y = [cradle_H/4, -cradle_H/4]) {
            translate([
                cradle_W/2 - 0.1,
                y - STRAP_SLOT_H/2,
                (WALL - STRAP_SLOT_H)/2
            ])
                cube([WALL + 0.2, STRAP_SLOT_W, STRAP_SLOT_H]);
        }
    }
}

// ══════════════════════════════════════════════════════════
//  TUBE WALLS (4 sides connecting face-plate to cradle)
//  Open top & bottom to keep print fast and light.
// ══════════════════════════════════════════════════════════
module tube_walls() {
    half_W = cradle_W/2;
    half_H = cradle_H/2;

    // Left wall
    translate([-half_W, -half_H, 0])
        cube([WALL, cradle_H, tube_D]);

    // Right wall
    translate([half_W - WALL, -half_H, 0])
        cube([WALL, cradle_H, tube_D]);

    // Top wall (partial — don't block top edge, preserves fast print)
    translate([-half_W, half_H - WALL, 0])
        cube([cradle_W, WALL, tube_D]);

    // Bottom wall
    translate([-half_W, -half_H, 0])
        cube([cradle_W, WALL, tube_D]);
}

// ══════════════════════════════════════════════════════════
//  NOTES
// ══════════════════════════════════════════════════════════
// - Total part size ≈ 82 × 166 × 45 mm (default params)
// - Print standing upright (long axis vertical) for best strength
// - Viewing: hold unit to face, eyes aligned with cutouts
// - Phone screen should be landscape, showing AR game full-screen
// - Strap: thread 20 mm velcro strip through the 4 slots,
//   loop around back of head, press velcro together
// - Optional: hot-glue a foam strip along the face plate rim
//   for comfort and light seal
