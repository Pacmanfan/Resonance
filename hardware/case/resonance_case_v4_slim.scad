// Resonance enclosure, version 4 "slim": version 4 for the thinner cell.
//
// The operator's second cell (2026-11-21) has the same length and width as
// the first, 90 x 60 mm, and is 5.75 mm thick instead of 11 mm. This file
// is resonance_case_v4.scad with that one number changed; everything else
// follows from it. The front shell and the button are the same parts as in
// version 4; the back shell is 5.25 mm lower.
//
// Choose what to render as in version 4:
//   openscad -D 'part="back"' -o back.stl resonance_case_v4_slim.scad
// with "assembly", "exploded", "section", "front", "back" or "button".
//
// An assignment after the include replaces the included value everywhere,
// in the derived dimensions of version 4 too.
include <resonance_case_v4.scad>

bat_t = 5.75;          // operator: "the new thickness is 5.75mm"
