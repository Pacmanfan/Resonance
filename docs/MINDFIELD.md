# Mindfield — a research concept

**Status (firmware 0.12.0): implemented — sources, collection, raw files,
statistics, the screens, twelve pictures, history, replay and export. Built,
host-tested and run on the board through injected touches. No person has
yet looked at a Mindfield screen or picture, and no experiment worth the
name has been run: there are no long baselines yet.** See
[What is implemented](#what-is-implemented) at the end, including what is
still missing.

## The question

Experiments since the 1970s have asked whether the output of physical
random-number generators deviates from chance when people direct attention
or intention at them. The reported effects are very small, their
interpretation is disputed, and replication has been inconsistent. A
meditation device that already holds a hardware random-number generator, a
screen, storage and a session timer can at least collect clean data on the
question. Mindfield is that: a measuring instrument inside Resonance, not a
game with a hidden score.

## Random-number sources

All sources are to sit behind one interface that reports the source's name,
whether it is available, and what is known about its entropy.

### ESP32 hardware generator

`esp_random()` and `esp_fill_random()` read the chip's hardware
random-number generator. Espressif documents that its output is true random
only while an entropy source is feeding it: the radio (Wi-Fi or Bluetooth)
being enabled, or the SAR ADC entropy source that the bootloader switches on
during start-up. Without one, the same functions return pseudo-random
numbers. Resonance runs Bluetooth whenever it is in use, which should
satisfy the condition — **this has to be verified for the exact framework
version and radio state, and reported on screen, before any data is called
physical.** If the condition is not met the source must say so and must not
be silently replaced by software.

Also to be established: how fast the register may be read before successive
values stop being independent (Espressif gives guidance on read rate), and
whether the Bluetooth stack's own use of the generator matters.

### Other candidates — to be investigated, not assumed

- **Internal ADC noise.** The least significant bits of an ADC reading
  carry noise, but ordinary ADC variation is not automatically entropy: it
  can be dominated by supply ripple, the backlight or radio bursts. It would
  need characterising and probably conditioning, and must not disturb the
  audio. A floating pin is not a noise source.
- **Timing jitter** between independent oscillators. On this board it is
  hard to separate physical jitter from scheduling; touchscreen timing is
  not a trusted source. Experimental at best.
- **External hardware generator** on a serial connection. None is fitted;
  the interface should allow one, both as a better source and as an
  independent check on the internal one.

### Software generator — the control

A deterministic pseudo-random generator with a recorded seed. It is not
physical and must always be labelled as software. It exists to test the
statistics and the visualizations, to make runs reproducible, and to show
what the display looks like when nothing but arithmetic is driving it.

## Collecting raw samples

- A **fixed sampling schedule** (for example 100 to 1000 samples a second),
  independent of the animation and of anything the participant does.
- **Fixed measurement windows.**
- **Every raw sample is recorded** to the card with a sample counter, the
  source, the configuration, the firmware version, the experimental
  condition and — in directed mode — the target. Summaries never replace
  raw data.
- Buffered writes outside the audio path; an interrupted experiment leaves
  a file that says it was interrupted.
- A documented binary format with a header, and CSV export of summaries.

## Conditions and modes

**Baseline and control come first.** An experiment is only interpretable
against data collected the same way when nobody is trying anything:

- resting baseline;
- meditation condition;
- control condition (device running, participant occupied otherwise);
- optionally a sham-meditation condition;
- with the order randomised or counterbalanced, and the condition recorded.

Two modes:

- **Directed intention.** Before collection starts the participant chooses
  a target — more ones than zeros, one side of a balance, one of several
  outcomes. The target is recorded and cannot be changed during the run. The
  analysis asks one question, fixed in advance: did the output deviate in
  that direction?
- **Open exploration.** No target. The device records output and
  deviations for later study. Results from this mode are exploratory by
  definition and cannot confirm anything.

## Statistics

Computed by one engine that knows nothing about the display:

- **Bit balance:** counts of ones and zeros, the proportion, its deviation
  from one half, a standardised score (z) and a confidence interval; an
  exact binomial test for the directed question.
- **Distribution:** byte values and chosen bit patterns against the uniform
  expectation (chi-squared).
- **Runs:** number and lengths of runs of identical bits against
  expectation.
- **Autocorrelation** at chosen lags — also a check on the source itself.
- **Windows:** the same measures per fixed window and over the session, and
  the comparison of meditation with baseline.

Rules the analysis keeps:

1. The hypothesis and the target are fixed before the data exist.
2. No sample, window or session is dropped because it is unfavourable.
3. Every session is kept and listed, including the ones that show nothing.
4. Looking at many windows, many measures or many sessions means many
   comparisons; significance is corrected for that, and windows are not
   assumed independent.
5. A p-value below a conventional threshold is not proof of a cause. With
   millions of bits, a tiny bias in the source itself becomes
   "significant"; that is why baseline data from the same source matter
   more than any theoretical expectation.
6. Exploratory findings are labelled exploratory until a pre-specified
   experiment repeats them.

## Visualization concepts

A visualization is a picture of a **defined statistical measure**, never an
input to it. It may smooth motion; it may not change a number. Each must be
replayable from recorded data, and each has a clear neutral state so that
chance looks like chance.

| Concept | Idea |
|---|---|
| **Orb** | A glowing sphere at the centre of a circular field. Its distance from the centre is a standardised score of deviation in the chosen direction — not a random walk, which would drift outward by itself |
| **Balance** | A beam that tilts with the measured excess of one binary outcome over the other |
| **Tree** | Branching decided by recorded samples: a cumulative picture of a run |
| **Mandala** | Radial, symmetric construction from the same decisions |
| **Flow** | Particles choosing between channels (river, labyrinth, waterfall); the resulting distribution is the observed one |
| **Intensity** | A star or a flame whose size follows the measured deviation |

A picture that grows or moves can feel like success when it shows only
chance. For that reason the active screen is to show no "score", and the
results screen is to state what was expected, what was observed and how
uncertain that is, in plain numbers.

## What has to be true before this is useful

- The entropy conditions of the hardware generator are verified and shown.
- Baseline runs from the device, long ones, exist and are published with
  the raw data.
- The statistics are validated against synthetic data with known answers.
- The software control source produces, through the same pipeline, exactly
  the results chance predicts.
- Audio stays reliable while samples are collected and written.

## What this is not

It is not evidence. It is not a training game. It is not a claim that the
device, a frequency, or meditation does anything to a random-number
generator.

## What is implemented

Firmware 0.12.0, requirements RES-MIND-001..006. Code: `rc_rng`,
`rc_mindstats`, `rc_mindfile`, `rc_mindviz` in the portable core (44 host
test cases with synthetic data of known answers), `firmware/src/mind/` and
`firmware/src/gui/ui_mind.cpp` on the device.

### Sources, as measured on the board (2026-11-23)

| Source | Class | State on this board | Measured |
|---|---|---|---|
| ESP32 hardware RNG | Hardware entropy | Available while the Bluetooth controller is enabled, which is the condition Espressif's header for the framework in use (ESP-IDF 5.5) gives for "true random numbers"; unavailable otherwise, never replaced | 2000 samples: proportion 0.49986, z −0.07, runs z −1.27, bytes z −0.23, lag-1 z 1.29. A 15 s run at 1000 samples/s: no gaps |
| Internal ADC (IO34, light sensor) | Experimental noise | **Not available on this board: the reading is fixed at 0**, so there is no noise to collect. The source reports this and is not offered | — |
| ADC noise input (IO35) | Experimental noise | Not available: the pin goes only to an empty connector. A floating pin is not a noise source; a noise circuit would have to be fitted | — |
| Timing jitter (RC oscillator against crystal) | Experimental timing | Available for study, **marked not suitable for experiments**: its bits are strongly correlated | 2000 samples: runs z 19.4, bytes z 10.4, lag-1 correlation −0.077 (z −19.4); 0.6 ms a sample |
| External RNG | External hardware | Not available: none fitted. The class is a placeholder | — |
| Software PRNG | Software control | Available, always labelled a control. xoshiro128** from a recorded seed | 20 s at 1000 samples/s, seed 777: z −0.87; replay from the card gives the same numbers |

A first longer baseline (experiment 16, 2026-11-23, hardware generator,
1000 samples a second for ten minutes, a session started, no headset):
600 000 of 600 000 samples, no gap; proportion of ones 0.4999533 (95 %
interval 0.49973 to 0.50018), z −0.41; runs z 0.43; bytes z −0.86; lag
scores −0.44, −2.09, −1.65, −1.17; 600 windows with 35 beyond 1.96 where 30
are expected, the largest 3.75 against a corrected threshold of 3.93; window
variance 1.14 where 1 is expected. Nothing here is outside chance, but the
window variance and the lag-2 score are the figures to watch in longer
baselines. One run is not a characterisation of the source.

Not established: the hardware generator's behaviour over long runs, its
independence at 1000 samples a second beyond these short tests, and any
effect of the Bluetooth stack's own use of it. Long baseline runs are the
next thing this needs.

### Collection

A sampling task above the interface and below the audio player takes one
sample per slot, counted in 1 ms system ticks; if it is held up the slots are
made up, so the number of samples follows the clock. The main loop feeds the
statistics and writes blocks of 128 samples; data reaches the card within a
second and the card's directory within five. Memory (about 10 kB) is taken
when an experiment starts and returned when it ends. Measured: 20 000 of
20 000 samples at 1000/s, no gaps; a run interrupted by a reset was read back
to its last complete block (7424 samples) and summarised as interrupted.

Not measured yet: collection while an MP3 streams over Bluetooth, and stream
statistics while collecting — no headset was connected.

### Files

`/RESONANCE/MINDFIELD/EXPERIMENTS/Ennnnn.RMF` is the raw file; its format is
documented in `firmware/lib/resonance_core/src/rc_mindfile.h`.
`RESULTS/Ennnnn.TXT` is the summary in text, and `EXPORTS/SUMMARY.CSV` the
table of all summaries. The device has no clock, so a file records the time
since the device started, not the date.

### Screens and pictures (0.12.0)

SET → MINDFIELD opens the home screen; from there the source, the set-up
(mode, target, condition, rate, length, picture), START, the last result and
the history. Tapping the title goes back; leaving the active screen does not
stop the experiment.

- **Score pictures** — Orb, Balance, Star, Flame, Pendulum, Vortex — show
  the standard score of the bit balance: towards the target when directed,
  towards "more ones" when open. Zero is a marked neutral state; the limit
  is a score of 3; motion is smoothed over about 0.6 s without changing the
  number. A host test runs 400 000 samples of the control source four times
  and checks that the picture stays near the centre and does not creep out
  with time.
- **Pattern pictures** — Tree, Mandala, Lotus, River, Waterfall, Labyrinth —
  take one recorded sample a window and grow by it. The screen says "Pattern
  from samples - not a score". River and Waterfall show eight equally likely
  channels with a line at the equal share.
- **The active screen shows no score in figures**: time, samples, and
  REC / NOT REC / PAUSED / REPLAY.
- **The result screen** gives the proportion of ones beside the expected
  0.5, its 95 % interval, z and p, the score towards the target, runs, bytes
  and lag-1 scores, the window figures with the threshold corrected for
  their number, and the comparison with the newest baseline of the same
  source ("No baseline from this source yet" otherwise).
- **Replay** plays the raw file again through the picture, a window a step,
  and ends with the statistics recomputed from the file.

Measured on the board (2026-11-23): a one-minute directed experiment at
1000 samples a second with the Orb, paused and resumed: 60 000 samples, no
gaps, result shown by itself; each of the other eleven pictures run for a
few seconds without a lost sample; replay, history paging, opening an older
experiment and export worked. Free heap 94 kB with the screens idle, 80 kB
at the lowest while collecting and drawing.

### Not built, or not verified

- **Not seen by a person:** every screen and picture. Layout was checked by
  listing buttons and label widths, not by eye.
- **Not measured:** collection and drawing while an MP3 streams over
  Bluetooth (no headset was connected), and runs longer than a minute.
- Targets other than more ones / more zeros (a favoured category).
- Randomised or counterbalanced ordering of conditions: the condition is
  chosen by hand and recorded.
- Choosing a session from the Mindfield set-up: start the session on SESS
  first; the experiment records which was running.
- Deleting an experiment on the device: files are removed with the card in
  a computer.
- Export of raw samples as text: the raw file is binary (documented) and the
  table export holds the summaries.
- Opening an old experiment recomputes it from the raw file, which takes
  about four seconds per megabyte and holds the screen meanwhile.
- A pattern picture starts again from empty if its screen is left and
  reopened during a run; a replay of the whole file shows it complete.
- The Mandala's number of folds is fixed at eight.

### Serial commands

`mind`, `mind sources`, `mind start <source> <rate> <seconds>
[open|ones|zeros] [baseline|meditation|control|sham] [seed]`, `mind pause`,
`mind resume`, `mind stop`, `mind list`, `mind recover`, `mind export`,
`mind replay <id>`, `mind bench <source>`.

### External generator: expected interface

Not built. A future external generator is expected on a serial line
(UART, 3.3 V; which pins is not decided), sending raw bytes with no framing
at a fixed speed; the source class would read four bytes a sample, report
itself unavailable when no byte arrives within one sampling period, and
record that as a gap.
