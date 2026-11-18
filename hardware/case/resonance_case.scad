// Resonance enclosure for the ESP32-2432S032C and a 90 x 60 x 11 mm LiPo cell.
// RES-HW-007 (docs/requirements/resonance.md). Read README.md first.
//
// DRAFT v0: every part renders as a valid solid (OpenSCAD 2025.02.11), but
// nothing has been printed, and the positions of the openings are guesses
// until the board has been measured (the "TO BE MEASURED" block below).
//
// Printed parts, stacked from the screen backwards. Every part prints flat
// on the bed without supports:
//
//   front   the bezel shell: screen window, the board on four bosses, the
//           openings for USB-C, the microSD card and the power button
//   spacer  four tubes between the board and the tray (print four)
//   tray    the battery tray: its floor is the divider between the cell and
//           the board
//   lid     the back cover over the cell
//   button  the push-button that reaches the board's power switch
//
// Four countersunk M3 screws go through the tray floor, the spacers and the
// board's mounting holes into the front bosses and hold everything; their
// heads sit flush under the cell. Two more screws hold the lid.
//
// Coordinates: X along the board's long side, from the inside of the wall at
// the USB-C end; Y across, from the inside of the wall on the microSD side;
// Z from the outer face of the screen side towards the back.
//
// Choose what to render:  "assembly", "front", "spacer", "tray", "lid" or "button".
part = "assembly";

// ---- From the manufacturer's drawing (Jingcai specification, 2023) ---------
pcb_l = 93.7;          // board length
pcb_w = 55.0;          // board width
hole_inset = 3.5;      // mounting hole centres from the board edges
hole_d = 3.2;          // mounting hole diameter
lcd_active_l = 64.8;   // display active area, along X
lcd_active_w = 48.6;   // display active area, along Y

// ---- From the operator ------------------------------------------------------
bat_l = 90;            // battery length ("approximately": measure)
bat_w = 60;            // battery width
bat_t = 11;            // battery thickness

// ---- TO BE MEASURED on the board (RES-PROC-002). The values are guesses. ----
pcb_t = 1.6;           // board thickness
lcd_h = 4.6;           // front of the board to the top of the screen glass
rear_h = 7.0;          // tallest part on the back of the board, plus 1 mm
lcd_offset_x = 14.4;   // board edge at the USB-C end to the active area
lcd_offset_y = 3.2;    // board edge on the microSD side to the active area
usb_y = 27.5;          // centre of the USB-C socket from the microSD-side edge
usb_z = 1.7;           // centre of the USB-C socket above the back of the board
sd_x = 47.0;           // centre of the microSD slot from the USB-C end
btn_x = 20.0;          // centre of the power button from the USB-C end
btn_z = 1.5;           // centre of the power button above the back of the board
bat_lead_x = 12.0;     // battery connector from the USB-C end (for the wire slot)

// ---- Design choices ---------------------------------------------------------
wall = 2.0;            // side walls
front_t = 1.6;         // thickness of the screen-side face
floor_t = 2.0;         // tray floor: the divider between cell and board; thick enough for the countersinks
lid_t = 1.6;
corner_r = 3.0;        // outer corner radius
fit = 0.25;            // clearance between printed parts that slide together
pcb_gap = 0.5;         // board to the walls at the USB-C end and the microSD side
bat_gap = 0.8;         // room around and above the cell
window_margin = 0.8;   // screen window larger than the active area by this, each side
boss_d = 7.0;          // bosses and columns at the board's mounting holes
pilot_d = 2.5;         // hole an M3 screw cuts its own thread into
clear_d = 3.4;         // hole an M3 screw passes through
head_d = 6.4;          // countersink / recess diameter for an M3 screw head
csk_h = 1.7;           // depth of the countersink for a flat-head M3
usb_open_w = 13.0;     // opening for the plug's moulding, not just the socket
usb_open_h = 7.0;
sd_open_w = 15.0;
sd_open_h = 3.2;
btn_d = 4.0;           // push-button shaft
btn_flange_d = 6.5;
btn_flange_t = 1.0;
btn_out = 1.2;         // how far the button stands out of the wall

// ---- Derived ----------------------------------------------------------------
cav_l = max(pcb_l + 2 * pcb_gap, bat_l + 2 * 5);       // 5 mm each end for the lid screws
cav_w = max(pcb_w + 2 * pcb_gap, bat_w + 2 * bat_gap);
out_l = cav_l + 2 * wall;
out_w = cav_w + 2 * wall;
front_h = front_t + lcd_h + pcb_t + rear_h;            // height of the front shell
tray_h = floor_t + bat_t + bat_gap;                    // height of the tray
pcb_front_z = front_t + lcd_h;                         // screen side of the board
pcb_rear_z = pcb_front_z + pcb_t;                      // component side of the board
pcb_x0 = pcb_gap;                                      // board origin inside the cavity
pcb_y0 = pcb_gap;
btn_gap = cav_w - pcb_y0 - pcb_w;                      // board edge to the wall, button side
btn_len = btn_gap + wall + btn_out - 0.6;              // shaft length; 0.6 mm of free travel

holes = [
    [pcb_x0 + hole_inset, pcb_y0 + hole_inset],
    [pcb_x0 + pcb_l - hole_inset, pcb_y0 + hole_inset],
    [pcb_x0 + hole_inset, pcb_y0 + pcb_w - hole_inset],
    [pcb_x0 + pcb_l - hole_inset, pcb_y0 + pcb_w - hole_inset]
];

$fn = 48;
eps = 0.01;

// A box with rounded vertical edges. Origin at the outer corner.
module rounded_box(l, w, h, r) {
    hull() {
        for (x = [r, l - r], y = [r, w - r])
            translate([x, y, 0]) cylinder(r = r, h = h);
    }
}

// The outer shell footprint, positioned so that the cavity starts at X = 0, Y = 0.
module shell(h) {
    translate([-wall, -wall, 0]) rounded_box(out_l, out_w, h, corner_r);
}

// ---- front ------------------------------------------------------------------
module front() {
    difference() {
        union() {
            difference() {
                shell(front_h);
                // The cavity for screen, board and components.
                translate([0, 0, front_t]) cube([cav_l, cav_w, front_h]);
            }
            // Bosses the board rests on, at its mounting holes.
            for (p = holes)
                translate([p[0], p[1], front_t - eps]) cylinder(d = boss_d, h = lcd_h + eps);
        }
        // Screen window.
        translate([pcb_x0 + lcd_offset_x - window_margin, pcb_y0 + lcd_offset_y - window_margin, -eps])
            cube([lcd_active_l + 2 * window_margin, lcd_active_w + 2 * window_margin, front_t + 2 * eps]);
        // Pilot holes for the four screws. They stop 0.8 mm short of the face.
        for (p = holes)
            translate([p[0], p[1], 0.8]) cylinder(d = pilot_d, h = front_h);
        // USB-C, through the wall at X = 0.
        translate([-wall - eps, pcb_y0 + usb_y - usb_open_w / 2, pcb_rear_z + usb_z - usb_open_h / 2])
            cube([wall + 2 * eps, usb_open_w, usb_open_h]);
        // microSD, through the wall at Y = 0.
        translate([pcb_x0 + sd_x - sd_open_w / 2, -wall - eps, pcb_rear_z - 0.6])
            cube([sd_open_w, wall + 2 * eps, sd_open_h]);
        // A scallop for the fingernail that pulls the card.
        translate([pcb_x0 + sd_x, -wall - 4, pcb_rear_z + 1]) sphere(d = 12);
        // Power button, through the wall on the far side.
        translate([pcb_x0 + btn_x, cav_w - eps, pcb_rear_z + btn_z])
            rotate([-90, 0, 0]) cylinder(d = btn_d + 2 * fit, h = wall + 2 * eps);
    }
}

// ---- spacer -----------------------------------------------------------------
// A tube between the back of the board and the tray floor. Print four.
module spacer() {
    difference() {
        cylinder(d = boss_d, h = rear_h);
        translate([0, 0, -eps]) cylinder(d = clear_d, h = rear_h + 2 * eps);
    }
}

// ---- tray -------------------------------------------------------------------
// Drawn in assembly position: its floor starts where the front shell ends.
module tray() {
    difference() {
        union() {
            // Walls and floor.
            translate([0, 0, front_h]) difference() {
                shell(tray_h);
                translate([0, 0, floor_t]) cube([cav_l, cav_w, tray_h]);
            }
            // Bosses for the two lid screws, in the middle of each end wall.
            for (x = [0, cav_l])
                translate([x, cav_w / 2, front_h + floor_t - eps])
                    intersection() {
                        cylinder(d = 8, h = tray_h - floor_t);
                        translate([x == 0 ? 0 : -4, -4, 0]) cube([4, 8, tray_h]);
                    }
        }
        // Board screws: through the floor, countersunk on the cell's side so
        // that nothing stands up under the cell.
        for (p = holes) {
            translate([p[0], p[1], front_h - eps]) cylinder(d = clear_d, h = floor_t + 2 * eps);
            translate([p[0], p[1], front_h + floor_t - csk_h]) cylinder(d1 = clear_d, d2 = head_d, h = csk_h + eps);
        }
        // Slot in the floor for the battery lead.
        translate([pcb_x0 + bat_lead_x - 5, cav_w - 6, front_h - eps]) cube([10, 4, floor_t + 2 * eps]);
        // Pilot holes for the lid screws.
        for (x = [1.9, cav_l - 1.9])
            translate([x, cav_w / 2, front_h + floor_t + 1]) cylinder(d = pilot_d, h = tray_h);
    }
}

// ---- lid --------------------------------------------------------------------
module lid() {
    z0 = front_h + tray_h;
    lip_h = 1.6;
    lip_t = 1.2;
    difference() {
        union() {
            translate([0, 0, z0]) shell(lid_t);
            // Rim that sits inside the tray walls.
            translate([fit, fit, z0 - lip_h]) difference() {
                cube([cav_l - 2 * fit, cav_w - 2 * fit, lip_h + eps]);
                translate([lip_t, lip_t, -eps])
                    cube([cav_l - 2 * fit - 2 * lip_t, cav_w - 2 * fit - 2 * lip_t, lip_h + 3 * eps]);
            }
        }
        // Room for the two screw bosses of the tray.
        for (x = [0, cav_l])
            translate([x, cav_w / 2, z0 - lip_h - eps]) cylinder(d = 8 + 2 * fit, h = lip_h + eps);
        // Screw holes with a recess for the heads.
        for (x = [1.9, cav_l - 1.9]) {
            translate([x, cav_w / 2, z0 - eps]) cylinder(d = clear_d, h = lid_t + 2 * eps);
            translate([x, cav_w / 2, z0 + lid_t - 1.0]) cylinder(d = head_d, h = 1.0 + eps);
        }
    }
}

// ---- button -----------------------------------------------------------------
// Put in from the inside before the board: the flange keeps it from falling
// out, and the board's switch stops it going in.
module button() {
    cylinder(d = btn_flange_d, h = btn_flange_t);
    cylinder(d = btn_d, h = btn_len);
}

module button_in_place() {
    translate([pcb_x0 + btn_x, cav_w - btn_gap + 0.6, pcb_rear_z + btn_z])
        rotate([-90, 0, 0]) button();
}

// ---- output -----------------------------------------------------------------
if (part == "front") front();
else if (part == "spacer") spacer();
else if (part == "tray") translate([0, 0, -front_h]) tray();          // floor on the bed
else if (part == "lid") translate([0, out_w - 2 * wall, front_h + tray_h + lid_t]) rotate([180, 0, 0]) lid();  // outer face on the bed
else if (part == "button") button();
else {
    front();
    for (p = holes) translate([p[0], p[1], pcb_rear_z]) spacer();
    tray();
    lid();
    button_in_place();
    // The board and the cell, for checking clearances.
    %translate([pcb_x0, pcb_y0, pcb_front_z]) cube([pcb_l, pcb_w, pcb_t]);
    %translate([(cav_l - bat_l) / 2, (cav_w - bat_w) / 2, front_h + floor_t]) cube([bat_l, bat_w, bat_t]);
}

echo(str("Outer size: ", out_l, " x ", out_w, " x ", front_h + tray_h + lid_t, " mm"));
echo(str("Board screws: countersunk M3, about ", floor_t + rear_h + pcb_t + 4, " mm long (no longer than ",
         floor_t + rear_h + pcb_t + lcd_h + front_t - 0.8, ")"));
