# Mindfield — a research concept

**Status: concept. Nothing described here is implemented in the firmware.**

**No causal relationship between consciousness, meditation or intention and
the output of a random-number generator has been demonstrated by this
project, and none is claimed.** This document describes a method for
looking, built so that it cannot flatter the person looking. If the
instrument finds nothing, that is a result.

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
