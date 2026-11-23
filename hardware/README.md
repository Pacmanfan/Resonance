# Resonance enclosure (CAD)

A 3D-printable case for the Resonance device: the **ESP32-2432S032C** board
(Sunton 3.2-inch display board, capacitive touch, USB-C) with a flat LiPo
cell behind it. The case is written in OpenSCAD and is parametric. It gives
access to USB-C, the microSD slot and the board's power switch (through a
separate printed push-button) and has two small windows in the front face
for the RGB LED and the light sensor.

Everything in this file is taken from the OpenSCAD sources, from the design
log ([`revisions/HISTORY.md`](revisions/HISTORY.md)) and from the checks
described under [Validation](#validation). The design log holds the detail:
assembly, hardware, openings, what is measured and what is not.

| Directory | Contents |
|---|---|
| [`openscad/`](openscad/) | The sources. They include each other by relative path, so they stay together |
| [`stl/`](stl/) | Binary STL exports, one per printable part per revision |
| [`previews/`](previews/) | PNG renders, the same six views for every revision |
| [`previews/original/`](previews/original/) | The pictures made during development, as used by the design log |
| [`revisions/HISTORY.md`](revisions/HISTORY.md) | The design log (formerly `hardware/case/README.md`) |
| [`tools/check_stl.py`](tools/check_stl.py) | The STL check used for the validation table (Python, numpy) |

## The board and the cell: where the numbers come from

No 3D model of the bare board was found (searched 2026-11-18), so the case
is checked against a reference model, [`openscad/esp32_2432s032.scad`](openscad/esp32_2432s032.scad).
Neither it nor [`openscad/battery.scad`](openscad/battery.scad) is a printed
part. The reference model marks each number with its source:

| Mark | Source | Good for |
|---|---|---|
| D | The manufacturer's dimension drawing, mirrored at `github.com/kpeeem/3.2inch_ESP32-2432S032` (`3-Structure_Diagram/3.2.jpg`) | Printed numbers: outline 93.7 × 55.0 mm, holes 3.2 mm at 3.5 mm from the edges |
| P | The same drawing (front) and the vendor's rendering of the back shown on `espboards.dev/esp32/cyd-esp32-2432s032`, scaled from the mounting holes | Positions in the plane, about ±0.3 mm. A rendering, not a photograph |
| C | Two published cases, their files parsed: Printables models 1315032 and 832282 | Independent check of USB-C, microSD, power switch, RESET, BOOT and three connectors, within about 0.5 mm; the only source for heights, and then only as upper bounds |
| E | Estimate | All component heights, the board's thickness (1.6 mm) and corner radius (3.5 mm) |

Measured by the operator and recorded in the sources: the glass, 77.6 × 55.0
mm and 4.0 mm above the board (2026-11-18, used from version 3 on); the
positions of USB-C, the microSD slot and the button are marked "confirmed by
the printed v1". Not measured: where the glass sits along the board's length
(`glass_x`), the component heights, the board's corner radius.

The cell is "approximately 90 × 60 × 11 mm" (the operator's figure). Version
4 slim is for a second cell with the same length and width and a thickness
of 5.75 mm (the operator's figure); the length and width are still the
approximate ones.

## Revisions

The revision name is the suffix of the source file. `resonance_case.scad`
without a suffix is version 1. `v2`, `v3` and `v4` follow one another, each
a complete file made from the one before. `v4_slim` is not a later version
but a variant of `v4`: its source includes `resonance_case_v4.scad` and
changes one number, the thickness of the cell.

| Revision | Source | Outer size (mm) | Printed parts | What changed |
|---|---|---|---|---|
| v1 | `resonance_case.scad` | 106 × 67 × 30.3 | `front`, `tray`, `lid`, `button` | First version. The board is screwed to the front shell; a tray whose 2 mm floor divides board and cell is screwed on with three screws; a lid is screwed to the tray. Walls 2.0 mm, faces 1.6 mm |
| v2 | `resonance_case_v2.scad` | 105 × 66 × 25.1 | `front`, `back`, `button` | From the operator's notes on the printed v1: two shells that snap together (no tray, no lid, no case screws), no divider, window cut to the glass (76.6 × 54.4 mm, from the drawing) so that the glass is level with the face, walls and faces 1.5 mm, bosses with holes for M3 heat-set inserts, button stem 0.5 mm longer, rounded face edges |
| v3 | `resonance_case_v3.scad` | 105 × 66 × 25.1 | `front`, `back`, `button` | Window cut to the glass as measured, 77.6 × 55.0 mm; it runs on to the wall on the microSD side. USB-C opening is a hole with round ends, 13 × 7 mm, in the front shell alone. Corner radius 4.5 mm, edge radius 2.2 mm |
| v4 | `resonance_case_v4.scad` | 106 × 67 × 25.1 | `front`, `back`, `button` | Side walls 2.0 mm instead of 1.5 mm; faces stay 1.5 mm; the inside is unchanged |
| v4 slim | `resonance_case_v4_slim.scad` | 106 × 67 × 19.85 | `front`, `back`, `button` | v4 for the 5.75 mm cell (`bat_t = 5.75`). Front shell and button are the same parts as in v4; the back shell is 5.25 mm lower |

The outer sizes are the ones the sources print when rendered ("Outer size:
..."), and they agree with the design log.

### Status

| Revision | Renders as valid solids | STL exported | Printed and fitted |
|---|---|---|---|
| v1 | yes | yes, 4 parts | **Printed once, 2026-11-18.** The design log records that its USB-C, microSD and button openings lined up with the board. The operator's notes on that print (the "Asked for" column of the version 2 table in the design log; the v4 source quotes the button as "very slightly too short") led to v2 |
| v2 | yes | yes, 3 parts | Not recorded. The design log lists what is "not known until printed" for this version and nowhere says that it was printed |
| v3 | yes | yes, 3 parts | Not printed (design log) |
| v4 | yes | yes, 3 parts | **Not printed.** Rendered only |
| v4 slim | yes | yes, 3 parts | **Not printed.** Rendered only |

"Renders as valid solids" means the checks under [Validation](#validation),
run on 2026-11-23; it says nothing about fit. Only v1 has been checked
against a real board. The header comment of `resonance_case.scad` still says
that nothing has been printed; it was written before the print and the
source has been left unchanged.

### Pictures

All pictures are rendered by OpenSCAD from the sources, with the commands
under [Regenerating](#regenerating-the-stl-files-and-the-previews). The
assembled and exploded views include the reference models of the board and
the cell (in the assembled view of v2 to v4 slim the cell is hidden inside;
in v1 the board shows through the window). `inside` is the front shell
alone, lying on its face as printed. `section` is the cut along the length
through the mounting holes on the microSD side, screen at the bottom.

| | Assembled | Front | Back |
|---|---|---|---|
| v1 | ![v1 assembled](previews/v1_assembled.png) | ![v1 front](previews/v1_front.png) | ![v1 back](previews/v1_back.png) |
| v2 | ![v2 assembled](previews/v2_assembled.png) | ![v2 front](previews/v2_front.png) | ![v2 back](previews/v2_back.png) |
| v3 | ![v3 assembled](previews/v3_assembled.png) | ![v3 front](previews/v3_front.png) | ![v3 back](previews/v3_back.png) |
| v4 | ![v4 assembled](previews/v4_assembled.png) | ![v4 front](previews/v4_front.png) | ![v4 back](previews/v4_back.png) |
| v4 slim | ![v4 slim assembled](previews/v4_slim_assembled.png) | ![v4 slim front](previews/v4_slim_front.png) | ![v4 slim back](previews/v4_slim_back.png) |

| | Front shell from inside | Section | Exploded |
|---|---|---|---|
| v1 | ![v1 inside](previews/v1_inside.png) | ![v1 section](previews/v1_section.png) | ![v1 exploded](previews/v1_exploded.png) |
| v2 | ![v2 inside](previews/v2_inside.png) | ![v2 section](previews/v2_section.png) | ![v2 exploded](previews/v2_exploded.png) |
| v3 | ![v3 inside](previews/v3_inside.png) | ![v3 section](previews/v3_section.png) | ![v3 exploded](previews/v3_exploded.png) |
| v4 | ![v4 inside](previews/v4_inside.png) | ![v4 section](previews/v4_section.png) | ![v4 exploded](previews/v4_exploded.png) |
| v4 slim | ![v4 slim inside](previews/v4_slim_inside.png) | ![v4 slim section](previews/v4_slim_section.png) | ![v4 slim exploded](previews/v4_slim_exploded.png) |

## Opening and modifying the sources

The files were written with, and everything here was exported by,
**OpenSCAD 2025.02.11** (`OpenSCAD version 2025.02.11.ai23426`, the AppImage
build), with its default geometry backend (CGAL). Other versions have not
been tried.

Open a `resonance_case*.scad` file from `openscad/` in OpenSCAD. The variable
`part` near the top of each file selects what is drawn:

| `part` | What it shows |
|---|---|
| `"assembly"` (default) | Everything together, with the board and the cell |
| `"exploded"` | The stack pulled apart |
| `"section"` | A flat drawing of the cut along the length |
| `"front"`, `"back"`, `"button"` (v1: `"front"`, `"tray"`, `"lid"`, `"button"`) | One printable part, lying the way it is to be printed |

Open `esp32_2432s032.scad` or `battery.scad` by itself to look at the board
or the cell alone.

The main parameters (names as in `resonance_case_v4.scad`; values in mm):

| Parameter | v4 value | What it is |
|---|---|---|
| `bat_l`, `bat_w`, `bat_t` | 90, 60, 11 (in `battery.scad`; v4 slim sets `bat_t = 5.75`) | The cell. The case's height follows from `bat_t` |
| `bat_gap`, `bat_end` | 1.5, 6.0 | Room at each side of the cell and above it; room at each end |
| `comp_h` | 5.5 | Back of the board to the cell |
| `wall`, `front_t`, `back_t` | 2.0, 1.5, 1.5 | Side walls, front face, back face |
| `corner_r`, `edge_r` | 4.5, 2.2 | Corner radius; rounding of the edges of the two faces |
| `fit`, `pcb_gap` | 0.2, 0.5 | Clearance between printed parts; board to the walls |
| `glass_x`, `glass_l`, `glass_y`, `glass_w`, `glass_fit` | 8.3, 77.6, 0.0, 55.0, 0.4 | The window for the glass. `glass_x` is not measured |
| `lcd_h` | 4.0 | Front of the board to the top of the glass |
| `usb_y`, `usb_z`, `usb_open_w`, `usb_open_h` | 27.5, 1.65, 13.0, 7.0 | USB-C opening |
| `sd_x`, `sd_open_w`, `sd_open_h` | 55.1, 15.0, 3.2 | microSD slot |
| `btn_x`, `btn_z`, `btn_travel`, `btn_overhang` | 24.8, 1.6, 0.0, 0.6 | The push-button |
| `insert_d`, `insert_depth` | 4.0, 3.4 | Holes for the M3 heat-set inserts |
| `skirt_h`, `skirt_t`, `snap_grip` | 3.0, 1.0, 0.4 | The snap joint. `snap_grip` is the number to change, by 0.1 at a time, if the snap is too loose or too tight |
| `front_h` | 13.0 | Where the seam between the shells is |

The sources end with `assert` lines that stop the render if a change makes
an opening break through a wall or the window cut into the bosses. Version 1
has its own, different set of parameters (tray, lid, columns); see its
source and the design log.

## Regenerating the STL files and the previews

Run from this directory (`hardware/`). Set `OPENSCAD` to the program; the
files here were made with the AppImage, which has to be started with
`--appimage-extract-and-run` where FUSE is not available.

```bash
OPENSCAD="${OPENSCAD:-openscad}"
# for example: OPENSCAD="$HOME/Desktop/OpenSCAD.AppImage --appimage-extract-and-run"
export QT_QPA_PLATFORM=offscreen    # no display needed for the PNG export

src() { case "$1" in v1) echo openscad/resonance_case.scad ;; *) echo "openscad/resonance_case_$1.scad" ;; esac; }

# STL: one binary file per printable part, full render
stl() {   # stl <revision> <part>
    $OPENSCAD --render --export-format binstl \
        -o "stl/resonance_case_$1_$2.stl" -D "part=\"$2\"" "$(src "$1")"
}
for p in front tray lid button; do stl v1 "$p"; done
for r in v2 v3 v4 v4_slim; do for p in front back button; do stl "$r" "$p"; done; done

# PNG: the same six views for every revision
png() {   # png <revision> <view> <part> <camera> <projection> [--render]
    $OPENSCAD -o "previews/$1_$2.png" --imgsize=1200,900 --colorscheme=Tomorrow \
        --viewall --autocenter --camera="$4" --projection="$5" $6 \
        -D "part=\"$3\"" "$(src "$1")"
}
for r in v1 v2 v3 v4 v4_slim; do
    png "$r" assembled assembly 0,0,0,215,0,30,300 p
    png "$r" front     assembly 0,0,0,180,0,0,300  o
    png "$r" back      assembly 0,0,0,35,0,215,300 p
    png "$r" inside    front    0,0,0,35,0,25,300  p --render
    png "$r" section   section  0,0,0,0,0,0,300    o --render
    png "$r" exploded  exploded 0,0,0,250,0,35,300 p
done

# Check the STL files (numpy only)
python3 tools/check_stl.py stl/*.stl
```

A single file, written out:

```bash
openscad --render --export-format binstl -o stl/resonance_case_v4_slim_back.stl \
    -D 'part="back"' openscad/resonance_case_v4_slim.scad
openscad -o previews/v4_slim_assembled.png --imgsize=1200,900 --colorscheme=Tomorrow \
    --viewall --autocenter --camera=0,0,0,215,0,30,300 --projection=p \
    -D 'part="assembly"' openscad/resonance_case_v4_slim.scad
```

The camera is given as translation, rotation and distance; `--viewall
--autocenter` then fits each model to the picture, so the angle is the same
for every revision and the scale follows the model. `assembled`, `front`,
`back` and `exploded` are OpenSCAD previews, which keep the colours set in
the sources; `inside` and `section` are full renders.

A repeated export gives the same triangles, but OpenSCAD does not always
write them in the same order, so the STL files are not byte-for-byte
reproducible. A PNG can differ by a few pixels between runs.

## Validation

Run on 2026-11-23 with OpenSCAD 2025.02.11, for every part of every
revision:

1. Full render and export (`--render --export-format binstl`). Every one of
   the 16 exports exited with status 0, printed no `WARNING` and no `ERROR`,
   tripped none of the sources' `assert` checks, and OpenSCAD reported the
   top-level object as `Simple: yes`.
2. [`tools/check_stl.py`](tools/check_stl.py) on the exported file: number of
   triangles, bounding box, every edge shared by exactly two triangles with
   opposite directions (closed and consistently oriented surface), number of
   separate shells, enclosed volume. No degenerate triangles were found.
3. The bounding box compared with what the source declares.
4. Every preview picture was looked at: none is blank or cut off.

| STL file (`stl/resonance_case_…`) | Triangles | Bounding box X × Y × Z (mm) | Manifold | Shells | Volume (mm³) | Declared by the source |
|---|---|---|---|---|---|---|
| `v1_front.stl` | 4604 | 106.00 × 67.00 × 15.19 | yes | 1 | 16185.3 | 106 × 67; `front_h` 14.2 + rim 1.0 (`tongue_h`), less the 0.01 overlap `eps` |
| `v1_tray.stl` | 2008 | 106.00 × 67.00 × 14.50 | yes | 1 | 22311.9 | 106 × 67; `tray_h` 14.5 |
| `v1_lid.stl` | 1224 | 106.00 × 67.00 × 3.20 | yes | 1 | 11513.1 | 106 × 67; `lid_t` 1.6 + rim 1.6 |
| `v1_button.stl` | 572 | 8.50 × 8.50 × 7.70 | yes | 1 | 166.9 | lip 8.5 (`btn_lip_d`); 1.85 + 1.75 + 0.7 + 3.4 = 7.7 long |
| `v2_front.stl` | 4376 | 105.00 × 66.00 × 13.00 | yes | 1 | 9979.4 | 105 × 66; front shell 13 |
| `v2_back.stl` | 2064 | 105.00 × 66.00 × 15.10 | yes | 1 | 17399.5 | 105 × 66; back shell 12.1 + skirt 3.0 |
| `v2_button.stl` | 572 | 7.00 × 7.00 × 8.20 | yes | 1 | 148.6 | flange 7.0; shaft 3.3 + cone 1.0 + flange 0.8 + stem 3.1 = 8.2 |
| `v3_front.stl` | 4572 | 105.00 × 66.00 × 13.00 | yes | 1 | 9567.0 | 105 × 66; front shell 13 |
| `v3_back.stl` | 2056 | 105.00 × 66.00 × 15.10 | yes | 1 | 17019.5 | 105 × 66; back shell 12.1 + skirt 3.0 |
| `v3_button.stl` | 572 | 7.00 × 7.00 × 8.20 | yes | 1 | 148.6 | as v2 |
| `v4_front.stl` | 4568 | 106.00 × 67.00 × 13.00 | yes | 1 | 11693.1 | 106 × 67; front shell 13 |
| `v4_back.stl` | 2056 | 106.00 × 67.00 × 15.10 | yes | 1 | 19075.0 | 106 × 67; back shell 12.1 + skirt 3.0 |
| `v4_button.stl` | 572 | 7.00 × 7.00 × 8.70 | yes | 1 | 158.4 | flange 7.0; shaft 3.8 + cone 1.0 + flange 0.8 + stem 3.1 = 8.7 |
| `v4_slim_front.stl` | 4568 | 106.00 × 67.00 × 13.00 | yes | 1 | 11693.1 | 106 × 67; front shell 13 (same part as v4) |
| `v4_slim_back.stl` | 2056 | 106.00 × 67.00 × 9.85 | yes | 1 | 15413.4 | 106 × 67; back shell 6.85 + skirt 3.0 |
| `v4_slim_button.stl` | 572 | 7.00 × 7.00 × 8.70 | yes | 1 | 158.4 | as v4 |

In every row the bounding box agrees with the source. The shell heights add
up to the declared outer height: v1 14.2 + 14.5 + 1.6 = 30.3; v2 to v4
13 + 12.1 = 25.1; v4 slim 13 + 6.85 = 19.85. The skirt and the rims lie
inside the neighbouring part.

"Manifold: yes" means that every edge of the mesh is shared by exactly two
triangles and the triangles are consistently oriented. It is a check of the
file, not of printability, fit or strength. No slicer was run.

## Print notes

From the design log; the first paragraph is for v2 and the versions made
from it, and of these nothing has been confirmed by a print (see
[Status](#status)).

- v2, v3, v4, v4 slim: all parts print without supports, the front on its
  face, the back on its back face, the button on the end that shows.
  Hardware: four M3 heat-set inserts (3 mm long, about 4.2 mm outside) and
  four M3 × 4 pan-head screws for the board.
- v1: all parts print flat on the bed without supports, the front shell on
  its face, the tray on its floor, the lid on its outer face, the button
  standing on its shaft. Hardware: four M3 × 6 pan-head screws (board), three
  M3 × 10 countersunk screws (tray), two M3 × 6 screws (lid).
- Each STL file has the part in that printing position, with its lowest
  point at Z = 0.
- Suggested first print (design log, written for v1): print only `front`, in
  draft quality, and lay the board in it to see whether the window and the
  openings are where the board has them.
- Check the polarity of the battery lead with a meter before plugging it in.

No material, layer height or printer settings are recorded.

## Licence

Hardware design files are part of the Resonance project and are licensed under GPL-3.0-or-later; see ../LICENSE
