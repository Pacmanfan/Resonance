// Resonance enclosure, VERSION 4, for the ESP32-2432S032C and a
// 90 x 60 x 11 mm LiPo cell. RES-HW-007. Read README.md first.
//
// Version 4 is version 3 with side walls of 2.0 mm instead of 1.5 mm
// (operator, 2026-11-18: the faces are thick enough, the walls were not).
// The inside is unchanged; the case is 1 mm longer and 1 mm wider.
//
// Version 3 is version 2 with the window cut to the glass as the operator
// measured it on 2026-11-18: 77.6 x 55.0 mm, 4.0 mm above the board. The
// glass is as wide as the board, so the window reaches the wall on the
// microSD side.
//
// Changes from version 1 (operator, 2026-11-18, after printing v1):
//   - two shells that snap together; no tray, no lid, no case screws
//   - no divider between the cell and the board (a sheet of paper or foam
//     goes between them)
//   - the screen's glass stands in the front panel, flush with its face
//   - walls and faces 0.5 mm thinner (1.5 mm)
//   - the board's four bosses take M3 heat-set inserts
//   - the button's stem is 0.5 mm longer
//   - the edges of the front and back faces are rounded
// Unchanged: the positions of USB-C, the microSD slot and the button, which
// lined up on the printed v1.
//
// Printed parts:
//   front   screen window, four bosses with inserts for the board, openings
//   back    the cup over the cell, with a skirt that snaps into the front
//   button  the push-button that reaches the board's power switch
//
// Coordinates: X along the board's long side, from the inside of the wall at
// the USB-C end; Y across, from the inside of the wall on the microSD side;
// Z from the outer face of the screen side towards the back.
//
// Choose what to render: "assembly", "exploded", "section", "front", "back"
// or "button".
part = "assembly";

include <battery.scad>          // bat_l, bat_w, bat_t and battery()
battery_demo = false;
use <esp32_2432s032.scad>       // board_reference()

// ---- The board (sources: esp32_2432s032.scad, README.md) --------------------
pcb_l = 93.7;
pcb_w = 55.0;
pcb_t = 1.6;
hole_inset = 3.5;
lcd_h = 4.0;           // front of the board to the top of the glass: measured
// The glass: size measured (77.6 x 55.0). Its position along the board is
// not measured: it is centred where the drawing has the 76.6 mm outline.
glass_x = 8.3;         // from the board's edge at the USB-C end
glass_l = 77.6;
glass_y = 0.0;         // from the board's edge on the microSD side
glass_w = 55.0;
glass_fit = 0.4;       // room round the glass, each side
usb_y = 27.5;          // confirmed by the printed v1
usb_z = 1.65;
sd_x = 55.1;           // confirmed by the printed v1
btn_x = 24.8;          // confirmed by the printed v1
btn_z = 1.6;
led_x = 89.5;
led_y = 11.7;
ldr_x = 90.1;
ldr_y = 42.4;

// ---- Design choices ---------------------------------------------------------
wall = 2.0;            // side walls (v3: 1.5)
front_t = 1.5;         // face round the glass (v1: 1.6)
back_t = 1.5;          // back face (v1: 1.6)
corner_r = 4.5;        // corners, seen from the front (v2: 3.0)
edge_r = 2.2;          // rounding of the edges of the front and back faces (v2: 1.2)
fit = 0.2;             // clearance between printed parts
pcb_gap = 0.5;         // board to the walls at the USB-C end and the microSD side
bat_gap = 1.5;         // room at each side of the cell and above it
bat_end = 6.0;         // room at each end of the cell (leads, plug)
comp_h = 5.5;          // back of the board to the cell: components (about 4), the barrier sheet, foam
boss_d = 7.0;          // bosses at the board's mounting holes
insert_d = 4.0;        // hole for an M3 heat-set insert, 4.2 mm outside, 3 mm long
insert_depth = 3.4;
led_window_d = 4.0;
ldr_window_d = 3.5;
usb_open_w = 13.0;
usb_open_h = 7.0;
sd_open_w = 15.0;
sd_open_h = 3.2;
// The snap joint: a skirt on the back shell goes inside the front shell's
// walls; ridges on the skirt click into grooves in the walls.
skirt_h = 3.0;
skirt_t = 1.0;
snap_grip = 0.4;       // how far a ridge reaches into its groove. Tighter: larger. Change by 0.1
snap_z = 1.6;          // ridge centre below the seam
// The power button.
btn_pad_t = 3.0;       // the wall is this much thicker, inside, where the button goes through
btn_pad_w = 16.0;
btn_dish_d = 8.0;
btn_dish_depth = 0.6;
btn_shaft_d = 5.0;
btn_flange_d = 7.0;    // keeps the button in the case
btn_flange_t = 0.8;
btn_proud = 0.4;       // the button's end above the floor of its dish
btn_stem_d = 3.2;
btn_overhang = 0.6;    // the switch's actuator beyond the board's edge
btn_travel = 0.0;      // v1 had 0.5 and the button was "very slightly too short"

// ---- Derived ----------------------------------------------------------------
cav_l = max(pcb_l + 2 * pcb_gap, bat_l + 2 * bat_end);
cav_w = max(pcb_w + 2 * pcb_gap, bat_w + 2 * bat_gap);
out_l = cav_l + 2 * wall;
out_w = cav_w + 2 * wall;
pcb_front_z = lcd_h;                       // the glass is flush with Z = 0
pcb_rear_z = pcb_front_z + pcb_t;
cell_z = pcb_rear_z + comp_h;              // the face of the cell nearest the board
total_h = cell_z + bat_t + bat_gap + back_t;
front_h = 13.0;                            // the seam between the shells
back_h = total_h - front_h;
pcb_x0 = pcb_gap;
pcb_y0 = pcb_gap;
bat_x0 = (cav_l - bat_l) / 2;
bat_y0 = (cav_w - bat_w) / 2;
btn_gap = cav_w - pcb_y0 - pcb_w;
btn_cone_h = (btn_flange_d - btn_shaft_d) / 2;
btn_shaft_len = btn_pad_t + wall - btn_dish_depth - btn_cone_h + btn_proud;
btn_stem_len = btn_gap - btn_pad_t - btn_flange_t - btn_overhang - btn_travel;
btn_y_in = cav_w - btn_pad_t;
btn_y_out = cav_w + wall;
btn_zc = pcb_rear_z + btn_z;
usb_z0 = pcb_rear_z + usb_z - usb_open_h / 2;
usb_z1 = usb_z0 + usb_open_h;
usb_yc = pcb_y0 + usb_y;
win_y0 = pcb_y0 + glass_y - glass_fit < 0.3 ? 0 : pcb_y0 + glass_y - glass_fit;
cav_r = corner_r - wall;                   // inside corners, so that the wall is 1.5 mm there too

holes = [for (x = [hole_inset, pcb_l - hole_inset], y = [hole_inset, pcb_w - hole_inset]) [pcb_x0 + x, pcb_y0 + y]];
// Ribs on the button side that the cell's edge rests on, beside the board.
rib_x = [42, 72];
rib_t = 1.6;
rib_len = 6.0;
// Snap ridges: [wall, centre along the wall, length]. Clear of the USB-C
// notch, the card slot, the button's pad and the ribs.
snaps = [
    ["y0", 25, 14], ["y0", 82, 14],
    ["y1", 57, 14], ["y1", 88, 14],
    ["x0", 11, 10], ["x0", cav_w - 11, 10],
    ["x1", cav_w / 2, 18]
];

$fn = 48;
eps = 0.01;

// The outer solid of a shell: rounded corners, and the edge of its lower
// face rounded with edge_r. The cavity starts at X = 0, Y = 0.
module shell_solid(h) {
    profile = concat([[0, 0]],
                     [for (a = [0 : 15 : 90]) [corner_r - edge_r + edge_r * sin(a), edge_r - edge_r * cos(a)]],
                     [[corner_r, h], [0, h]]);
    hull() for (x = [corner_r, out_l - corner_r], y = [corner_r, out_w - corner_r])
        translate([x - wall, y - wall, 0]) rotate_extrude() polygon(profile);
}

// A box with rounded vertical edges, inset by `inset` from the cavity walls.
module cavity_box(inset, h) {
    r = max(cav_r - inset, 0.2);
    hull() for (x = [inset + r, cav_l - inset - r], y = [inset + r, cav_w - inset - r])
        translate([x, y, 0]) cylinder(r = r, h = h);
}

// A ridge of triangular section lying along a wall, pointing into the wall.
// `off` is how far inside the wall's inner face its base is.
module wall_prism(side, centre, len, depth, half_w, off, z) {
    module prism() rotate([90, 0, 90]) linear_extrude(len, center = true)
        polygon([[0, -half_w], [-depth, 0], [0, half_w]]);
    if (side == "y0") translate([centre, off, z]) prism();
    if (side == "y1") translate([centre, cav_w - off, z]) mirror([0, 1, 0]) prism();
    if (side == "x0") translate([off, centre, z]) rotate([0, 0, -90]) prism();
    if (side == "x1") translate([cav_l - off, centre, z]) rotate([0, 0, 90]) prism();
}

// ---- front ------------------------------------------------------------------
module front() {
    difference() {
        union() {
            difference() {
                shell_solid(front_h);
                translate([0, 0, front_t]) cavity_box(0, front_h);
            }
            // The thicker patch of wall the button goes through. It ends
            // where the cell begins.
            translate([pcb_x0 + btn_x - btn_pad_w / 2, btn_y_in, front_t - eps])
                cube([btn_pad_w, btn_pad_t + eps, cell_z - front_t + eps]);
            // Bosses the board rests on.
            for (p = holes) translate([p[0], p[1], front_t - eps]) cylinder(d = boss_d, h = lcd_h - front_t + eps);
            // Ribs for the edge of the cell.
            for (x = rib_x) translate([x - rib_t / 2, cav_w - rib_len, front_t - eps])
                cube([rib_t, rib_len + eps, cell_z - front_t + eps]);
        }
        // The window the glass stands in.
        // Where less than 0.3 mm of face would be left beside it, it runs
        // on to the wall.
        translate([pcb_x0 + glass_x - glass_fit, win_y0, -eps])
            cube([glass_l + 2 * glass_fit, pcb_y0 + glass_y + glass_w + glass_fit - win_y0, front_t + 2 * eps]);
        // Windows over the RGB LED and the light sensor.
        translate([pcb_x0 + led_x, pcb_y0 + led_y, -eps]) cylinder(d1 = led_window_d + 1.6, d2 = led_window_d, h = front_t + 2 * eps);
        translate([pcb_x0 + ldr_x, pcb_y0 + ldr_y, -eps]) cylinder(d1 = ldr_window_d + 1.6, d2 = ldr_window_d, h = front_t + 2 * eps);
        // Holes for the heat-set inserts.
        for (p = holes) translate([p[0], p[1], lcd_h - insert_depth]) cylinder(d = insert_d, h = insert_depth + eps);
        // USB-C: a hole with round ends. The socket stands 1.1 mm over the
        // board's edge, so the board goes in USB-C end first, at a slant.
        hull() for (y = [-1, 1])
            translate([-wall - eps, usb_yc + y * (usb_open_w - usb_open_h) / 2, usb_z0 + usb_open_h / 2])
                rotate([0, 90, 0]) cylinder(d = usb_open_h, h = wall + 2 * eps);
        // microSD slot and a scallop for the fingernail.
        translate([pcb_x0 + sd_x - sd_open_w / 2, -wall - eps, pcb_rear_z - 0.6]) cube([sd_open_w, wall + 2 * eps, sd_open_h]);
        translate([pcb_x0 + sd_x, -wall - 4.8, pcb_rear_z + 1]) sphere(d = 12);
        // Button: hole, a 45 degree seat for its shoulder, finger dish.
        translate([pcb_x0 + btn_x, 0, btn_zc]) {
            translate([0, btn_y_in - eps, 0]) rotate([-90, 0, 0])
                cylinder(d = btn_shaft_d + 2 * fit, h = btn_pad_t + wall + 2 * eps);
            translate([0, btn_y_in - eps, 0]) rotate([-90, 0, 0])
                cylinder(d1 = btn_flange_d + 2 * fit, d2 = btn_shaft_d + 2 * fit, h = btn_cone_h + eps);
            translate([0, btn_y_out - btn_dish_depth, 0]) rotate([-90, 0, 0])
                cylinder(d = btn_dish_d, h = btn_dish_depth + eps);
        }
        // Grooves for the snap ridges.
        for (s = snaps) wall_prism(s[0], s[1], s[2] + 1, snap_grip + 0.1, 0.75, eps, front_h - snap_z);
    }
}

// ---- back -------------------------------------------------------------------
// Drawn in its place on the front shell.
module back() {
    difference() {
        union() {
            difference() {
                translate([0, 0, total_h]) mirror([0, 0, 1]) shell_solid(back_h);
                translate([0, 0, front_h - eps]) cavity_box(0, back_h - back_t + eps);
            }
            // The skirt that goes inside the front shell's walls.
            translate([0, 0, front_h - skirt_h]) difference() {
                cavity_box(fit, skirt_h + 1);
                translate([0, 0, -eps]) cavity_box(fit + skirt_t, skirt_h + 1 + 2 * eps);
            }
            // The band that joins the skirt to the walls of the back shell.
            // It tapers at 45 degrees so that it prints without support.
            translate([0, 0, front_h - eps]) difference() {
                cavity_box(0, 1 + fit + skirt_t);
                hull() {
                    translate([0, 0, -eps]) cavity_box(fit + skirt_t, 1);
                    translate([0, 0, 1 + fit + skirt_t]) cavity_box(0, eps);
                }
            }
            // Snap ridges on the skirt.
            for (s = snaps) wall_prism(s[0], s[1], s[2], fit + snap_grip, 0.6, fit + eps, front_h - snap_z);
            // Stops that keep the cell from sliding lengthways, clear of its leads.
            for (x = [bat_x0 - 2.2, bat_x0 + bat_l + 1.0])
                translate([x, cav_w / 2 - 7, front_h + 1]) cube([1.2, 14, back_h - back_t - 1 + eps]);
        }
        // The skirt gives way to the button's pad and to the ribs.
        translate([pcb_x0 + btn_x - btn_pad_w / 2 - 0.4, btn_y_in - 0.4, 0]) cube([btn_pad_w + 0.8, btn_pad_t + 1, cell_z + 0.3]);
        for (x = rib_x) translate([x - rib_t / 2 - 0.4, cav_w - rib_len - 0.4, 0]) cube([rib_t + 0.8, rib_len + 1, cell_z + 0.3]);
        // The skirt stays clear of the USB-C hole.
        translate([-eps, usb_yc - usb_open_w / 2, 0]) cube([fit + skirt_t + 1, usb_open_w, usb_z1 + 0.3]);
        // A slot at the seam, at the end away from USB-C, for a coin or a
        // fingernail to open the case.
        translate([cav_l + wall - 0.9, cav_w / 2 - 6, front_h - eps]) cube([1, 12, 1.2]);
    }
}

// ---- button -----------------------------------------------------------------
// Drawn as it is printed: the end that shows on the bed. Put it in from the
// inside before the board.
module button() {
    cylinder(d = btn_shaft_d, h = btn_shaft_len);
    translate([0, 0, btn_shaft_len]) cylinder(d1 = btn_shaft_d, d2 = btn_flange_d, h = btn_cone_h);
    translate([0, 0, btn_shaft_len + btn_cone_h]) cylinder(d = btn_flange_d, h = btn_flange_t);
    translate([0, 0, btn_shaft_len + btn_cone_h + btn_flange_t]) cylinder(d = btn_stem_d, h = btn_stem_len);
}
module button_in_place() {
    translate([pcb_x0 + btn_x, btn_y_out - btn_dish_depth + btn_proud, btn_zc]) rotate([90, 0, 0]) button();
}

// ---- reference parts (not printed) --------------------------------------------
module board_in_place() { translate([pcb_x0, pcb_y0, pcb_front_z]) board_reference(); }
module battery_in_place() { translate([bat_x0, bat_y0, cell_z]) battery(); }
module board_screws() {
    color("dimgray") for (p = holes) translate([p[0], p[1], pcb_rear_z]) cylinder(d = 5.6, h = 2.4);
}

cut_y = pcb_y0 + hole_inset;
module slice() {
    mirror([0, 1, 0]) projection(cut = true) translate([0, 0, -cut_y]) rotate([90, 0, 0]) children();
}

// ---- output -----------------------------------------------------------------
if (part == "front") front();
else if (part == "back") translate([0, cav_w, total_h]) rotate([180, 0, 0]) back();   // back face on the bed
else if (part == "button") button();
else if (part == "exploded") {
    gap = 18;
    color("steelblue") front();
    translate([0, 14, 0]) color("gold") button_in_place();
    translate([0, 0, gap]) { board_in_place(); board_screws(); }
    translate([0, 0, 2.2 * gap]) battery_in_place();
    translate([0, 0, 3.6 * gap]) color("slategray") back();
}
else if (part == "section") {
    color("steelblue") slice() front();
    color("slategray") slice() back();
    color("seagreen") slice() board_in_place();
    color("black") slice() board_screws();
    color("orange") slice() battery_in_place();
}
else {
    color("steelblue") front();
    color("slategray") back();
    color("gold") button_in_place();
    board_in_place();
    %battery_in_place();
}

echo(str("Outer size: ", out_l, " x ", out_w, " x ", total_h, " mm (v1: 106 x 67 x 30.3)"));
echo(str("Front shell ", front_h, " mm, back shell ", back_h, " mm; cell starts ", cell_z, " mm from the face"));
echo(str("Button: shaft ", btn_shaft_len, ", stem ", btn_stem_len, " mm"));
assert(btn_zc + btn_shaft_d / 2 + fit + 0.5 <= cell_z, "the button's hole breaks through the top of its pad");
assert(btn_zc + btn_flange_d / 2 <= cell_z, "the button's flange reaches into the cell's room");
assert(btn_zc + btn_dish_d / 2 + 0.6 <= front_h, "the button's dish reaches the seam");
assert(front_h - skirt_h >= pcb_rear_z + 4.0, "the skirt reaches down to the board's connectors");
assert(btn_stem_len > 1, "no room for the button's stem");
assert(pcb_x0 + glass_x - glass_fit >= holes[0][0] + boss_d / 2, "the window cuts into the bosses at the USB-C end");
assert(pcb_x0 + glass_x + glass_l + glass_fit <= holes[2][0] - boss_d / 2, "the window cuts into the bosses at the far end");
assert(usb_z1 + 1.2 <= front_h, "too little wall left over the USB-C hole");
