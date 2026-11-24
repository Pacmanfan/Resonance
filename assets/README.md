# Project images

| File | What it is |
|---|---|
| `banner.png` | README banner |
| `social-preview.png` | GitHub social preview (1280 × 640) |
| `make_images.py` | The script that draws both |

These are drawings: rings as on the device's home screen and two sine waves
a cycle apart, standing for the two ears. They are not photographs, not
screenshots and not CAD renders. Renders of the enclosure, made from the
OpenSCAD sources, are in `hardware/previews/`. There are no photographs of
the device or screenshots in the repository yet.

## screenshots/

The device's screens at twice their size (480 x 640), read back from the
display with `firmware/tools/screenshot.py` (firmware 0.13.2). Not
photographs and not drawn by hand. To renew one, for example the FREQ tab:

```bash
python3 firmware/tools/screenshot.py assets/screenshots/frequency.png --scale 2 --tap 72 305
```
