// Reference model of the LiPo cell, for checking the fit in the case.
// Not a printed part. Size as given by the operator: "approximately"
// 90 x 60 x 11 mm; measure the real cell, including its protection board
// and the swelling allowance below, before printing.
//
// Origin: the cell's corner at the lead end; X along its length, Y across,
// Z through its thickness.
bat_l = 90;            // overall length, including the protection board strip
bat_w = 60;
bat_t = 11;
bat_pcm_l = 5;         // strip at the lead end holding the protection board
bat_lead_len = 5;      // leads, drawn straight and cut short
bat_lead_y = 12;       // where the leads leave the cell, from its corner
bat_swell = 0.5;       // a pouch cell may grow about this much in thickness with age

module battery(swollen = false) {
    t = bat_t + (swollen ? bat_swell : 0);
    r = min(2.5, t / 2 - 0.1);
    // Pouch: soft rounded long edges.
    color("silver") translate([bat_pcm_l, 0, 0]) hull() {
        for (y = [r, bat_w - r], z = [r, t - r])
            translate([0, y, z]) rotate([0, 90, 0]) cylinder(r = r, h = bat_l - bat_pcm_l, $fn = 24);
    }
    // Protection board under tape: thinner than the pouch.
    color("gold") translate([0, 2, t / 2 - 2]) cube([bat_pcm_l, bat_w - 4, 4]);
    // Leads.
    color("red") translate([-bat_lead_len, bat_lead_y, t / 2]) rotate([0, 90, 0]) cylinder(d = 1.3, h = bat_lead_len, $fn = 12);
    color("black") translate([-bat_lead_len, bat_lead_y + 2, t / 2]) rotate([0, 90, 0]) cylinder(d = 1.3, h = bat_lead_len, $fn = 12);
}

// Shown when this file is opened by itself; the case switches it off.
battery_demo = true;
if (battery_demo) battery();
