# Resonance Product Requirements

**Status:** ACTIVE
**Owner:** Resonance firmware (`firmware/`, created in Stage 2) and its documentation under `docs/architecture/`
**Source request:** REQ-CHAT-20261117-001 (the complete project specification; section numbers below, "§n", refer to it)
**Depends On:** `development_operations.md` (lifecycle, evidence and completion rules)

Resonance is a standalone binaural meditation and ambient audio device built on
the Jingcai / Sunton ESP32-2432S032C-I board, with firmware in C++ on the
Arduino framework. This document owns the product's normative behaviour. The
request is the human input it was derived from; where the two differ, this
document is corrected explicitly, never silently.

Requirements are grouped by domain. A requirement marked *(optional)* may be
omitted only with a documented reason (RES-PROC-005).

## Process

### RES-PROC-001 — Audit before implementation

No firmware SHALL be written before the Stage 1 audit exists: `HARDWARE.md`,
`GPIO_MAP.md`, `ARCHITECTURE.md`, `LIBRARY_EVALUATION.md` and
`IMPLEMENTATION_PLAN.md` under `docs/architecture/`. The audit SHALL identify
the exact board, the display and touch controllers, the SD connections, a GPIO
ownership map, the I2S pin options, the battery-monitoring options, the onboard
amplifier routing, the library evaluation (including Bluetooth A2DP and MP3/WAV
decoding) and the memory budget (§18 Stage 1).

### RES-PROC-002 — No invented hardware facts

Pin assignments, controller types and electrical properties SHALL be taken
from a named source (schematic, manufacturer example, board-definition file or
a measurement on the board). A fact without such a source SHALL be recorded as
an assumption together with the measurement that would confirm it, and SHALL
NOT be presented as verified (§2, §2.1, §18).

### RES-PROC-003 — Staged delivery

The firmware SHALL be delivered in the stages of §18 (board foundation, stereo
audio prototype, playback and mixing, Bluetooth, session engine, web interface,
product refinement). Each stage SHALL be tested on its own before the next
begins.

### RES-PROC-004 — Recorded acceptance results

The acceptance tests of §19 SHALL be repeatable and their actual results
recorded in `evidence/`. A test that needs the physical device and was not run
on it SHALL be recorded as not run, never as passed. Compilation alone is not
completion.

### RES-PROC-005 — No silent omission

A specified feature that proves infeasible on the original ESP32 (4 MB flash,
no PSRAM) SHALL be reported with the limitation and a practical alternative;
it SHALL NOT be dropped silently (§20).

### RES-PROC-006 — No health claims

Firmware, web interface and documentation SHALL NOT claim that a frequency
guarantees a medical or neurological outcome (§4.2, §21).

## Hardware

### RES-HW-001 — Exact board

The firmware SHALL target the ESP32-2432S032C-I (3.2-inch 240 × 320 display,
capacitive touch) and SHALL NOT assume compatibility with the 2.8-inch
resistive CYD or other variants (§2).

### RES-HW-002 — Existing peripherals preserved

Display, touchscreen, SD card, Wi-Fi and Bluetooth SHALL keep working. No GPIO
change for audio output SHALL disable one of them (§2.1).

### RES-HW-003 — Wired stereo output *(on hold, ADR-000204)*

The wired output is on hold by the operator's decision of 2026-11-17: audio
is delivered over Bluetooth (RES-BT-001) and no DAC is connected. When it is
taken up again, wired output SHALL use an external PCM5102A I2S DAC followed by a stereo
headphone amplifier (not a speaker amplifier such as the PAM8403) and a 3.5 mm
stereo jack, with the wiring, GPIO assignment and bill of materials documented.
The onboard mono speaker amplifier SHALL NOT interfere with the stereo output
(§11).

### RES-HW-004 — Speaker connector

The speaker connector is a bridge-tied-load output. Documentation and wiring
SHALL NOT connect either terminal to ground or treat it as a GPIO (§2.1).

### RES-HW-005 — Power

The device SHALL run from USB and from a single-cell Li-ion/LiPo battery and
charge over USB. Documentation SHALL require a protected cell or external
protection. The board has no battery measurement circuit and none is added
(ADR-000204): the firmware SHALL NOT display a battery level (§2.2).

### RES-HW-006 — Low battery and power loss

The device SHALL recover safely after power loss: files are written
atomically and configuration is validated on load. Detecting a low battery
before it happens is not possible without a measurement circuit (RES-HW-005,
ADR-000204) (§2.2, §17).

### RES-HW-007 — Enclosure

The device SHALL have a printable enclosure that holds the board and the
battery, with a rigid divider between the cell and the board's components.
It SHALL give access to the USB-C port, the microSD card and the board's
power button (through a separate printed push-button with a lip seated in
the case wall), SHALL expose the RGB LED and the light sensor, and SHALL
leave the expansion connectors, the speaker connector and the reset and
boot buttons covered.
Its source SHALL be parametric, and every dimension that was not taken from
the manufacturer's drawing or measured on the board SHALL be marked as to be
measured (RES-PROC-002) (operator, 2026-11-17).

## Software architecture

### RES-ARCH-001 — Modular firmware

The firmware SHALL be modular C++ on the Arduino framework, with
hardware-specific code separated from application logic behind interfaces and
with one defined owner per shared resource (§3).

### RES-ARCH-002 — Layer independence

The audio engine SHALL NOT depend on the GUI. The GUI SHALL NOT manipulate
audio buffers. The web server SHALL NOT manipulate hardware registers or block
audio (§3).

### RES-ARCH-003 — Tasks and priority

Audio processing, SD buffering, GUI, network and session automation SHALL run
as separate FreeRTOS tasks where appropriate, with audio scheduled above
network and GUI work (§3).

### RES-ARCH-004 — Memory discipline

Buffer sizes, task stack sizes and heap use SHALL be explicit and budgeted for
an ESP32 without PSRAM. Dynamic allocation on the audio path SHALL be avoided
(§3).

### RES-ARCH-005 — Space first, and operating modes

Where a requirement can be met by a smaller means, the smaller means SHALL be
used. A subsystem not needed in the current operating mode SHALL NOT be
started. The firmware SHALL provide a **Play** mode (Bluetooth, audio,
playback, sessions) and a **Setup** mode (Wi-Fi, web administration, file
management), selected by the user and entered by a restart; a combined mode
MAY be offered if measurement on the board shows it to be reliable
(ADR-000204).

## Audio engine

### RES-AUDIO-001 — Continuous stereo engine

The engine SHALL produce continuous stereo audio at 44.1 kHz, the rate
Bluetooth A2DP fixes, mixing several sources at once, with at least 32-bit
internal processing. With the wired output on hold (RES-HW-003) no I2S driver
is used (§4, ADR-000204).

### RES-AUDIO-002 — Binaural generator

The left channel SHALL carry the primary frequency f0 and the right channel
f0 + fb, each from its own oscillator, with f0 and fb independently adjustable.
Initial ranges: f0 20–1000 Hz, fb 0.5–40 Hz, both in 0.1 Hz steps; the ranges
SHALL be configurable (§4.1).

### RES-AUDIO-003 — Phase continuity and smooth change

Frequency changes SHALL be phase-continuous. Level changes, start and stop
SHALL be faded so that no click or pop is produced; an immediate mute SHALL
also be available (§4.1, §17).

### RES-AUDIO-004 — Waveforms

Sine SHALL be provided. Triangle and experimental waveforms are *(optional)*
(§4.1).

### RES-AUDIO-005 — Parameter validity

The engine SHALL reject or clamp invalid and unstable parameter values (§4.1,
§17).

### RES-AUDIO-006 — Brainwave presets

Delta, Theta, Alpha, Beta and Gamma presets SHALL set an initial beat
frequency without restricting later adjustment; custom presets SHALL be
possible (§4.2).

### RES-AUDIO-007 — Mixer

A real-time stereo mixer SHALL combine three layers — binaural generator,
background audio, noise — each with its own on/off, volume, mute and fade,
under a master volume, with left/right balance (§5).

### RES-AUDIO-008 — Output limiting

Mixing SHALL prevent uncontrolled clipping: all layers at maximum SHALL still
give bounded, limited output. Maximum headphone level SHALL be reasonable and
limitable (§5, §17).

### RES-AUDIO-009 — Equaliser *(optional)*

Bass / mid / treble EQ MAY be provided if it does not compromise audio
performance (§5).

### RES-AUDIO-010 — Noise generator

White, pink and brown noise SHALL be provided with independent volume (§7).

### RES-AUDIO-011 — Output abstraction

Engine and mixer SHALL write to a common output interface and SHALL be
independent of the implementation behind it. The Bluetooth A2DP
implementation SHALL be provided; the wired I2S implementation and the
output selection return with RES-HW-003 (§10.1, ADR-000204).

### RES-AUDIO-012 — Audio task never blocks

The audio task SHALL NOT block on SD, network, GUI or logging (§6, §17).

## SD playback

### RES-AUDIO-013 — Channel phase and inversion

The tone engine SHALL allow a phase offset for each channel separately, in
degrees, and SHALL allow each channel to be inverted separately. A change of
either SHALL be spread over time so that the output has no step, and SHALL
not disturb the phase continuity of RES-AUDIO-003. With no offset and no
inversion the output SHALL be identical to the engine without this feature
(operator brief "Mindforge feature integration", 2026-11-22, §3.A.1).

### RES-AUDIO-014 — Golden partials

The tone engine SHALL be able to add two partials to each channel, at φ and
φ² times the primary frequency (φ = 1.618…), each with its own level and
each off unless switched on. In the right ear a partial SHALL lie above the
left ear's partial by the beat frequency itself, not by a multiple of it, so
that every pair beats at the selected beat frequency. The fundamental SHALL
not be altered by them (brief §3.A.2).

### RES-AUDIO-015 — Phase sweep

A channel's phase offset SHALL be able to move to a new value over a chosen
time, from a fraction of a second to an hour, without loss of precision over
that time. This is separate from the frequency sweeps of RES-SESS-001
(brief §3.A.4).

### RES-PLAY-001 — Formats

MP3 (common bitrates and sample rates) and standard PCM WAV files SHALL play
from the microSD card (§6).

### RES-PLAY-002 — Transport

Play/pause, stop, previous/next, track selection, repeat, shuffle, playback
position, duration where available, folder browsing and playlists SHALL be
provided (§6).

### RES-PLAY-003 — Buffered reads

SD reads SHALL be buffered in a task other than the audio task (§6).

### RES-PLAY-004 — Directory layout

The card SHALL use `/RESONANCE` with `AUDIO` (`OCEAN`, `NATURE`, `RAIN`,
`WIND`, `AMBIENT`, `MUSIC`, `CUSTOM`), `PLAYLISTS`, `PRESETS`, `SESSIONS` and
`CONFIG` (§6.1).

### RES-PLAY-005 — Card removal

Removal, replacement and read errors of the card SHALL NOT crash the device;
any library index SHALL be rebuilt or discarded safely (§6.1, §17).

### RES-PLAY-006 — Crossfade *(optional)*

Crossfade between background tracks MAY be provided (§7).

## Sessions and automation

### RES-SESS-001 — Sweep engine

An automation engine separate from the oscillator SHALL provide linear sweeps,
stepped changes and multi-stage progressions with start, end, duration and
optional hold, applicable separately to beat frequency, primary frequency,
background volume and binaural volume (§8).

### RES-SESS-002 — Staged sessions

A session SHALL consist of stages, each configuring duration, frequencies,
background track and volume, binaural volume, noise state and volume, sweep
and fade behaviour. User sessions SHALL persist (§8.1).

### RES-SESS-003 — Session control

Durations of 10, 15, 20, 30, 45 and 60 minutes and continuous SHALL be
offered, with start, pause, resume, stop, restart and cancel (§9).

### RES-SESS-004 — End-of-session fade

The end of a session SHALL offer immediate stop, 30 s, 1 min, 5 min or a
custom fade. A sleep session SHALL reduce volume gradually (§9).

### RES-SESS-005 — Independence from the GUI

A running session SHALL continue regardless of the screen shown, of screen
dimming and of GUI updates (§8, §9).

## Presets and configuration

### RES-SESS-006 — Sweep shapes and custom sweeps

A stage SHALL be able to move to its frequencies in a straight line or
exponentially (equal ratios in equal times), and SHALL be able to name the
frequencies it starts from instead of continuing from the previous stage, so
that a sweep with a chosen start, end and duration can be described. A stage
that names no start and sweeps in a straight line SHALL behave as before
(brief §3.A.3).

### RES-SESS-007 — Transition and hold, stage layers, session fade

A sweeping stage SHALL be able to reach its values in a part of its time and
hold them for the rest. A stage SHALL be able to set the levels of the
golden partials and the type and level of the noise; levels SHALL move with
the stage's transition, and a stage that names no layer SHALL leave the
layers as the user has them. A session SHALL be able to carry the length of
its own end fade. A session SHALL have at most ten stages (brief §3.C, §4).

### RES-SESS-008 — Progressive descent

A descent SHALL be describable by a starting pair of frequencies, up to five
target pairs, a transition time, a hold time and a linear or exponential
shape, and SHALL run on the session runner with pause, resume and stop like
any session (brief §3.B).

### RES-SESS-009 — User sessions on the card

User sessions SHALL be stored as text files under `/RESONANCE/SESSIONS` on
the card, at most twelve, and not in the non-volatile settings store. A file
SHALL be validated when it is read: a damaged, out-of-range or oversized
file SHALL be refused with a reason and SHALL never start. A file with
entries of a later version SHALL still load. Writing SHALL not leave a
half-written file under the session's name (brief §3.C, RES-CFG-002).

### RES-CFG-001 — Presets

A preset SHALL hold name, frequencies, waveform, layer volumes, background
selection, noise selection, session duration, sweep and fade settings. Factory
and user presets SHALL be provided (§15).

### RES-CFG-002 — Robust storage

Configuration SHALL be stored in a structured format, validated on load, and
SHALL fall back to safe defaults when a file is corrupt or incomplete (§15).

### RES-CFG-003 — Safe start

Audio SHALL NOT start by itself after boot, and loud audio SHALL NOT resume
after an unexpected restart (§2.2, §17).

## Bluetooth

### RES-BT-001 — A2DP source

The device SHALL stream its mixed output to Bluetooth headphones and speakers
as an A2DP source (§10).

### RES-BT-002 — Device management

Enable/disable, scan, list, pair, connect, disconnect, remembered devices,
automatic reconnection to the last device, selection among remembered devices,
forgetting one device, connection status and manual reconnect SHALL be
provided (§10).

### RES-BT-003 — Bonding owned by the stack

Pairing keys SHALL remain in the Bluetooth stack's own storage; the firmware
SHALL store only what it needs to identify remembered devices (§10).

### RES-BT-004 — Coexistence

Wi-Fi and Bluetooth operation together, including an upload during Bluetooth
playback, SHALL be tested. If playback is audibly interrupted, the limitation
SHALL be reported and an explicit coexistence behaviour implemented; playback
SHALL NOT be degraded silently (§10.1).

### RES-BT-005 — Loss of connection

A lost Bluetooth connection SHALL NOT crash the device (§17).

## Wi-Fi

### RES-NET-001 — Optional connectivity

All core functions SHALL work with Wi-Fi and Bluetooth off (§1).

### RES-NET-002 — Station and access point

Station mode with saved credentials and automatic connection, a configuration
access point on first boot, a fallback access point when the station
connection fails, reconnection after loss, manual disconnect and forgetting a
network SHALL be provided (§12).

### RES-NET-003 — Discovery

The device SHALL answer as `resonance.local` (configurable hostname) through
mDNS and SHALL show its IP address on the touchscreen (§12).

### RES-NET-004 — Credentials are not disclosed

Saved Wi-Fi passwords SHALL NOT appear in API responses or status pages (§12).

### RES-NET-005 — Network does not stop audio

Loss of Wi-Fi and network load SHALL NOT stop or interrupt the audio engine
(§3, §17).

## Web administration

### RES-WEB-001 — Local responsive interface

A browser interface for desktop, tablet and phone SHALL be served by the
device from local storage, without a cloud service, through documented
REST-style endpoints (§13).

### RES-WEB-002 — Panels

It SHALL provide Dashboard, Music Library, File Upload, Audio Configuration,
Session Configuration, Bluetooth Configuration and System Configuration with
the contents of §13.1–§13.7.

### RES-WEB-003 — State synchronisation

Changes made in the browser SHALL take effect on the device, and changes made
on the touchscreen SHALL appear in the browser, through WebSocket updates or
polling. In Setup mode, where audio does not run, this applies to settings,
presets, sessions and files (RES-ARCH-005) (§13.4).

### RES-WEB-004 — Uploads

MP3 and WAV uploads SHALL support several files, drag and drop, progress,
cancellation, a destination folder and the free-space display, and SHALL be
atomic: written to a temporary file and renamed on success (§13.3).

### RES-WEB-005 — Upload validation

Uploads SHALL be validated by extension and file signature, not by the
browser's MIME type. Path traversal and paths outside `/RESONANCE` SHALL be
refused. An existing file SHALL NOT be overwritten without explicit
confirmation (§6.1, §13.3).

### RES-WEB-006 — Authentication

File management, configuration and firmware update SHALL require
authentication. There SHALL be no unauthenticated upload service (§13.7, §17).

### RES-WEB-007 — Uploads and playback

An upload SHALL NOT block playback. If both together are unreliable on the
hardware, the interface SHALL warn or defer the upload (§13.3).

### RES-WEB-008 — System functions

Restart, configuration backup/restore and firmware version SHALL be provided.
OTA update is not provided: the flash holds one application slot
(ADR-000204); firmware is updated over USB and the procedure documented
(§13.7).

## Touchscreen GUI

### RES-GUI-001 — Five areas

The GUI SHALL have Home, Frequency Studio, Sound Library, Sessions and
Settings with the contents of §14.2–§14.6.

### RES-GUI-002 — Visual identity

The GUI SHALL use the RESONANCE identity: dark charcoal / midnight blue
background, cyan and teal highlights, violet accents, large touch targets and
clear typography, designed for 240 × 320, portrait by default (§14).

### RES-GUI-003 — Audio control is explicit

Home SHALL have a prominent audio on/off control (§14.2); see RES-CFG-003.

### RES-GUI-004 — Lightweight visualisation

Visualisations SHALL NOT affect audio timing, and a dimmed or
reduced-animation mode SHALL be available (§16).

On Home, while the audio is on, the rings SHALL pulse at the beat frequency;
a beat faster than 5 Hz is shown at that frequency halved until it is 5 Hz
or slower, because the screen cannot draw a faster motion truthfully. The
rings rest while the audio is off, and nothing is animated while the screen
is dimmed, on another screen, or in Setup mode. A reduced-animation setting
is not built yet.

### RES-GUI-005 — Responsiveness

Touch response SHALL remain acceptable during playback (§17).

## Deliverables

### RES-DOC-001 — Project documentation

The project SHALL include build instructions, a dependency list with versions,
hardware wiring and GPIO documentation, DAC and amplifier wiring, battery
documentation, the SD layout, the configuration format, the API, firmware
update instructions, a troubleshooting guide, test results and a bill of
materials (§20).
