// Reference model of the ESP32-2432S032C board (Sunton 3.2-inch display
// board, capacitive touch, USB-C). Not a printed part: it is here to check
// the case against.
//
// WHERE THE NUMBERS COME FROM (details and links: README.md)
//   [D]  printed on the manufacturer's dimension drawing: outline 93.7 x 55,
//        holes 3.2 mm at 3.5 mm from the edges.
//   [P]  scaled from pictures: the same drawing (front) and the vendor's
//        rendering of the back. About +-0.3 mm, and a rendering is not a
//        photograph of the board on the bench.
//   [C]  cross-checked against the openings of two published cases for this
//        board (USB-C, microSD, power switch, RESET, BOOT, P1, P3, CN1 agree
//        within about 0.5 mm).
//   [E]  estimate: nothing published. ALL COMPONENT HEIGHTS are [E] or
//        typical values for the part type. Measure before trusting a
//        clearance in Z.
//
// No published 3D model of this board was found (2026-11-18).
//
// Coordinates of board_reference(): origin at the board's corner at the
// USB-C end on the microSD side, on the SCREEN side of the PCB. X along the
// length away from USB-C, Y across away from the microSD edge, Z from the
// screen side of the PCB towards the back; the screen is at negative Z.
// This is how resonance_case.scad looks at the board.
//
// The positions below are written in the frame of the back-view picture
// they were measured in (u from the antenna end, v from the power-switch
// edge) and converted by at(): x = 93.7 - u, y = 55 - v.

brd_l = 93.7;            // [D]
brd_w = 55.0;            // [D]
brd_t = 1.6;             // [E] usual thickness; one case's ledge implies it
brd_hole_inset = 3.5;    // [D]
brd_hole_d = 3.2;        // [D]
brd_corner_r = 3.5;      // [E] rounded corners, radius not dimensioned
brd_lcd_h = 4.0;         // measured: screen + touch glass above the PCB
brd_rear_h = 4.0;        // [C] upper bound: tallest part on the back

// Heights [E]
h_wroom = 3.1;           // module datasheet
h_usb = 3.2;             // USB-C mid-mount receptacle, typical
h_sd = 1.9;              // push-push microSD socket, typical
h_jst = 3.4;             // 1.25 mm side-entry header, typical
h_switch = 1.6;
h_tact = 2.0;            // RESET and BOOT
h_ic = 1.6;
h_fpc = 1.5;

// A block on the BACK of the board, given in the picture's frame:
// u0..u1 from the antenna end, v0..v1 from the power-switch edge.
module at(u0, u1, v0, v1, h, c = "dimgray") {
    color(c) translate([brd_l - u1, brd_w - v1, brd_t]) cube([u1 - u0, v1 - v0, h]);
}
// The same on the FRONT (screen side).
module at_front(u0, u1, v0, v1, h, c = "dimgray") {
    color(c) translate([brd_l - u1, brd_w - v1, -h]) cube([u1 - u0, v1 - v0, h]);
}

module board_pcb() {
    color("goldenrod") difference() {
        hull() for (x = [brd_corner_r, brd_l - brd_corner_r], y = [brd_corner_r, brd_w - brd_corner_r])
            translate([x, y, 0]) cylinder(r = brd_corner_r, h = brd_t, $fn = 32);
        for (x = [brd_hole_inset, brd_l - brd_hole_inset], y = [brd_hole_inset, brd_w - brd_hole_inset])
            translate([x, y, -1]) cylinder(d = brd_hole_d, h = brd_t + 2, $fn = 32);
        // Notch under the module's antenna. [P]
        translate([brd_l - 6.8, brd_w - 36.6, -1]) cube([6.8 + 1, 18.1, brd_t + 2]);
    }
}

module board_front() {
    // Display module with its touch glass. Size measured by the operator
    // (2026-11-18): 77.6 x 55.0, 4.0 above the board. Position along the
    // length [P], centred on the outline in the drawing.
    at_front(7.8, 85.4, 0.0, 55.0, brd_lcd_h, "black");
    // Lit area 64.8 x 48.6 (specification); position [P], +-0.5 along the length.
    color("lightskyblue") translate([brd_l - 76.0, brd_w - 51.9, -brd_lcd_h - 0.05]) cube([64.8 - 1.0, 48.4, 0.05]);
    // RGB LED (5050) and light sensor, beside the screen at the antenna end. [P][C]
    at_front(1.85, 6.55, 40.75, 45.85, 1.5, "white");
    at_front(2.7, 4.5, 11.1, 14.1, 1.0, "saddlebrown");
}

module board_back() {
    // ESP32-WROOM-32: module board and shield can. [P]; antenna over the notch.
    at(0.2, 25.7, 18.3, 36.4, 0.8, "black");
    at(6.3, 24.8, 19.2, 35.7, h_wroom, "silver");
    // USB-C: overhangs the end of the board by about 1.1 mm. [P][C]
    color("silver") translate([-1.1, 27.5 - 4.6, brd_t]) cube([8.4, 9.2, h_usb]);
    // microSD socket, flush with the edge. [P][C] The card's centre line is
    // about 1.2 mm off the socket's, at 55.1 mm from the USB-C end.
    at(30.0, 44.7, 40.5, 55.0, h_sd, "silver");
    // A card, pushed in: it stands about 2 mm out of the socket. [E]
    color("darkred") translate([brd_l - 38.6 - 5.5, -2, brd_t + 0.7]) cube([11, 12, 0.8]);
    // Power switch SW1 with its actuator over the edge. [P][C]
    at(66.8, 71.5, 0.6, 3.4, h_switch, "silver");
    at(67.9, 70.3, -0.6, 0.6, h_switch - 0.4, "black");
    // RESET and BOOT. [P][C]
    at(12.6, 16.2, 1.75, 9.25, h_tact, "silver");
    at(16.5, 20.1, 1.75, 9.25, h_tact, "silver");
    // 1.25 mm connectors. P3 and CN1 face the microSD edge, P1 the USB-C
    // end, speaker and battery the power-switch edge. [P]; P1, P3, CN1 also [C].
    at(10.6, 20.6, 50.4, 54.7, h_jst, "ivory");   // P3  "extended IO"
    at(49.8, 60.0, 50.2, 54.4, h_jst, "ivory");   // CN1 "temperature and humidity"
    at(88.9, 93.6, 8.9, 19.1, h_jst, "ivory");    // P1  serial / 5 V
    at(38.0, 42.1, 0.7, 4.1, h_jst, "ivory");     // speaker
    at(80.5, 85.4, 0.1, 3.9, h_jst, "orange");    // battery (P5)
    // Display and touch flat-cable connectors, and the largest IC. [P]
    at(47.0, 52.4, 14.5, 40.5, h_fpc, "ivory");
    at(62.6, 67.9, 42.6, 49.9, h_fpc, "ivory");
    at(74.5, 78.8, 22.7, 33.1, h_ic, "black");
}

module board_reference() {
    board_pcb();
    board_front();
    board_back();
}

// Shown when this file is opened by itself: back side up.
board_reference();
