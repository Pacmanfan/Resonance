# Display refresh audit, 2026-11-18

**Request:** REQ-CHAT-20261117-001 (operator: "I like the main screen, and I would like to see it animated. Right now, the refresh rate seems very low. Audit and report about that.")
**Work:** WORK-000027
**Requirement:** RES-GUI-004 (lightweight visualisation), RES-GUI-005 (responsiveness)
**Method:** the firmware's `bench` serial command (firmware 0.7.1), run on the device in Play mode with a headset connected and the tone streaming. Times are from the device's microsecond clock. Whether the picture is correct at the higher clocks was **not** judged: that needs eyes on the panel.

## Findings

### 1. Nothing on the screen moves by design

The interface draws only when something changes: a touch, or the status
snapshot once a second, and then only the lines that differ. There is no
animation loop. The rings on the home screen are drawn once. What reads as a
low refresh rate is therefore two things: a still picture, and the visible
time a redraw takes when one does happen.

### 2. A full redraw of the home screen takes 187 ms

| Operation at the working clock (24 MHz) | Time |
|---|---|
| Whole home screen (`redraw`) | 187 ms |
| Fill of the full screen | 62 ms |
| Fill of a 120 × 120 region | 11.6 ms |
| Three thick rings (filled arcs) | 6.2 ms |
| Six one-pixel circles | 4.4 ms |
| Five lines of 18-pixel text | 26 ms (5.2 ms a line) |
| One read of the touch controller | 0.8 ms |

A tab change or any change that redraws the content is a wipe of about a
fifth of a second, drawn straight to the panel, so the eye sees it being
built. About a third of it is clearing the area; most of the rest is text
and the outlined buttons.

### 3. The display clock is conservative

| Clock | Full-screen fill | 120 × 120 fill | Five text lines |
|---|---|---|---|
| 24 MHz (working) | 62 ms | 11.6 ms | 26 ms |
| 40 MHz | 31 ms | 5.8 ms | 16 ms |
| 80 MHz | 16 ms | 2.9 ms | 11 ms |

Pixel transfer scales with the clock. 24 MHz was taken from a
board-definition file at the audit stage and never raised
(`docs/architecture/HARDWARE.md` M7). The panel accepted commands at 40 and
80 MHz in this test; **whether the picture was clean at those clocks is not
known.** Text does not scale as well, because building the glyphs is
processor time, not transfer time.

### 4. A buffered region is fast enough to animate

Drawing the three rings into an off-screen buffer of 120 × 120 pixels at
8 bits per pixel and sending it in one transfer:

| | Time at 24 MHz |
|---|---|
| Draw into the buffer | 1.4 ms |
| Send to the panel | 11.7 ms |
| **One frame** | **13 ms** |

That is flicker-free and allows 25 to 30 frames a second for the rings at
the present clock, using about a third of the loop's time; at 40 MHz about a
fifth.

### 5. The buffer's memory is the constraint

The buffer is 14.9 kB. In Play mode with an MP3 playing to a connected
headset, 38 kB of heap were free (largest block 30 kB) and **the buffer
could not be allocated** in one run of the test; with only the tone playing
(75 kB free) it could. A buffer must therefore be allocated once at start,
before the Bluetooth stack, and kept; it cannot be created when the home
screen is opened.

### 6. The loop has room

`loop()` ran 56 times a second: a fixed 15 ms delay per pass, 0.8 ms for the
touch read, the rest idle. A button press blocks the loop for 60 ms (the
pressed-state flash). Neither limits an animation of 20 to 30 frames a
second, but the fixed delay would have to become a frame timer.

### 7. Drawing did not disturb the audio

With the tone streaming, the longest gap between the Bluetooth stack's
requests for audio was 25 ms during the benchmark, the same as without it,
with no gap over 60 ms and no underrun. The display's SPI traffic and the
audio do not contend at this load.

One unexplained observation: in the first run, with an MP3 also playing, the
statistics line after the benchmark showed no audio requests at all in the
preceding seven seconds. The second run, without the MP3, showed a normal
stream throughout. The operator was using the audio button during both runs,
so the cause is not established.

## What this allows

| Step | Effect | Cost and risk |
|---|---|---|
| A. Raise the display clock to 40 MHz | Every redraw twice as fast (home screen about 100 ms) | One constant. Needs the operator to confirm the picture is clean; fall back to 24 MHz if not |
| B. Animate the rings from a buffer allocated at start | Smooth motion at 20-25 frames a second, no flicker | 15 kB of Play-mode heap, permanently. With an MP3 playing that leaves about 23 kB free where 38 kB are now: tight |
| C. Animate the rings without a buffer | No memory cost | Each ring is erased and redrawn in place (about 6 ms): some shimmer at the edges |
| D. Stop clearing the whole content area on a change | Tab changes and updates look instant where little changed | Rework of the screen-drawing code, screen by screen |
| E. Pause animation when the screen is dimmed or another tab is shown; a reduced-animation setting | Meets RES-GUI-004 | Small |

A reasonable order is A, then C (motion without the memory risk), then B
only if C does not look good enough and the heap with an MP3 playing is
measured again first.

## Not covered

- Whether 40 or 80 MHz gives a clean picture on this panel.
- Frame times of a real animation: only its building blocks were timed.
- The effect of animation on battery life.

## What was done afterwards

The operator chose options A and C with a pulse at the beat frequency
(WORK-000028, firmware 0.8.0): the display clock is 40 MHz, and the Home
rings are erased and redrawn in place when they move (`rc_pulse.h`,
`Ui::animateHome`). With the tone streaming to a headset and the rings
moving, the longest gap between audio requests over five seconds was 22 ms
with no underrun. The picture at 40 MHz and the look of the pulse are for
the operator to judge.
