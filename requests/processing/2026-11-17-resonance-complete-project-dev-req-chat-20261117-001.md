# Request: # RESONANCE
## Complete Project Development Specification
### ESP32-2432S032C-I 

**Request ID:** REQ-CHAT-20261117-001
**Source:** chat
**Type:** FEATURE
**Priority:** HIGH
**Created:** 2026-11-17T20:50:03-05:00
**Status:** processing
**Attachments:** - none

## Original User Statement

> # RESONANCE
## Complete Project Development Specification
### ESP32-2432S032C-I Binaural Meditation & Ambient Audio System

**Project name:** Resonance  
**Hardware:** Jingcai / Sunton ESP32-2432S032C-I CYD  
**Display:** 3.2-inch, 240 × 320 capacitive touchscreen  
**Firmware:** Arduino framework / C++  
**Primary purpose:** Standalone binaural meditation generator, stereo ambient audio player, and configurable audio environment.

---

# 1. PROJECT VISION

Build a polished, standalone binaural meditation and ambient audio device called RESONANCE.

This is intended to be a complete consumer-style embedded audio product, not merely a frequency generator demonstration.

The device must provide:

- Accurate, independently configurable stereo binaural tone generation.
- User-adjustable primary/carrier frequency.
- User-adjustable binaural beat frequency.
- Independent volume controls for each audio layer.
- MP3 and WAV playback from microSD.
- Real-time mixing of binaural tones and background audio.
- Ambient sound and noise generation.
- Bluetooth wireless headphone and speaker support.
- Wired stereo headphone support through an external I2S DAC.
- Wi-Fi connectivity and a browser-based administration interface.
- Remote uploading, organizing, and deleting audio files.
- Configurable meditation sessions and frequency progressions.
- Persistent configuration and saved presets.
- Rechargeable battery operation and USB power.
- Attractive, intuitive capacitive touchscreen GUI.

The device should function entirely offline after initial configuration. Wi-Fi and Bluetooth should be optional services, not requirements for basic operation.

The interface and overall experience should feel like a premium meditation instrument.

# 2. HARDWARE PLATFORM

Exact board:

Jingcai ESP32-2432S032C-I

Known characteristics:

- Original dual-core ESP32.
- 240 MHz processor.
- 4 MB onboard flash.
- No external PSRAM.
- 3.2-inch TFT display.
- 240 × 320 resolution class.
- ST7789 display controller.
- GT911 capacitive touchscreen.
- microSD card slot.
- Onboard NS8002 mono speaker amplifier.
- Onboard IP5306 battery charging/power management.
- Wi-Fi 2.4 GHz.
- Bluetooth Classic and BLE.
- USB-C connection.
- Battery connector.

IMPORTANT:

Do not assume this board is interchangeable with the common 2.8-inch resistive CYD.

Use the exact ESP32-2432S032C-I model and verify all hardware definitions against its schematic, manufacturer examples, board markings, and actual installed Arduino libraries.

The exact display initialization, touch configuration, SD interface, and GPIO assignments must be established before implementation.

## 2.1 GPIO audit

The following connectors have been identified on the board:

Connector A:
- GND
- IO35
- IO22
- IO21

Connector B:
- GND
- IO22
- IO21
- 3.3V

Connector C:
- +5V
- GND
- TX
- RX

Speaker connector:
- Two speaker amplifier output connections.

Known constraints:

- IO21 and IO22 are shared between connectors.
- IO35 is input-only.
- IO21/IO22 are associated with the capacitive touch interface and must not be reassigned without understanding their existing use.
- TX/RX correspond to the USB serial interface and require special consideration.
- The speaker connector is an amplified bridge-tied-load output. Never connect either speaker terminal to ground or use it as a GPIO.
- GPIO26 is associated with the onboard speaker amplifier input and must be evaluated separately.
- Onboard RGB LED signals and accessible test pads may offer additional GPIO possibilities.
- The SD card, display, and touch controller must continue working.

Before wiring the DAC:

1. Produce a complete GPIO ownership table.
2. Identify all existing peripheral assignments.
3. Identify available output-capable GPIOs.
4. Determine whether GPIOs can be reassigned through the ESP32 GPIO matrix.
5. Determine whether test pads or alternate connections are needed.
6. Produce a verified I2S wiring proposal.
7. Document conflicts and required hardware modifications.
8. Do not invent pin assignments.

Preserving the touchscreen, display, SD card, Wi-Fi, and Bluetooth functionality takes priority over convenient pin assignments.

## 2.2 Battery and power

The board includes an IP5306 charging/power management circuit.

Battery connector:
- Two-pin 1.25 mm Molex PicoBlade-compatible male connector.
- Single-cell LiPo/Li-ion battery.
- Nominal 3.7V, 4.2V fully charged.

Requirements:

- USB-powered operation.
- Battery-powered operation.
- USB charging.
- Battery status display if reliable voltage measurement is available.
- Safe battery integration.
- No automatic audio playback after boot.
- Graceful handling of low battery.
- Avoid sudden shutdown during SD writes.

Important: The board does not provide a confirmed battery protection circuit. Use a protected battery or appropriate external protection. Verify the exact charging and power-path behavior of the board.

Battery monitoring must be implemented only after identifying a valid measurement circuit or ADC connection. Do not assume a battery voltage divider exists.

# 3. SOFTWARE ARCHITECTURE

Use Arduino framework with C++.

Prefer a modular architecture rather than a monolithic sketch.

Suggested modules:

- ResonanceApp
- DisplayManager
- TouchManager
- AudioEngine
- BinauralGenerator
- AmbientAudioPlayer
- AudioMixer
- NoiseGenerator
- AudioOutputManager
- I2SAudioOutput
- BluetoothAudioOutput
- SessionManager
- FrequencySweepEngine
- PresetManager
- StorageManager
- SDCardManager
- WiFiManager
- WebServerManager
- UploadManager
- BluetoothManager
- BatteryManager
- ConfigurationManager

Separate hardware-specific implementations from application logic.

Use interfaces and well-defined ownership of shared resources.

The audio engine must not depend on the GUI implementation.

The GUI must never directly manipulate low-level audio buffers.

The web server must not directly manipulate hardware registers or block audio playback.

Use independent FreeRTOS tasks where appropriate.

Suggested task separation:

- Audio processing task.
- Audio playback/SD buffering task.
- GUI task.
- Network/web server task.
- Session and automation task.

Prioritize audio task scheduling and prevent network activity from interrupting continuous audio output.

Because the ESP32 has limited internal RAM and no PSRAM, explicitly manage buffer sizes, stack sizes, and memory fragmentation.

Avoid unnecessary dynamic allocation.

# 4. STEREO AUDIO ENGINE

This is the central subsystem.

Implement a continuous stereo audio processing engine.

Use I2S with DMA buffering.

Target audio configuration:

- 44.1 kHz or 48 kHz sample rate.
- Stereo output.
- 32-bit internal audio processing.
- Appropriate DAC output formatting.
- Phase-continuous oscillators.
- Smooth parameter transitions.
- Independent channel processing.

The engine must support multiple simultaneous audio sources.

## 4.1 Binaural generator

Generate independent left and right sine waves.

Left channel:

L(t) = A × sin(2π × f0 × t)

Right channel:

R(t) = A × sin(2π × (f0 + fb) × t)

Where:

- f0 = primary/carrier frequency.
- fb = binaural frequency difference.
- A = amplitude.

The frequency controls must be independent.

Suggested initial operating ranges:

Primary frequency:
- Minimum: 20 Hz.
- Maximum: 1000 Hz.
- Fine adjustment: 0.1 Hz.
- Coarse adjustment: configurable.

Binaural frequency:
- Minimum: 0.5 Hz.
- Maximum: 40 Hz.
- Fine adjustment: 0.1 Hz.

Ranges should be configurable.

Use phase accumulators or an equivalent efficient oscillator implementation.

Avoid discontinuities when changing frequency.

Implement:

- Sine waveform.
- Optional triangle waveform.
- Optional experimental waveform modes.
- Independent oscillator amplitude.
- Smooth fade in/out.
- Immediate mute.
- Phase-continuous frequency changes.

Never allow invalid or unstable parameter values.

## 4.2 Binaural presets

Provide preset starting points:

- Delta: 0.5–4 Hz.
- Theta: 4–8 Hz.
- Alpha: 8–12 Hz.
- Beta: 12–30 Hz.
- Gamma: 30–40 Hz.

Preset selection should set an initial value but never lock the user into that range.

Allow custom presets.

Do not claim that specific frequencies guarantee medical or neurological outcomes.

# 5. AUDIO MIXER

Implement a real-time stereo mixer.

Required audio layers:

1. Binaural generator.
2. Background audio player.
3. Noise generator.

Each layer must have:

- Independent ON/OFF control.
- Independent volume control.
- Independent mute.
- Independent fade control.

Provide a master volume control.

Use floating-point internal mixing where practical.

Implement appropriate clipping prevention and output limiting.

Ensure that mixing multiple maximum-volume sources cannot produce uncontrolled output.

Allow independent left/right channel levels and balance.

Optional EQ:

- Bass.
- Midrange.
- Treble.

Implement EQ only if it can be done without compromising audio performance.

# 6. MICROSD AUDIO PLAYER

Use the onboard microSD card.

Support:

- MP3 playback.
- WAV playback.
- Playlist support.
- Track selection.
- Play/pause.
- Stop.
- Previous/next.
- Repeat.
- Shuffle.
- Playback position.
- Track duration where available.
- Folder browsing.

WAV support should initially cover standard PCM WAV formats.

MP3 support should include common bitrates and sample rates.

Investigate compatible, maintained Arduino audio decoder libraries.

Evaluate ESP8266Audio or suitable successors and alternatives. Verify compatibility with the exact Arduino ESP32 core version.

Do not assume every library supports the required simultaneous I2S and Bluetooth output.

Implement buffered SD reads.

Never allow blocking SD reads to starve the audio processing task.

## 6.1 SD card directory structure

Use an organized directory layout:

/RESONANCE
    /AUDIO
        /OCEAN
        /NATURE
        /RAIN
        /WIND
        /AMBIENT
        /MUSIC
        /CUSTOM

    /PLAYLISTS
    /PRESETS
    /SESSIONS
    /CONFIG

Support creation and management of playlists.

Maintain a music library index if useful, but recover gracefully if the SD card is removed or replaced.

Validate file names and paths.

Do not permit path traversal or arbitrary filesystem access through web uploads.

# 7. NOISE AND AMBIENT GENERATION

Implement:

- White noise.
- Pink noise.
- Brown noise.

Each should have independent volume control.

Ambient background audio will primarily come from SD card files.

Support multiple background tracks and future sound-layer extensions.

Consider crossfade between tracks.

# 8. FREQUENCY SWEEP ENGINE

Implement an automation engine independent of the oscillator.

Support:

- Linear frequency sweep.
- Stepped frequency changes.
- Custom multi-stage progression.
- Adjustable start frequency.
- Adjustable end frequency.
- Adjustable duration.
- Optional hold periods.
- Smooth transitions.

Allow separate progression of:

- Binaural frequency.
- Primary/carrier frequency.
- Background volume.
- Binaural volume.

Allow users to save custom progression profiles.

A session must continue to run even if the user changes GUI screens.

## 8.1 Session editor

Support multiple stages within a session.

Each stage may configure:

- Duration.
- Primary frequency.
- Binaural frequency.
- Background track.
- Background volume.
- Binaural volume.
- Noise generator state.
- Noise volume.
- Frequency sweep behavior.
- Fade behavior.

Support stage transitions.

Persist user-created sessions.

# 9. SESSION MANAGEMENT

Provide preset durations:

- 10 minutes.
- 15 minutes.
- 20 minutes.
- 30 minutes.
- 45 minutes.
- 60 minutes.
- Continuous.

Provide session actions:

- Start.
- Pause.
- Resume.
- Stop.
- Restart.
- Cancel.

Implement independent fade-out behavior at session completion.

Allow the user to choose:

- Immediate stop.
- 30-second fade.
- 1-minute fade.
- 5-minute fade.
- Custom fade duration.

Sleep session behavior should gradually reduce volume.

The session engine must not depend on GUI updates or screen state.

# 10. BLUETOOTH AUDIO

The ESP32 supports Bluetooth Classic.

Implement Bluetooth A2DP Source functionality to stream audio to Bluetooth headphones and speakers.

Investigate the Arduino-compatible A2DP library and its current compatibility with the selected ESP32 Arduino core.

Requirements:

- Enable/disable Bluetooth.
- Scan for nearby audio devices.
- Display discovered devices.
- Pair with selected device.
- Connect.
- Disconnect.
- Remember previously paired devices.
- Automatically reconnect to last-used device.
- Select from previously paired devices.
- Forget individual devices.
- Display connection status.
- Handle connection loss.
- Allow manual reconnect.

Persist remembered device information where the Bluetooth stack permits.

Do not assume that arbitrary pairing keys can safely be reconstructed or stored independently of the ESP32 Bluetooth stack.

## 10.1 Audio output abstraction

Create a common audio output interface.

Implement:

- Wired I2S stereo DAC.
- Bluetooth A2DP output.

The audio engine and mixer should not care which output is active.

Allow the user to select the output.

If a wired headphone jack with detection is implemented, allow automatic output switching.

Bluetooth and Wi-Fi use the same 2.4 GHz radio.

Test concurrent operation, including music upload while Bluetooth audio is active.

If simultaneous operation causes unacceptable audio interruptions, report the limitation and implement sensible network/audio coexistence behavior.

Do not silently compromise playback.

# 11. WIRED STEREO AUDIO HARDWARE

Use an external PCM5102A I2S stereo DAC.

Add a proper stereo headphone amplifier.

Do not use a PAM8403 as a headphone amplifier. It is a speaker amplifier.

Investigate a suitable headphone driver such as MAX97220 or an equivalent module.

Provide:

- Left analog output.
- Right analog output.
- Ground.
- 3.5 mm stereo headphone jack.
- Optional line-level output.

Provide a wiring diagram and verified GPIO assignments.

The PCM5102A must be treated as a DAC, not a headphone power amplifier.

Use appropriate power supply decoupling and audio grounding.

Ensure that the onboard mono speaker amplifier cannot interfere with the new stereo output.

# 12. WI-FI CONNECTIVITY

Implement a robust Wi-Fi configuration system.

Support:

- Station mode.
- Access point mode.
- Initial configuration portal.
- Saved Wi-Fi credentials.
- Automatic connection on startup.
- Connection status.
- Reconnection after network loss.
- Manual disconnect.
- Forget network.
- Network information display.

On first boot, if no Wi-Fi credentials are configured, create a configuration hotspot.

Provide a browser-based configuration page.

Allow the user to configure:

- SSID.
- Password.
- Device hostname.
- Wi-Fi behavior.
- Web interface access credentials if supported.

Use mDNS where supported.

Suggested device address:

http://resonance.local

Also display the assigned IP address on the touchscreen.

Provide a fallback access point if station-mode connection fails.

Never expose saved Wi-Fi passwords in ordinary web API responses or device status pages.

# 13. WEB ADMINISTRATION INTERFACE

This is a major feature, not an afterthought.

Build a polished, responsive browser-based interface.

The interface must work on:

- Desktop browsers.
- Tablets.
- Mobile phones.

Use a lightweight frontend suitable for the ESP32's limited resources.

Do not require an external cloud service.

The web server should serve static assets from flash or another appropriate local resource, with a compact design.

Use REST-style endpoints or a clearly documented equivalent.

The interface should contain the following panels.

## 13.1 Dashboard

Display:

- Device name.
- Device online status.
- Wi-Fi connection status.
- Bluetooth status.
- Current audio output.
- SD card status.
- SD card capacity and free space.
- Current track.
- Playback state.
- Primary frequency.
- Binaural frequency.
- Volume levels.
- Active session.
- Remaining session time.
- Battery status when available.

Provide basic playback controls.

## 13.2 Music Library

Provide:

- Browse SD card.
- Browse folders.
- List audio files.
- Display file name.
- Display file type.
- Display file size.
- Display duration where available.
- Play selected track.
- Delete selected track.
- Rename files.
- Create folders.
- Manage playlists.

## 13.3 File Upload

Implement direct browser-based uploads to the microSD card.

Supported formats:

- MP3.
- WAV.

Requirements:

- Single-file upload.
- Multiple-file upload.
- Drag-and-drop upload on desktop.
- Mobile file picker.
- Upload progress indicator.
- Upload cancellation.
- File size display.
- Upload success/failure indication.
- Duplicate filename handling.
- Destination folder selection.
- Available SD space display.
- Safe handling of interrupted uploads.
- Atomic upload behavior using temporary files and rename on success.

Validate file extensions and basic file signatures.

Do not trust the browser-supplied MIME type.

Reject unsupported formats.

Prevent directory traversal and malicious paths.

Never overwrite an existing file without explicit confirmation.

Do not block audio playback during uploads.

If simultaneous upload and playback are not reliable on the available hardware, provide a clear warning or temporarily defer upload.

## 13.4 Audio Configuration

Provide configuration for:

- Primary frequency.
- Binaural frequency.
- Waveform.
- Binaural volume.
- Background volume.
- Noise volume.
- Master volume.
- Stereo balance.
- Audio output selection.
- Fade duration.
- Sample rate if supported.
- EQ if implemented.

Changes made in the web interface should update the device.

Changes made on the touchscreen should be reflected in the web interface.

Use a reliable synchronization mechanism, such as WebSocket updates or periodic status polling.

## 13.5 Session Configuration

Allow users to:

- Create sessions.
- Edit sessions.
- Delete sessions.
- Select session presets.
- Configure session duration.
- Configure frequency progressions.
- Configure audio layers.
- Configure fade behavior.
- Save custom session profiles.

## 13.6 Bluetooth Configuration

Provide:

- Bluetooth enable/disable.
- Scan.
- Discovered device list.
- Pair.
- Connect.
- Disconnect.
- Previously paired devices.
- Forget device.
- Connection state.

## 13.7 System Configuration

Provide:

- Device name.
- Wi-Fi configuration.
- Bluetooth configuration.
- Display brightness.
- Screen timeout.
- Theme.
- Startup volume.
- Default output.
- Automatic reconnect options.
- SD card status.
- Firmware version.
- Restart device.
- Configuration backup/restore.
- Firmware update through OTA where practical.

Protect sensitive configuration and administrative functions.

Use authentication for file management and device configuration.

Do not expose an unauthenticated file upload service to the local network.

# 14. TOUCHSCREEN GUI

Use the 3.2-inch capacitive touchscreen.

Use LVGL or another suitable Arduino-compatible GUI framework.

Verify the display driver and GT911 touch driver for the exact board.

The GUI must be visually polished and designed for 240 × 320 resolution.

Use a dark, atmospheric, modern appearance.

Project visual identity:

RESONANCE

Design language:

- Deep charcoal and midnight blue backgrounds.
- Cyan and teal highlights.
- Violet accents.
- Subtle glowing elements.
- Circular frequency visualizations.
- Smooth transitions.
- Large touch targets.
- Clear typography.
- Minimal visual clutter.

Avoid tiny controls.

Support portrait and landscape orientation if practical, with portrait as the initial default.

## 14.1 Main navigation

Create five main areas:

1. HOME
2. FREQUENCY STUDIO
3. SOUND LIBRARY
4. SESSIONS
5. SETTINGS

## 14.2 Home screen

Display:

- RESONANCE branding.
- Primary frequency.
- Binaural frequency.
- Left channel frequency.
- Right channel frequency.
- Animated visualization.
- Main play/pause control.
- Master volume.
- Background track information.
- Active session and remaining time.
- Bluetooth status.
- SD card status.
- Battery status when available.

Provide a prominent tone/audio ON/OFF control.

Audio should never start unexpectedly.

## 14.3 Frequency Studio

Provide:

- Primary frequency adjustment.
- Binaural frequency adjustment.
- Numeric entry.
- Increment/decrement controls.
- Fine adjustment.
- Coarse adjustment.
- Preset selection.
- Waveform selection.
- Frequency sweep configuration.

## 14.4 Sound Library

Provide:

- SD card browsing.
- Folder navigation.
- Track selection.
- Play/pause.
- Previous/next.
- Repeat.
- Shuffle.
- Layer volume controls.
- Current track information.

## 14.5 Sessions

Provide:

- Session presets.
- Session timer.
- Custom session creation.
- Session progression editor.
- Start/pause/resume/stop.
- Remaining time.
- Session history.

## 14.6 Settings

Provide:

- Audio output.
- Bluetooth devices.
- Wi-Fi configuration.
- Screen brightness.
- Display timeout.
- Battery status.
- Volume limits.
- Device information.
- System restart.

# 15. PRESET MANAGEMENT

Implement persistent presets.

A preset should contain:

- Preset name.
- Primary frequency.
- Binaural frequency.
- Waveform.
- Binaural volume.
- Background audio selection.
- Background volume.
- Noise selection.
- Noise volume.
- Session duration.
- Sweep configuration.
- Fade settings.

Provide factory presets and user-created presets.

Store configuration in a structured format such as JSON or another reliable compact representation.

Validate configuration when loading.

Recover safely from corrupt or incomplete configuration files.

# 16. VISUALIZATION

Provide attractive audio visualizations.

Ideas:

- Concentric frequency rings.
- Oscilloscope-style waveforms.
- Dual-channel waveforms.
- Animated frequency indicators.
- Subtle particle effects.
- Ambient color transitions.

Visualizations must be lightweight.

Never let rendering interfere with audio timing.

Provide a screen-dimming or reduced-animation mode.

# 17. RELIABILITY AND SAFETY

Important requirements:

- No clicks or pops during audio startup or shutdown.
- No abrupt frequency changes.
- No uncontrolled audio clipping.
- Smooth fade in/out.
- Reasonable maximum headphone output.
- Audio task must not block.
- SD errors must not crash the device.
- Wi-Fi loss must not stop the audio engine.
- Bluetooth disconnection must not crash the device.
- Touchscreen responsiveness must remain acceptable during playback.
- Recover safely after power loss.
- Never automatically resume loud audio after unexpected restart.
- Do not expose administrative file operations without authorization.
- Do not permit invalid configuration values.

Implement useful error reporting.

Provide serial diagnostic output during development.

Avoid excessive logging in production.

# 18. DEVELOPMENT PROCESS

Follow an audit-first, incremental implementation strategy.

Do not immediately generate the entire firmware.

## Stage 1 — Hardware and software audit

Before changing code:

1. Identify the exact board.
2. Verify display controller.
3. Verify capacitive touch controller.
4. Verify SD card connections.
5. Produce a GPIO ownership map.
6. Identify I2S pin options.
7. Identify battery monitoring options.
8. Identify onboard audio amplifier routing.
9. Evaluate Arduino libraries.
10. Evaluate Bluetooth A2DP support.
11. Evaluate MP3/WAV decoding.
12. Evaluate memory requirements.

Produce:

- HARDWARE.md
- GPIO_MAP.md
- ARCHITECTURE.md
- LIBRARY_EVALUATION.md
- IMPLEMENTATION_PLAN.md

Do not guess when hardware documentation is incomplete.

Clearly identify assumptions and required measurements.

## Stage 2 — Board foundation

Implement:

- Display.
- Touchscreen.
- SD card.
- Wi-Fi.
- Configuration storage.
- Basic navigation.

Test each independently.

## Stage 3 — Stereo audio prototype

Implement:

- I2S output.
- PCM5102A support.
- Stereo sine generation.
- Primary frequency.
- Binaural frequency.
- Volume.
- Mute.
- Smooth fades.

Verify actual stereo separation using headphones and audio test equipment.

## Stage 4 — Audio playback and mixing

Implement:

- WAV playback.
- MP3 playback.
- Audio mixer.
- Background audio.
- Noise generation.
- Independent layer volumes.

Test sustained playback and resource utilization.

## Stage 5 — Bluetooth

Implement:

- A2DP Source.
- Device scanning.
- Pairing.
- Remembered devices.
- Automatic reconnection.
- Output selection.

Test simultaneous Wi-Fi and Bluetooth.

## Stage 6 — Session engine

Implement:

- Session timer.
- Session presets.
- Frequency sweep.
- Custom progressions.
- Fade automation.
- Persistent sessions.

## Stage 7 — Web interface

Implement:

- Dashboard.
- Music library.
- MP3/WAV upload.
- File management.
- Configuration panel.
- Bluetooth management.
- Session management.
- OTA update.

Test using desktop and mobile browsers.

## Stage 8 — Product refinement

Implement:

- Complete GUI.
- Animations.
- Battery monitoring.
- Screen dimming.
- Error recovery.
- Startup behavior.
- Performance optimization.

# 19. ACCEPTANCE TESTS

The project is not complete simply because it compiles.

Provide repeatable tests.

Audio:
- Generate 200 Hz left and 210 Hz right.
- Verify independent channels.
- Verify accurate frequency output.
- Verify smooth frequency changes.
- Verify mute and fades.
- Verify no clipping when mixing layers.

Playback:
- Play MP3 from SD.
- Play WAV from SD.
- Mix background audio with binaural tones.
- Change volume during playback.
- Switch tracks.
- Test long-duration playback.

Bluetooth:
- Pair new headphones.
- Remember headphones.
- Restart device.
- Reconnect automatically.
- Disconnect and reconnect.
- Test Wi-Fi coexistence.

Wi-Fi:
- Connect to home network.
- Recover from network interruption.
- Start fallback hotspot.
- Open web interface from mobile.
- Upload MP3.
- Upload WAV.
- Delete and rename files.
- Verify SD capacity.
- Verify upload interruption recovery.

GUI:
- Verify all touch controls.
- Verify navigation.
- Verify frequency adjustments.
- Verify playback controls.
- Verify session controls.
- Verify display dimming.

Reliability:
- Run an extended audio playback test.
- Test SD removal and reinsertion.
- Test Wi-Fi interruption during playback.
- Test Bluetooth interruption.
- Test restart and power recovery.
- Monitor free heap and task stack usage.

Document actual results, not assumed success.

# 20. FINAL DELIVERABLES

Deliver a complete Arduino project.

Include:

- Compilable firmware.
- Source code organized into modules.
- PlatformIO or Arduino IDE build instructions.
- Dependency list with versions.
- Hardware wiring documentation.
- GPIO allocation table.
- DAC and headphone amplifier wiring.
- Battery connection documentation.
- Web interface source files.
- SD card directory documentation.
- Configuration format documentation.
- API documentation.
- Firmware update instructions.
- Troubleshooting guide.
- Test results.

Provide a clear bill of materials for any external hardware.

Do not silently omit features that prove difficult.

If a feature is not feasible on the original ESP32 hardware, explain the limitation and propose a practical alternative.

# 21. IMPLEMENTATION PHILOSOPHY

This project should prioritize:

1. Audio quality.
2. Reliability.
3. Correct hardware integration.
4. Responsive user experience.
5. Modular software architecture.
6. Maintainability.
7. Attractive design.

Avoid unnecessary complexity.

Use established libraries when appropriate, but verify their actual compatibility.

Do not introduce a cloud dependency.

Do not build a phone application when a responsive local web interface will accomplish the task.

Do not sacrifice audio stability for visual effects.

Do not make unsupported claims about frequency-related health benefits.

The finished product should be a self-contained, battery-capable, Wi-Fi-managed, Bluetooth-enabled stereo meditation instrument.

**Project identity: RESONANCE**

**Tagline: Find Your Frequency.**

## Classification

- Type: FEATURE (final; validated by `scripts/request_classification.py`, REQ-DEVOPS-045)
- Priority: HIGH
- Source: chat
- classification:
  - proposed_type: FEATURE (override)
  - final_type: FEATURE
  - heuristic_type: FEATURE (confidence high)
  - validation: PASS
  - classifier_reason: request statement is framed as feature: configurable, framework
  - evidence: 'framework' [head]; 'configurable' [head]; 'Build a ' [body]; 'must provide' [body]; 'configurable' [body]

## Inferred Intent (agent interpretation — not user statement)

Intent: implement requested capability per classification; derive requirement, implement, test, verify.
Conversational context: User pasted the full Resonance specification in chat, then said: 'add that as a ticket and proceed'.

## Observed / Unknown

- Observed: see original statement above.
- Unknown: exact scope — agent to derive
- Follow-ups will be appended under ## Follow-up Context


### Follow-up 2026-11-18T05:31:53.794812+00:00

> So - on the main 'Home' screen - the "Resonance" title sshould be centered. Remove the 'Play' from teh top. Make all other (Freq,snd,sess,set) the same size/font as 'resonance' on the home screen



### Follow-up 2026-11-18T05:26:56.978457+00:00

> also - for the buttons - a 'tap and hold' on a button should repeat after a brief pause. research and implement



### Follow-up 2026-11-18T05:25:32.104386+00:00

> so, about the 3d model and usage - if I were to hold the device in my left hand and look at it, on the lower right side of the device, there is a button or the on/off  . the sdcard is on the left center/top. there are 2 devices - a rgb led and a photoresistor, make sure the case exposes these. show me some pics of the new, fixed rendered models



### Follow-up 2026-11-18T05:23:36.762502+00:00

> So - in the sound library - if I go to the next screen using the > button, the screen show a < button that is centered. I would expect it to be on the left.



### Follow-up 2026-11-18T05:19:40.806791+00:00

> the saved settings leads to saving all previously manipulated values by the user.



### Follow-up 2026-11-18T05:17:47.967351+00:00

> So - let's talk about the 3d model. I was wondering why there were holes opn the back and the model doesn't have a place for the button on the side. I also want a seprate button with a lip that fits into a recess in the side of the case that is pressed against the actual button from the pcb.



### Follow-up 2026-11-18T05:12:37.808057+00:00

> How is the download of the media coming along for wav and mp3 imports? So - when the system reconnects - it says "connected to 'a remembered device'". I would like to remember the actual name of the connected device by storing a settings file on the SD card or in memory.



### Follow-up 2026-11-18T05:08:47.568938+00:00

> So - In setup mode - it requires a restart and you can't use BT to play sounds, right? If that is the case,then we should have a simplfied UI in Setup mode, let's talk about it. In the SND - Sound library - at the top - there is a '/'. Remove it.



### Follow-up 2026-11-18T05:06:37.800361+00:00

> So - when it says 'OFF' - don't put a % after that. It's off -it doesnt matter



### Follow-up 2026-11-18T05:04:17.492712+00:00

> So, I'm looking at the 3d models - at least the pngs - and I have questions and comments. Under Noise - the 'Brown' is a smaller font



### Follow-up 2026-11-18T04:57:20.451605+00:00

> the +0.1 button looks right now. put some  '<' '>' buttons next to the primary selection.



### Follow-up 2026-11-18T04:53:02.965016+00:00

> in the frequency studio - the +0.1 button is a smaller font.



### Follow-up 2026-11-18T04:50:39.829647+00:00

> I would like to see it. Where is the file? We need some preset frequencies when the user is in the frequency studio. something they can cycle through. There are lists of known good freqs for those in the know. (touches nose)



### Follow-up 2026-11-18T04:42:38.314436+00:00

> The battery is 90x60x11 approximately. I want the SDCard and USB and button for on/off - maybe a 3d printed button for it. I don't need the expansion ports.



### Follow-up 2026-11-18T04:37:02.317773+00:00

> In sessions -> programs, there should be a way to set the length - duration of the program.



### Follow-up 2026-11-18T04:35:39.341411+00:00

> I'm using the UI, and there is a way to decrease the volume with a '-' button, but there is not corresponding "+" button. Also - start finding some free sounds for oceans, nature, rain, wind, etc. find substantial material and copy it to the sdcard. I also heard a stutter in the tone. Not sure if it was a connection error. Let's also launch a parallel effort to design a case. Start searching for existing designs. I have a battery (3.7V) that is almost the exact dimesions. Also - how do we turn on / off this device if the battery is plugged in? I didn't see a switch. How is battery charging and usage typically handled on this device?



### Follow-up 2026-11-18T04:25:05.306199+00:00

> My headset powers on and says it's in connecting mode and when it connects, it tells me. is this a matter of BTLE compatibility? it found it. connecting now. hasnt connected yet



### Follow-up 2026-11-18T04:16:17.041154+00:00

> proceed with sessions, so - I am trying to connect my by headset to this device, and I don't think it's seeing it. So when I powered up my bt headset, it connected to my tablet, and said it was connected. I turned off the BT on my tablet and it went back to pairing mode. I also saw the Resonance device 'see' other bt devices, and I paired with my television. It looks like the device remembered the connection, which is desired.



### Follow-up 2026-11-18T03:38:52.752938+00:00

> I see it says "playback is not built yet" . What more need sto be done for this to be complete. Give a todo list and proceed



### Follow-up 2026-11-18T03:37:24.093293+00:00

> the 'sound' label is too long. Maybe make it "SND"



### Follow-up 2026-11-18T03:36:53.414238+00:00

> I tapped twice to put it into setup mode, then the screen went black. it's still black after power cycling



### Follow-up 2026-11-18T03:33:40.079577+00:00

> So - the bottom row - It has text - but I want them to look more like buttons. My BT audio earbuds are still charging. Also - format the 32GB SD card attached to my laptop. I have already backed it up



### Follow-up 2026-11-18T03:14:15.593118+00:00

> I conencted to the wifi and went to the address, nothing happened. The flicker and yellow dots appear fixed.



### Follow-up 2026-11-18T03:06:06.846657+00:00

> OK, the screen is brighter - but I see a ~1Hz flashing of the screen. At the top of the screen - where it says 'Resonance' - if I touch there - it leaves yellow dots that stay. How do I scan and connect to a BT audio device?



### Follow-up 2026-11-18T02:57:04.011826+00:00

> I see the screen with dark blue. It is very low contrast. We need to make that higher contrast. I can not seem to select anything on screen. Check the touch controller? I don't have a sd card right now. Let me grab one and you can check the touchscreen and adjust colors to be brighter



### Follow-up 2026-11-18T02:13:23.907983+00:00

> drop battery measurement. We are also going to put on hold the external stereo jack connected to i2s - we will rely on BT audio. Code conservatively for space. We may need to page swap in code. Begin implementation. The baord is plugged in


## Follow-up Context

User pasted the full Resonance specification in chat, then said: 'add that as a ticket and proceed'.

## Processing Notes

- This is the canonical persistent request artifact. Conversation history is contextual evidence; this file is authoritative.
- After intake, workflow is `requests/inbox -> processing -> intent -> requirement -> work -> implementation -> test -> verification -> documentation -> complete` per `docs/process/state-model.md` and `docs/process/autonomous-operating-model.md`.
- Next: agent claims via `scripts/claim.py`, then `scripts/preflight.py`, then requirement/work linkage.

## Traceability

- Intent: pending (created from this request)
- Requirement: pending
- Work: WORK-000001, WORK-000002, WORK-000003, WORK-000004, WORK-000005, WORK-000006, WORK-000007, WORK-000008, WORK-000009, WORK-000010, WORK-000011, WORK-000012, WORK-000013, WORK-000014, WORK-000015, WORK-000016, WORK-000017, WORK-000018, WORK-000019, WORK-000020, WORK-000021, WORK-000022, WORK-000023
- Evidence: pending
