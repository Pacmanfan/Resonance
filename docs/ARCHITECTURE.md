# Resonance firmware architecture

An overview for someone opening the code for the first time. The detailed
design record, with the reasons behind each choice, is
[architecture/ARCHITECTURE.md](architecture/ARCHITECTURE.md); the decisions
are in [decisions/](decisions/); the signal path is in [DSP.md](DSP.md).

## Two halves

```
firmware/lib/resonance_core/src/     portable core (rc_*)
firmware/src/                        the device
```

**The portable core** is plain C++17 with no Arduino, ESP-IDF or display
header. It holds everything that is arithmetic or rules, so that it can be
compiled and tested on a PC:

| Module | What it is |
|---|---|
| `rc_tone` | Binaural tone engine: two table-lookup sine oscillators, glides, phase offset, inversion, golden partials |
| `rc_audio` | Audio engine: mixes tone, partials, noise and the recording from the ring buffer; layer gains, master volume, limiter |
| `rc_noise` | White, pink and brown noise |
| `rc_lab` | Resonance Lab: stereo width, interaural delay, bilateral alternation, loudness estimate |
| `rc_ring` | Lock-free ring buffer of stereo frames between the decoder and the render |
| `rc_resample`, `rc_wav` | Linear resampler to 44.1 kHz; WAV header parser and PCM conversion |
| `rc_session`, `rc_session_text` | Sessions as lists of stages; the runner that turns elapsed time into values; their text form |
| `rc_sequence`, `rc_edit` | Frequency sequences; the editing operations of the session builder |
| `rc_breath` | Breathing pacer |
| `rc_presets` | Frequency presets |
| `rc_settings`, `rc_state` | The settings record and its validation; the advanced settings kept on the card |
| `rc_paths` | Rules for file names and paths on the card |
| `rc_rng`, `rc_mindstats`, `rc_mindfile`, `rc_mindviz` | Mindfield: the random-source interface and software control source; the statistics engine; the raw-file format and summaries; what each picture shows |
| `rc_pulse`, `rc_repeat` | Home-screen ring animation timing; tap-and-hold repeat |

**The device half** connects the core to the hardware:

| Module | What it is |
|---|---|
| `main.cpp` | The application: owns the engine, the session runner and the settings; applies interface events; serial diagnostics |
| `audio/bt_audio` | Bluetooth A2DP source (ESP32-A2DP on Bluedroid): scan, connect, and the callback that pulls audio |
| `audio/player` | File player task: reads WAV or MP3 from the card, decodes, resamples, fills the ring |
| `gui/display`, `gui/ui`, `gui/ui_advanced`, `gui/ui_mind`, `gui/theme.h` | LovyanGFX display and touch set-up; the five tabs; the advanced screens; the Mindfield screens and pictures; colours and fonts |
| `storage/sdcard` | The only module that touches the card |
| `storage/settings_store` | Settings in the ESP32's non-volatile storage (NVS) |
| `storage/session_store` | Sessions, sequences and advanced settings as text files on the card |
| `hal/board`, `hal/board_pins.h` | Pins, reset reason, backlight |
| `mind/rng_sources`, `mind/mindfield` | Mindfield: the board's random-number sources; the sampling task, raw-file writing, history and export |
| `radio/bt_stack.cpp` | Keeps the memory reserved for Bluetooth |

There is one operating mode and no network function (ADR-000205).

## Who calls whom

```
 touchscreen ──► gui::Ui ──events──► main ──setters──► rc::AudioEngine ◄── render() ── Bluetooth task
                   ▲                  │                      ▲
                   └── Status ────────┘                      │ ring buffer (lock-free)
                                      └──commands──► player task ──► SD card
```

- **The interface never reaches into another module.** `gui::Ui` reads a
  `Status` snapshot, edits the settings records it is handed, and reports
  one `Event` per touch. `main` applies it.
- **The audio is pulled.** The Bluetooth stack calls back in its own task
  and asks for frames; `rc::AudioEngine::render()` produces them. It never
  blocks, allocates, logs or touches a file. Setters store single atomic
  values that the render picks up at the start of a block, so there is no
  lock on the audio path.
- **Files are decoded in their own task** (`player`), below the Bluetooth
  stack in priority and above the interface. It writes decoded frames into
  `rc::FrameRing`; the render reads them. An empty ring is silence and a
  counted underrun, never a wait.
- **Sessions are separate from oscillators.** `rc::SessionRunner` only
  turns time into target values; `main` applies them ten times a second.

## Tasks

| Task | Does | Must never |
|---|---|---|
| Bluetooth (stack's own) | Calls `render()`, encodes, sends | Wait for the card, the screen or a lock |
| `player` (priority 4, core 1) | Reads and decodes files, fills the ring | Starve the Bluetooth task |
| Arduino loop (priority 1) | Touch, screens, session runner, breathing pacer, settings writes, serial | Do anything the render waits for |

## Memory

No PSRAM. At rest with Bluetooth started about 98 kB of heap are free
(firmware 0.10.0); while an MP3 streams over Bluetooth the lowest figure
measured was 22 kB (firmware 0.9.2, with tone, partials and noise) against a
working limit of 16 kB. To stay there:

- the ESP-IDF libraries are rebuilt with Bluetooth Classic only;
- the decoder's ring buffer lives in the memory region that allows only
  32-bit access, which ordinary data — and the Bluetooth stack — cannot use;
- the card is mounted for two open files;
- the interface draws directly, with no frame buffer and no GUI framework;
- nothing on the audio path allocates.

## Storage

- **In the device (NVS):** the settings record — volumes, brightness,
  frequencies, session choices — validated on every load; names of
  Bluetooth devices. The stack keeps its own pairing keys.
- **On the card:** `/RESONANCE/AUDIO/` sound files, `/RESONANCE/SESSIONS/`
  user sessions, `/RESONANCE/SEQUENCE/` sequences, `/RESONANCE/ADVANCED.TXT`
  mixer, breathing and Lab settings. All text files are written under a
  temporary name and renamed, and are validated when read; a file that
  fails is ignored, not half-used.

The device works without a card: tone, noise and the factory sessions need
none.

## Interface

Five tabs — HOME, FREQ, SND, SESS, SET — and, behind ADVANCED on SET (and
BUILD on SESS), the layer mixer, breathing, session builder, sequence and
Resonance Lab screens. Tapping the header of an advanced screen goes back
one level. Widgets are a few hundred lines of our own on LovyanGFX.

## Host tests

`firmware/tests/test_*.cpp`, one program per core module, built by
`firmware/tools/host_tests.py` with the PC's compiler, `-Werror` and the
address and undefined-behaviour sanitizers: 19 programs. They check, among
other things, that frequency changes are phase-continuous, that every
optional stage is sample-for-sample neutral when off, and that no malformed
file or stored record is accepted.

What host tests cannot show — sound, picture, touch, Bluetooth, memory on
the board — is measured on the device and recorded with the work
(`evidence/`); where nobody has checked, the documents say so.
