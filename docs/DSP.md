# Resonance signal path

What the audio engine computes, in the order it computes it. Source:
`firmware/lib/resonance_core/src/` (`rc_tone`, `rc_audio`, `rc_noise`,
`rc_lab`, `rc_resample`, `rc_wav`, `rc_ring`). Everything runs at 44.1 kHz,
stereo, 16-bit, because that is what Bluetooth A2DP carries.

No statement is made here, or anywhere in the project, about what any
frequency does to a listener.

```
 left osc  ─┐                                         ┌─ recording (ring buffer) × level × fade
 + partials ├─ × tone/partial gains ─ × breath ─┐     │
 right osc ─┘                                   ├─ Lab ─ + ─ × master × session gain ─ limiter ─ int16 ─► Bluetooth
 noise ────── × noise gain ────────── × breath ─┘
```

## Binaural tone

Two independent sine oscillators (`rc::Oscillator`):

- left ear: the **primary** (carrier) frequency f, 20–1000 Hz;
- right ear: f + b, where b is the **beat** frequency, 0.5–40 Hz.

Each ear receives a pure tone; the beat exists only as the difference
between the two. Both values move in 0.1 Hz steps.

Each oscillator is a 32-bit phase accumulator read through a 1024-entry sine
table with linear interpolation. A change of frequency changes only the
increment, spread over 50 ms, never the phase, so the waveform has no step.
Level changes ramp over 50 ms; switching the audio on or off fades over
0.5 s. The tone's ceiling is 0.5 of full scale, leaving headroom for the
other layers.

Optional, all off by default and then costing nothing:

- **Phase offset** per ear (degrees), reached by a glide of 50 ms or, as a
  **phase sweep**, over a given time of up to an hour.
- **Inversion** per ear, passing through silence in 50 ms.
- **Golden partials:** two more sines per ear at φ ≈ 1.618 and φ² ≈ 2.618
  times the primary frequency, each up to 0.5 of the fundamental's
  amplitude. The right ear's partial is the left's plus the beat frequency
  (φ·f + b, φ²·f + b), so every pair differs by the same b as the
  fundamental.

## Noise

One generator, the same signal in both ears (`rc::NoiseGenerator`):

- **White:** a xorshift32 sequence scaled to ±0.5.
- **Pink:** Paul Kellet's economy filter — three one-pole sections over the
  white noise, approximating −3 dB per octave.
- **Brown:** a leaky integrator of white noise, −6 dB per octave; the leak
  keeps it from drifting.

Coloured noise is clamped to the range. The noise layer's ceiling is 0.3 of
full scale. The sequence is deterministic: it is a sound source, not a
random-number source for anything else.

## Recordings and resampling

The file player task reads WAV (integer PCM, 8/16/24/32-bit, mono or stereo) or MP3
(Helix fixed-point decoder) from the card, converts to stereo 16-bit, and —
if the file is not at 44.1 kHz — resamples by **linear interpolation**,
block by block (`rc::Resampler`). Decoded frames go into a lock-free ring of
4096 frames (93 ms). The render reads from the ring; if it is empty the
recording contributes silence and an underrun is counted.

A recording fades in when it starts and out when it stops or gives way to
the next one; with one decoder, "crossfade" is fade out, then fade in. The
recording layer's ceiling is 0.6 of full scale.

## Layers, breathing and the Lab

Four layers — tone, partials, noise, recording — each have a switch, a
level and a fade time; gains are ramped per sample. Ramps longer than half a
second are computed from the samples remaining rather than accumulated, so a
long fade ends exactly on its target.

The **breathing pacer** can modulate tone and noise (not the recording)
with a gain between 1 and, at most depth, 0.2; with the pacer off the gain
is exactly 1.

The **Resonance Lab** acts on the generated sound only (tone, partials,
noise), before the recording is added:

- **width**: 100 % leaves the ears as generated, 0 % gives both the same mix;
- **interaural delay**: up to 2000 µs to either ear, by a short
  interpolated delay line, changed by a glide;
- **bilateral alternation**: the level moves between the ears with a period
  of 0.5–60 s, the time on each side equal or in the golden ratio, with
  smooth crossings;
- a **trim** used when comparing two settings A and B: the louder of the
  two, by an A-weighted estimate of tone and partials, is turned down.

With every Lab control at rest the stage is bypassed.

## Mix, master and limiter

Per sample: (tone × breath + recording + noise × breath) × master volume ×
session gain. The session gain is the fade at the end of a session.

At their ceilings the layers sum to 0.5 + 0.6 + 0.3 = 1.4 of full scale, so
the sum passes through a **soft limiter**: unchanged up to 0.7, and above
that bent by a hyperbolic tangent towards a ceiling of 0.98,

    y = 0.7 + 0.28 · tanh((|x| − 0.7) / 0.28)      for |x| > 0.7

with the sign restored. It is memoryless: no attack, release or look-ahead.
The result is rounded to 16 bits.

## Output

Bluetooth A2DP with the SBC codec, to headphones or a speaker. The stack
asks for blocks; rendering one second of audio takes about 0.18 s of one
core for the plain tone and 0.38 s with every layer and the Lab at once
(measured with `bench audio`).

## Limitations

- **SBC is lossy** and the receiving device adds its own processing. The
  difference between the ears is generated exactly, but what arrives depends
  on the headset; true-wireless earbuds in particular may not keep the two
  channels sample-aligned. This has not been measured.
- The wired output (I2S DAC) is not built; the board's amplifier is unused.
- Linear-interpolation resampling is adequate for ambient material and
  speech, not for critical listening; use 44.1 kHz files to avoid it.
- The limiter is a waveshaper: when several layers are loud at once it adds
  harmonics rather than pumping. Keep the layers below their ceilings.
- 16-bit output without dither.
- One decoder: two recordings cannot overlap.
- Sine waves only; no other waveform.
- The noise is not band-limited or loudness-matched between the three types.
- Features added in 0.9.x (phase, partials, layers, breathing, Lab) are
  proven by host tests but **have not yet been judged by ear**.
