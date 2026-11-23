# Resonance roadmap

Where the project is going. **Everything under "Planned" is planned: it is
not in the firmware, and no date is promised.** What exists is listed in the
[README](../README.md) and the [changelog](../CHANGELOG.md).

Order of priorities: a dependable binaural meditation device first, an audio
laboratory second, an experimental platform third. Nothing is added that
makes plain listening less reliable, and nothing is claimed about effects on
a listener.

## State of the audio features

The original roadmap listed the items below as plans. They were built in
firmware 0.9.0–0.9.6 and are **implemented but not yet verified by ear or
eye**; until someone has listened to them they are not "done".

| Item | State |
|---|---|
| Expanded binaural DSP: phase offset and inversion per ear, phase sweep | Implemented, host-tested, not heard |
| Golden harmonics (partials at φ and φ²) | Implemented, host-tested, not heard |
| Phase and stereo experiments: width, interaural delay, bilateral alternation | Implemented (Resonance Lab), host-tested, not heard |
| Progressive and multi-stage sessions, session builder | Implemented; storage on the card checked on the device; screens not seen by a person |
| Breathing pacer | Implemented; streamed cleanly at 60 % depth; not judged by a person |
| Guided audio and mixer: four layers, fades, a recording per session stage | Implemented; two recordings heard; fades not judged |
| Solfeggio sequences | Implemented as editable frequency sequences |
| Resonance Lab with A/B comparison | Implemented, not heard |

## Planned: finishing what exists

- Listening verification of everything in the table, and correction of what
  it turns up (clicks, levels, legibility, button sizes).
- Stream and memory figures with the Lab active under Bluetooth.
- Player transport controls (seek, previous / next).
- Bluetooth device management on screen: forget a device, choose among
  remembered ones.
- Noise type kept across restarts; card inserted while running recognised
  without a restart.
- Settling the MP3 decoder licence question so that firmware binaries can be
  published (see `THIRD_PARTY_LICENSES.md`).
- Printing and fitting enclosure v4 or v4 slim.

## Planned: audio

- Sequencing of several recordings within a session with timed gaps.
- User presets (a named set of frequencies and layer levels) on the card.
- Session history kept on the card.
- Wired stereo output through an I2S DAC, if Bluetooth's limits for binaural
  material prove to matter (on hold, ADR-000204).

## Planned: Resonance Studio and Lab

- Grouping the advanced screens as one "Studio" area.
- Repeating phase sweeps, and phase sweeps driven by a session stage.
- Refinement of loudness matching once it has been listened to.

## Planned: Mindfield

A research concept, described in [MINDFIELD.md](MINDFIELD.md). In order:

1. **Random-number sources** behind one interface: the ESP32 hardware
   generator, with its entropy conditions checked and reported; a
   deterministic software generator as a control; investigation of ADC
   noise and timing jitter; an extension point for an external generator.
2. **Collection:** raw samples on a fixed schedule, recorded to the card
   with full metadata, never discarded.
3. **Statistics:** bit balance, distribution, runs, autocorrelation,
   deviation from a target chosen beforehand, with honest handling of
   repeated comparisons.
4. **Experiment modes:** directed intention, open exploration, baseline and
   control conditions.
5. **Visualizations** driven by the measured statistics and replayable from
   recorded data: Orb, Balance, Tree, Mandala, then Flow (River, Labyrinth,
   Waterfall), Intensity (Star, Flame), Lotus, Vortex, Pendulum.
6. Results, history and export on the device.

**Items 1 to 3 and the condition and mode settings of item 4 exist since
firmware 0.11.0, without screens; items 5 and 6 do not. The project does not claim that meditation
or intention influences a random-number generator; the aim is an instrument
that would show honestly whether anything is there.**

## Not planned

- Wi-Fi, a web interface or cloud services (removed, ADR-000205).
- Battery level display on this board (no measurement hardware).
- Text entry on the device, session journaling.
- A GUI framework.
