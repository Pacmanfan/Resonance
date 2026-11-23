# Request: # RESONANCE — MASTER DEVELOPMENT DIRECTIVE
## Complete Audio Expansion, Mindfiel

**Request ID:** REQ-CHAT-20261123-001
**Source:** chat
**Type:** FEATURE
**Priority:** NORMAL
**Created:** 2026-11-23T11:20:58-05:00
**Status:** processing
**Attachments:** - none

## Original User Statement

> # RESONANCE — MASTER DEVELOPMENT DIRECTIVE
## Complete Audio Expansion, Mindfield Research Platform, RNG Experimentation, Visualization Suite, and Public Release

**Project:** Resonance  
**Local repository:** `/home/dev/Resonance`  
**GitHub:** `https://github.com/Pacmanfan/Resonance.git`  
**Current firmware baseline:** 0.8.2  
**Target license:** GPL-3.0  
**Hardware:** ESP32-2432S032C (ESP32-WROOM-32, CYD, 240×320 touchscreen, 4 MB flash, no PSRAM)

**MISSION: Implement the complete Resonance feature expansion described in this directive.**

This is an implementation assignment, not another planning or audit exercise.

Work directly in the existing project. Preserve working functionality, integrate with the existing architecture, and deliver tested, usable features.

Do not stop after producing recommendations, design documents, mockups, or a phased proposal. Implement the work, test it, and document what has been completed.

If a feature cannot safely be implemented on the current hardware, implement the best practical alternative, explain the limitation, and identify what additional hardware or future work would be needed.

---

# PART I — PRODUCT VISION

## 1. What Resonance is becoming

Resonance should remain an excellent, straightforward binaural meditation device while becoming a much more capable audio experimentation platform.

The product has three interconnected areas:

### Resonance Core
The standard meditation experience:
- Binaural tone generation.
- Frequency control.
- Noise generation.
- Audio playback.
- Audio mixing.
- Meditation sessions.
- Presets and settings.

### Resonance Studio
Advanced audio functionality:
- Advanced binaural DSP.
- Golden harmonics.
- Stereo and phase experimentation.
- Breathing pacer.
- Guided audio.
- Custom meditation programs.
- Modular audio layers.
- Solfeggio sequences.
- Resonance Lab.

### Resonance Mindfield
An experimental system investigating whether meditation and focused intention are associated with measurable changes in physical random-number-generator output.

Mindfield includes:
- Selectable random-number sources.
- Raw data collection.
- Statistical analysis.
- Experimental controls.
- Directed-intention and open-exploration sessions.
- Interactive visualizations.
- Experiment history.
- Data export.

All three areas must share the existing application, audio architecture, storage system, and touchscreen framework.

Do not create separate applications or require a separate boot mode.

## 2. Fundamental product principles

1. Standard binaural operation must remain simple and accessible.
2. Existing functionality must remain available.
3. Advanced features should be optional and accessed deliberately.
4. Mindfield must be a genuine measurement instrument, not merely an animated game.
5. Preserve raw RNG measurements.
6. Never represent an ordinary random fluctuation as evidence of successful influence.
7. Maintain offline operation.
8. Do not require cloud services or external servers.
9. Keep the user interface appropriate for a 240×320 display.
10. Design around actual hardware limitations.
11. Do not compromise audio stability to add visual features.
12. Prefer reusable engines over collections of unrelated implementations.

---

# PART II — EXISTING PROJECT BASELINE

## 3. Existing architecture

The repository already contains approximately 7,500 lines of project code.

The portable core is located in:

`firmware/lib/resonance_core/src/`

The device implementation is located in:

`firmware/src/`

The existing portable components include:
- Tone engine.
- Audio engine and mixer.
- Limiter.
- Noise generators.
- Resampler.
- WAV parser.
- Ring buffer.
- Session system.
- Presets.
- Settings validation.
- Path handling.

The existing device implementation includes:
- Main application.
- Audio output.
- File playback.
- GUI.
- Storage.
- Hardware abstraction.
- Existing application configuration.

The project uses:
- PlatformIO.
- Arduino framework based on ESP-IDF.
- LovyanGFX.
- ESP32-A2DP.
- Helix MP3 decoder.
- Custom lightweight GUI components.
- Host-side C++ tests.

Do not introduce LVGL or a large audio framework.

## 4. Existing hardware

The target hardware is the ESP32-2432S032C CYD.

Known configuration:
- ESP32-WROOM-32.
- 4 MB flash.
- No PSRAM.
- ST7789 240×320 display.
- GT911 touchscreen.
- microSD interface.
- Existing audio output architecture.
- Bluetooth audio headset support.

The onboard amplifier and speaker are currently unused.

The existing wired I2S output is not implemented.

Do not assume additional hardware is available.

## 5. Existing functionality — preserve it

The following already exists:

### Audio
- Two-channel binaural sine-wave generation.
- Independent left/right oscillators.
- Adjustable carrier and beat frequencies.
- Frequency presets.
- White, pink, and brown noise generation.
- Audio mixer.
- Master and source volume.
- Limiter.
- WAV playback.
- MP3 playback.
- Linear frequency sweeps.
- Existing fades and transitions.

### Sessions
- Seven factory programs.
- Session timer.
- Pause, resume, and stop.
- End fade.
- Existing stage and time tracking.

### Interface
- HOME.
- FREQ.
- SND.
- SESS.
- SET.
- Touchscreen controls.
- Backlight dimming.

### Storage
- SD card.
- Settings persistence.
- Existing audio file organization.

Do not rebuild the existing noise generators. They are already implemented. Verify their actual output during hardware testing.

Do not remove existing factory programs or presets.

Do not add session journaling or reflection entry. The CYD does not have a practical text-entry method.

---

# PART III — APPLICATION ARCHITECTURE AND MODE REMOVAL

## 6. Remove the separate Wi-Fi/Setup boot mode

The existing application has separate Play and Setup boot modes.

The Setup mode provides the Wi-Fi/web administration functionality and requires a restart to enter.

The owner has explicitly decided to remove this arrangement.

Implement the following:

- Remove the separate Setup boot mode.
- Remove boot-mode selection logic.
- Remove the corresponding persisted mode setting.
- Remove the Wi-Fi/web administration subsystem.
- Remove the web administration routes and supporting code.
- Remove dependencies used exclusively by that subsystem.
- Remove obsolete mode-related UI.
- Remove restart requirements that existed solely to switch modes.
- Preserve normal touchscreen configuration.
- Preserve settings persistence.
- Preserve offline operation.
- Preserve normal audio operation.

The device should always boot into its normal operating environment.

All configuration should be accessible through the touchscreen.

Do not introduce a replacement network administration system.

Before deleting code, identify all dependencies and shared components. Preserve anything required by the normal application.

Update the architecture documentation and tests.

Verify the resulting firmware build and actual device behavior.

Measure flash usage before and after.

Important: removing Setup mode may recover flash and simplify the application. Do not assume it will automatically recover active audio heap.

---

# PART IV — EXPANDED AUDIO ENGINE

## 7. Independent phase and inversion

Extend the existing tone engine.

Implement:
- Independent left-channel phase offset.
- Independent right-channel phase offset.
- Relative interaural phase.
- Left-channel inversion.
- Right-channel inversion.
- Phase continuity during parameter changes.
- Smooth transitions when applying new settings.

Do not introduce clicks or discontinuities.

The existing standard binaural output must remain unchanged when these options are disabled.

## 8. Golden harmonics

Implement optional golden-ratio partials.

Use:

\[
\phi = \frac{1+\sqrt{5}}{2}
\]

Support:
- Fundamental tone.
- φ-related partial.
- φ²-related partial.

Requirements:
- Independently enable or disable each partial.
- Independently adjust each partial's level.
- Preserve the fundamental carrier and beat relationship.
- Support appropriate stereo placement and phase configuration.
- Prevent clipping through appropriate mixer headroom and limiting.
- Smooth changes in harmonic levels.
- Avoid unnecessary computation in the audio render callback.

The golden partials should be an optional layer, not a replacement for the existing binaural tone.

## 9. Expanded sweep engine

Support:
- Linear sweeps.
- Exponential sweeps.
- Custom sweeps.
- Multi-stage sweeps.
- Configurable start frequency.
- Configurable end frequency.
- Configurable duration.
- Configurable hold duration.
- Smooth transitions.
- Pause and resume.
- Session-controlled sweeps.

Preserve existing linear sweep behavior.

## 10. Phase sweep

Implement controlled phase variation over time.

Support:
- Starting phase.
- Ending phase.
- Sweep duration.
- Sweep direction.
- Optional repeated phase sweep.
- Session integration.

This feature must be independently controllable and available in Resonance Lab.

## 11. Progressive frequency descent

Implement progressive descent sessions.

Support:
- Starting configuration.
- Target configuration.
- Multiple intermediate stages.
- Linear and exponential descent.
- Configurable hold periods.
- Configurable transition times.
- Smooth transitions.
- Optional noise and harmonic layers.
- Pause, resume, and stop.

Integrate with the existing session engine.

---

# PART V — MULTI-STAGE SESSIONS AND CUSTOM PROGRAMS

## 12. Extend the existing session architecture

Implement user-created meditation programs.

A session should support multiple stages, with each stage defining:

- Carrier frequency.
- Beat frequency.
- Duration.
- Sweep type.
- Transition behavior.
- Optional golden harmonics.
- Optional noise.
- Optional guided audio.
- Optional breathing pacer.
- Layer levels.
- Stage fade behavior.

Provide a touchscreen session editor.

Users should be able to:
- Create a session.
- Add stages.
- Delete stages.
- Select stages.
- Change stage order.
- Edit stage parameters.
- Preview configurations.
- Save sessions.
- Load sessions.
- Start sessions.
- Modify or delete saved sessions.

Use the existing session structures and validation wherever practical.

Persist custom sessions on the SD card.

Do not consume significant NVS space for collections of user-created programs.

Implement reasonable limits on the number of stages and saved sessions.

Preserve the seven existing factory programs.

---

# PART VI — AUDIO LAYERS AND GUIDED MEDITATION

## 13. Modular audio mixer

Extend the existing mixer into a lightweight layer-based architecture.

Initial layers:

1. Binaural fundamental.
2. Golden harmonics.
3. Noise.
4. Guided audio.

Each layer should support:
- Enable/disable.
- Independent level.
- Appropriate fade behavior.
- Session-controlled settings.
- Smooth transitions.

Preserve the existing master volume and limiter.

Do not introduce a large audio framework.

## 14. Guided audio

Extend the existing SD playback system.

Support:
- Guided voice recordings.
- WAV.
- MP3 where supported by the existing decoder.
- Voice mixed with binaural tones.
- Voice mixed with noise.
- Independent voice volume.
- Fade-in.
- Fade-out.
- Stage transitions.
- Optional sequencing of multiple recordings.

Continue streaming and decoding from SD.

Do not load complete recordings into RAM.

Implement crossfades where appropriate, but prioritize reliable playback over elaborate transitions.

## 15. Breathing pacer and coherence

Implement an optional breathing pacer.

Support:
- Inhale duration.
- Exhale duration.
- Optional hold after inhale.
- Optional hold after exhale.
- Repeating cycles.
- Visual breathing indicator.
- Optional audio modulation.
- Configurable modulation depth.
- Smooth amplitude modulation.
- Session integration.

The breathing indicator should be clear and calm.

Do not make breathing modulation mandatory.

It must be possible to use the device without the breathing pacer.

## 16. Solfeggio sequences

Implement editable Solfeggio sequences.

Support:
- Editable frequencies.
- Configurable stage durations.
- Multiple stages.
- Smooth transitions.
- Save and load.
- Integration with custom sessions.

Keep Solfeggio sequences distinct from ordinary binaural carrier/beat settings.

Do not hard-code a fixed list in a way that prevents user editing.

---

# PART VII — RESONANCE LAB

## 17. Advanced audio experimentation

Create an advanced Resonance Lab within the existing application.

Include:

- Golden bilateral stereo timing.
- Stereo width.
- Interaural delay.
- Independent left/right phase.
- Phase inversion.
- Phase sweep.
- Golden harmonics.
- Editable Solfeggio sequences.
- A/B comparison.

### A/B comparison

Users should be able to:
- Store configuration A.
- Store configuration B.
- Switch between them.
- Compare without losing either configuration.
- Match perceived loudness as closely as practical.
- Avoid abrupt transitions and clicks.

Do not add unnecessary calibration systems.

The Lab should be useful to experimenters without becoming part of normal Home operation.

---

# PART VIII — MINDFIELD EXPERIMENTAL PLATFORM

## 18. Mindfield overview

Implement a new experimental feature area called:

**Resonance Mindfield**

Mindfield investigates whether meditation and focused intention are associated with measurable changes in physical random-number-generator output.

This is an experimental research system.

Do not claim or imply that consciousness has been proven to influence random-number generators.

The system must distinguish:
- Actual RNG measurements.
- Statistical analysis.
- Visualization.
- User interpretation.

The visual display must never substitute for statistical evidence.

Mindfield must work offline and use the CYD's existing hardware.

## 19. RNG source architecture

This is a major requirement.

Do not hard-code one RNG source into Mindfield.

Create a selectable RNG source abstraction so the experimental system can use different entropy sources and compare them.

Define an interface conceptually equivalent to:

```cpp
class RngSource {
public:
    virtual bool initialize() = 0;
    virtual bool available() const = 0;
    virtual bool collect(uint8_t* buffer, size_t length) = 0;
    virtual const char* name() const = 0;
    virtual const char* status() const = 0;
};
```

Adapt the actual interface to the existing project's coding conventions and resource constraints.

Each RNG source must report its availability and limitations.

### Source A — ESP32 hardware RNG

<badge>Primary source</badge>

Use the ESP32 hardware RNG through the supported ESP-IDF API.

Relevant functions include:
- `esp_random()`
- `esp_fill_random()`

Requirements:
- Verify the actual ESP-IDF/Arduino implementation used by the project.
- Verify the entropy-source requirements for the specific ESP32.
- Confirm which physical entropy sources are active during normal operation.
- Do not assume that the RNG always has adequate physical entropy simply because the API returns values.
- Detect and report entropy-source availability where possible.
- Do not silently substitute a software PRNG.
- Preserve raw output for analysis.

This should be the default source if its physical entropy requirements are satisfied.

### Source B — Internal ADC noise

Investigate whether the ESP32's internal SAR ADC can provide an additional experimental noise source.

Requirements:
- Use supported ADC hardware APIs.
- Investigate ADC resource conflicts with existing audio and other peripherals.
- Do not enable entropy modes that interfere with existing application functions.
- Do not assume that ordinary ADC variation is true entropy.
- Characterize the raw signal.
- Determine whether it requires conditioning.
- Record sampling configuration.
- Report limitations.

If the SAR ADC entropy mechanism cannot coexist safely with the audio system, expose it as unavailable rather than compromising normal operation.

### Source C — Dedicated ADC noise experiment

If an available ADC input can safely be used, implement an experimental sampling mode.

Do not use a floating pin and call the result true randomness.

Investigate whether a properly configured external noise source would be required.

The software architecture should permit this source to be added later.

If no valid independent physical noise source exists on the CYD, document that limitation.

### Source D — Timing jitter experiment

Investigate whether measurable timing jitter from available hardware timers or oscillators can be collected.

Requirements:
- Identify the actual physical timing source.
- Distinguish physical jitter from scheduling variation and software artifacts.
- Avoid using touchscreen timing as a trusted entropy source.
- Do not present CPU scheduling irregularity as proven physical entropy.
- Keep this source experimental.
- Provide a way to compare its statistics with the hardware RNG.

### Source E — External RNG interface

Design an extension point for an external hardware RNG.

The current CYD does not have an external RNG installed.

Do not pretend this source is available.

Define a future interface that can support an external module over an appropriate serial interface if one is added later.

Document the expected interface and hardware requirements.

### Source F — Software PRNG control source

Implement a deterministic software PRNG option for testing and control experiments.

This is not a physical entropy source.

Use it for:
- Testing visualizations.
- Reproducing experiments.
- Testing statistical calculations.
- Demonstrating expected behavior.
- Verifying that the display does not introduce bias into measurements.

Clearly label it as a software-generated control source.

Never report it as true randomness.

## 20. RNG selection interface

Provide a touchscreen source-selection screen.

Show:
- Source name.
- Source type.
- Availability.
- Physical entropy status.
- Current sampling rate.
- Any known limitations.
- Whether the source is suitable for experimental collection.

Suggested source categories:

| Source | Category |
|---|---|
| ESP32 hardware RNG | Hardware entropy |
| Internal ADC | Experimental physical noise |
| ADC noise experiment | Experimental physical noise |
| Timing jitter | Experimental timing source |
| External RNG | Optional future hardware |
| Software PRNG | Deterministic control |

Do not show unavailable sources as selectable.

Do not label a source "true random" unless its physical entropy properties have been established.

## 21. Raw data collection

Implement a common sample collector.

Requirements:
- Fixed sampling schedule.
- Configurable sampling rate.
- Fixed measurement windows.
- Raw 32-bit sample collection where available.
- Timestamps or relative sample counters.
- Session identifiers.
- Source identifiers.
- Configuration metadata.
- Error and availability markers.
- SD card recording.
- Buffered writes.
- Recovery from interrupted experiments where practical.

Never discard raw samples after computing summary statistics.

Preserve the original data.

### Sampling rates

Provide configurable rates suitable for the hardware.

Start with conservative defaults:
- 100 samples/second.
- 250 samples/second.
- 500 samples/second.
- 1,000 samples/second.

Validate the actual source and SD throughput before exposing higher rates.

The sampling schedule must not depend on animation frame rate or user interaction.

### Data format

Use an efficient binary format for raw data.

Include a documented file header containing:
- Format version.
- Experiment identifier.
- RNG source.
- Sampling configuration.
- Start timestamp if available.
- Device and firmware version.
- Experimental condition.

Include enough information to reconstruct the experiment.

Store summary metadata separately or in a documented footer/index.

Provide an export option for raw data and summary results.

CSV export should be available where practical.

Do not write to the SD card from a real-time audio callback.

## 22. Statistical analysis engine

Implement a common statistical engine that is independent of visualization.

At minimum calculate:

### Bit balance
- Total bits.
- Number of 1s.
- Number of 0s.
- Proportion of 1s.
- Deviation from expected balance.

### Distribution
- Distribution of byte values.
- Distribution of selected bit patterns.
- Frequency of categorical outcomes.

### Runs
- Consecutive identical bits.
- Runs of 0s and 1s.
- Run-length distribution.

### Autocorrelation
- Correlation between successive samples or bits.
- Configurable lag measurements.

### Target deviation
- Difference between observed and expected target proportions.
- Standardized statistical score.
- Confidence intervals where appropriate.

### Time-window analysis
- Fixed measurement windows.
- Rolling summaries.
- Session-level summaries.
- Comparison between baseline and meditation periods.

Use mathematically appropriate statistical tests.

Avoid claiming statistical significance from arbitrary thresholds.

Account for repeated measurements and multiple comparisons when reporting significance.

Do not assume every measurement window is statistically independent.

Preserve enough data for external analysis.

## 23. Experimental modes

Implement two primary modes.

### Mode A — Directed intention

The participant chooses a desired outcome before the experiment.

Examples:
- More 1s than 0s.
- More 0s than 1s.
- Favor one categorical outcome.
- Encourage the Orb toward a selected target.
- Favor one side of a Balance visualization.

The target must be established before collection begins.

The system should calculate whether observed output differs from the expected distribution in the selected direction.

Do not change the target during an active experiment.

### Mode B — Open exploration

The participant has no predefined directional target.

The device records:
- Raw output.
- Statistical deviations.
- Time-window summaries.
- Session conditions.

This mode is intended to explore possible changes without imposing a particular direction.

### Control and baseline conditions

Implement:
- Resting baseline.
- Meditation condition.
- Control condition.
- Optional sham-meditation condition.

Support randomized or counterbalanced session ordering.

Record the condition in the experiment metadata.

Do not assume the user is meditating simply because a session has been started.

## 24. Experimental session workflow

A Mindfield experiment should follow this sequence:

1. Select RNG source.
2. Verify source availability.
3. Select visualization.
4. Select experiment mode.
5. Choose target if using directed intention.
6. Select session duration.
7. Select meditation audio/session if desired.
8. Establish baseline or control condition.
9. Start collection.
10. Run the visualization.
11. Stop or complete the experiment.
12. Calculate final results.
13. Save raw data and summaries.
14. Display results.
15. Allow review and export.

The participant should be able to run an experiment without audio if desired.

Mindfield must also integrate with Resonance meditation sessions.

Record which audio program and settings were active during the experiment.

---

# PART IX — MINDFIELD VISUALIZATION ENGINE

## 25. Shared visualization architecture

Create a reusable visualization interface.

Each visualization should consume common experiment data and render it through the existing LovyanGFX drawing framework.

Conceptually:

```cpp
class MindfieldVisualization {
public:
    virtual void initialize() = 0;
    virtual void update(const ExperimentMetrics& metrics) = 0;
    virtual void draw(/* existing graphics context */) = 0;
    virtual const char* name() const = 0;
};
```

Adapt to the existing GUI architecture.

Do not create separate RNG engines for individual visualizations.

Do not let a visualization modify raw samples or statistical calculations.

Visual smoothing must not change the underlying measurement.

Support deterministic replay using recorded data.

## 26. Visualization A — The Orb

<badge color="success">First priority</badge>

The primary visualization.

A glowing sphere starts in the center of a circular field.

The participant attempts to move it toward a target or toward the perimeter.

Requirements:
- Center starting position.
- Circular boundary.
- Radial movement.
- Directional target.
- Smooth motion.
- Visual representation of measured deviation.
- Clear neutral state.
- Configurable measurement threshold.
- Optional target direction.
- No automatic outward drift.

The Orb's position must represent a defined statistical measurement.

Do not simply integrate random values as a random walk.

A standardized statistical score is preferable.

The center should represent results consistent with expectation.

Movement outward should represent increasing evidence of deviation in the selected direction.

Movement inward should represent deviation in the opposite direction or return toward expectation, according to the chosen metric.

The animation may be smoothed, but the measured statistical score must remain available independently.

## 27. Visualization B — Balance

<badge color="success">First priority</badge>

A scale or balance beam begins centered.

The participant attempts to influence the result toward a selected side.

Requirements:
- Two opposing sides.
- Clear center reference.
- Adjustable target side.
- Visual balance indicator.
- Smooth movement.
- Measured statistical deviation.
- No arbitrary visual bias.

This visualization should be particularly suitable for binary experiments.

## 28. Visualization C — Growth

Implement a reusable growth engine with three visual styles.

### Tree
- Random branching decisions.
- Branch direction.
- Branch length.
- Branch generation.
- Cumulative visual history.

### Mandala
- Radial geometry.
- Repeating patterns.
- Symmetry.
- Incremental construction.
- Configurable complexity.

### Lotus
- Unfolding petals.
- Incremental growth.
- Radial symmetry.
- Smooth visual transitions.

The pattern should be reproducible from recorded experiment data.

The visualization should distinguish between random pattern development and statistically meaningful deviation.

## 29. Visualization D — Flow

Implement a reusable flow engine.

### River
Particles travel through branching channels.

Random decisions determine the direction of each particle.

### Labyrinth
A point moves through a maze.

Random decisions determine path choices.

The participant attempts to influence progress toward an exit.

### Waterfall
Particles enter multiple channels or destinations.

The resulting distribution represents observed outcomes.

All three should use the same underlying distribution engine where practical.

Avoid excessive particle counts and expensive rendering.

## 30. Visualization E — Intensity

Implement two lightweight visualizations.

### Star
A central star changes brightness and size according to measured deviation.

### Flame
A flame changes height and intensity according to measured deviation.

These should be lightweight, procedural animations suitable for the CYD.

They must not generate an artificial impression of success.

## 31. Additional visualization experiments

Implement these if the shared engine and performance budget permit:

### Vortex
- Radial and angular movement.
- Configurable target orbit.
- Statistical deviation mapping.

### Pendulum
- Oscillation between two extremes.
- Randomly measured influence represented through changes in amplitude or equilibrium.
- No arbitrary accumulation.

The initial release should prioritize:
1. Orb.
2. Balance.
3. Tree.
4. Mandala.
5. River.
6. Star.
7. Flame.

The remaining visualizations can follow once the shared engine is stable.

## 32. Visualization performance

The CYD has no framebuffer or PSRAM.

Requirements:
- Use the existing direct-drawing approach.
- Avoid large bitmaps.
- Avoid allocating memory every frame.
- Avoid excessive full-screen redraws.
- Use partial redraws where practical.
- Keep animation independent from RNG sampling.
- Maintain stable audio output.
- Allow the display to pause or reduce frame rate without changing experiment sampling.

The RNG engine must continue collecting at its configured rate even if rendering slows.

---

# PART X — USER INTERFACE INTEGRATION

## 33. Main application structure

Keep the normal Resonance experience simple.

Preserve the existing primary functions:
- Home.
- Frequency.
- Sound.
- Sessions.
- Settings.

Organize advanced functionality behind deliberate navigation.

Suggested arrangement:

**HOME**
- Current binaural state.
- Carrier and beat status.
- Volume.
- Active session.
- Audio state.

**FREQ**
- Existing frequency controls.
- Existing presets.
- Existing noise controls.

**SND**
- Existing file browser and playback.

**SESS**
- Factory sessions.
- Custom sessions.
- Session editor.

**STUDIO**
- Advanced audio configuration.
- Layer mixer.
- Breathing pacer.
- Resonance Lab.

**MINDFIELD**
- RNG source.
- Experiment selection.
- Visualization.
- Session controls.
- Results and history.

**SET**
- Display.
- Device settings.
- Storage status.
- About/version information.

If adding a permanent top-level tab would overcrowd the display, use a compact navigation entry or contextual menu.

Do not force all features onto the Home screen.

## 34. Mindfield UI

Implement the following screens:

### Mindfield home
- Select visualization.
- Start experiment.
- View recent experiment.
- Access settings and results.

### RNG source selection
- Available sources.
- Source status.
- Entropy status.
- Sampling rate.
- Source limitations.

### Experiment setup
- Directed intention.
- Open exploration.
- Control/baseline.
- Target selection.
- Duration.
- Visualization selection.
- Optional meditation program.

### Active experiment
- Full-screen visualization.
- Elapsed time.
- Sample count.
- Collection status.
- Pause/resume.
- Stop.

Do not display an emotionally manipulative live success score.

### Results
- Sample count.
- Observed distribution.
- Expected distribution.
- Deviation.
- Statistical score.
- Confidence interval where appropriate.
- Control/baseline comparison.
- Data file information.
- Export option.

### History
- Previous experiments.
- Date/time.
- Source.
- Condition.
- Visualization.
- Summary statistics.

Avoid requiring free-form text entry.

Use selectors, buttons, and sensible defaults.

## 35. Touchscreen interaction

The display is 240×320.

Use compact, readable controls.

Avoid:
- Long forms.
- Excessive menus.
- Tiny buttons.
- Large tables.
- Required text entry.

Use:
- Step controls.
- Selectable lists.
- Sliders.
- Compact cards.
- Clear state indicators.
- Consistent back navigation.

---

# PART XI — STORAGE AND EXPERIMENT MANAGEMENT

## 36. SD card organization

Use a dedicated folder structure.

Suggested:

`/Resonance/`
- `Audio/`
- `Sessions/`
- `Mindfield/`
  - `Experiments/`
  - `Exports/`
  - `Results/`
- `Presets/`

Adapt this to the existing storage architecture and avoid breaking existing audio paths.

## 37. Experiment persistence

Each experiment must have:
- Unique identifier.
- RNG source.
- Source configuration.
- Sampling rate.
- Experiment condition.
- Target if applicable.
- Visualization.
- Start and end information.
- Raw sample file.
- Summary file.
- Firmware version.

Provide:
- Experiment listing.
- Result review.
- Raw data export.
- CSV summary export.
- Recovery of incomplete experiment metadata where practical.
- Storage capacity checks.

Do not delete experiment data without explicit user action.

---

# PART XII — SCIENTIFIC VALIDITY

## 38. Experimental integrity

This is a critical requirement.

Mindfield must not manufacture apparent success through visualization choices or biased analysis.

Implement the following safeguards:

1. Define targets before experiments.
2. Preserve raw RNG data.
3. Keep RNG collection independent of visualization.
4. Use fixed sampling windows.
5. Record experimental conditions.
6. Keep baseline and control data.
7. Do not alter targets during active experiments.
8. Do not discard unfavorable measurements.
9. Do not hide unsuccessful sessions.
10. Do not selectively report favorable metrics.
11. Account for repeated statistical comparisons.
12. Distinguish exploratory results from confirmatory experiments.

Use established statistical methods.

For binary outcomes, use appropriate binomial tests and standardized effect estimates.

For multiple categories, use appropriate distribution tests.

For time-series data, investigate autocorrelation and dependence before treating windows as independent observations.

Do not assume a conventional p-value proves causation.

Provide clear explanations in the results interface and documentation.

## 39. Independent validation

Design the architecture so future experiments can compare the CYD's RNG output with an independent external hardware RNG.

For now, implement the external-source interface but do not claim it is present.

Provide deterministic software-generated test sequences for validating:
- Visualizations.
- Statistical calculations.
- Target mapping.
- Data recording.
- Replay.

The software PRNG is a control and testing source only.

---

# PART XIII — MEMORY, PERFORMANCE, AND REAL-TIME SAFETY

## 40. Critical hardware constraints

The audit measured:
- Approximately 94.6 KB free heap without active playback.
- Approximately 33 KB free heap during MP3 playback.
- Low point around 24 KB.
- Minimum safety floor of 16 KB during MP3 playback.

The project has no PSRAM.

The audio render callback must remain:
- Nonblocking.
- Allocation-free.
- Free of file I/O.
- Free of logging.
- Free of unnecessary calculations.

Do not put statistical analysis, SD writing, visualization rendering, or session persistence in the audio callback.

## 41. Task separation

Use a clear separation between:

- Audio rendering.
- File decoding.
- RNG sampling.
- Statistical calculations.
- SD data writing.
- GUI rendering.
- Session control.

Use fixed-size buffers and bounded queues.

Avoid unnecessary task creation.

Do not increase task stack sizes without measuring actual usage.

Avoid priority inversion and blocking dependencies.

Do not allow Mindfield collection to starve audio rendering.

## 42. Memory validation

Measure heap usage under these conditions:

1. Device idle.
2. Binaural audio.
3. Noise enabled.
4. Guided MP3.
5. MP3 plus binaural audio.
6. MP3 plus all practical audio layers.
7. Mindfield RNG collection.
8. Mindfield visualization.
9. Mindfield with audio and SD recording.
10. Full combined operation.

The system must remain above the measured 16 KB safety floor during active MP3 playback.

If necessary:
- Reduce buffer sizes.
- Reduce visualization complexity.
- Lower sampling rates.
- Reduce task stacks.
- Move processing outside constrained tasks.
- Defer nonessential features.

Do not compromise the standard audio experience.

---

# PART XIV — PUBLIC REPOSITORY AND GPL RELEASE

## 43. GitHub repository

Use:

`https://github.com/Pacmanfan/Resonance.git`

Preserve existing Git history.

Do not create a new unrelated history.

Inspect the remote before pushing.

Avoid force pushes.

Keep `master` as the stable branch.

Use feature branches if appropriate.

Review the working tree and `.gitignore`.

Do not commit:
- Build directories.
- Temporary audit files.
- Credentials.
- Wi-Fi passwords.
- Personal recordings.
- Private information.
- Unrelated local files.

## 44. GPL-3.0

Add the complete official GPL-3.0 license text.

Review third-party licenses.

Preserve original copyright and attribution notices.

Do not assume all third-party libraries have the same license.

Document any licensing conflicts or unresolved ownership questions.

## 45. Project documentation

Create and maintain:

- `README.md`
- `LICENSE`
- `CHANGELOG.md`
- `CONTRIBUTING.md`
- `docs/ARCHITECTURE.md`
- `docs/DSP.md`
- `docs/HARDWARE.md`
- `docs/ROADMAP.md`
- `docs/MINDFIELD.md`
- `hardware/README.md`

README requirements:
- Project overview.
- Actual implemented capabilities.
- Hardware description.
- Architecture overview.
- Build instructions.
- Test instructions.
- CAD links.
- Project roadmap.
- License.
- Screenshots and preview images.

Clearly distinguish implemented, tested, unverified, planned, and experimental functionality.

Do not advertise unfinished functionality as complete.

## 46. CAD publication

Publish the actual enclosure design files.

Include:
- OpenSCAD source.
- Corresponding STL exports.
- Preview images.
- Revision information.
- Build and modification instructions.

Preserve all useful historical revisions.

The v4 and v4 slim designs have been rendered but not physically printed and validated. Clearly identify this.

Do not substitute generated concept art for actual CAD previews.

Use actual OpenSCAD rendering or another suitable CAD renderer.

Suggested structure:

`hardware/`
- `README.md`
- `openscad/`
- `stl/`
- `previews/`
- `revisions/`

Verify that each STL matches its source revision.

## 47. Project visual identity

Establish a cohesive visual language for Resonance.

Create:
- README banner.
- GitHub social preview.
- Actual device screenshots if available.
- Actual CAD preview images.
- Mindfield concept images.
- Visualization assets where appropriate.

Use a restrained dark scientific aesthetic with subtle luminous color.

Generated concept imagery must be labeled as concept art.

Actual hardware images and CAD renders must remain factual representations.

---

# PART XV — TESTING AND VALIDATION

## 48. Preserve the existing test suite

Run:

`firmware/tools/host_tests.py`

All existing nine host-test programs must continue passing.

Do not delete or weaken existing tests to accommodate new functionality.

## 49. Add tests for expanded audio

Test:
- Phase offsets.
- Inversion.
- Phase continuity.
- Golden harmonic frequencies.
- Harmonic levels.
- Linear sweeps.
- Exponential sweeps.
- Custom sweeps.
- Multi-stage transitions.
- Mixer behavior.
- Crossfades.
- Breathing modulation.
- Session persistence.
- Solfeggio sequences.
- A/B comparison.

## 50. Add tests for Mindfield

Test:
- RNG source availability.
- Source selection.
- Source failure.
- Source metadata.
- Raw sample collection.
- Sampling schedule.
- Binary file format.
- Interrupted collection.
- Data integrity.
- Bit balance.
- Distribution.
- Runs.
- Autocorrelation.
- Target deviation.
- Confidence intervals.
- Multiple-comparison handling.
- Control conditions.
- Session metadata.
- Visualization mapping.
- Deterministic replay.

Include known synthetic datasets:
- Perfectly balanced binary data.
- Biased binary data.
- Repeating patterns.
- Known distributions.
- Deterministic sequences.

Verify that the statistical engine returns expected results.

## 51. Physical hardware tests

Host tests are not sufficient.

Verify on the actual CYD:

- Normal boot.
- Touchscreen operation.
- Frequency controls.
- Existing audio.
- Noise output.
- SD playback.
- Existing factory sessions.
- Custom sessions.
- Audio layer mixing.
- Mindfield collection.
- SD recording.
- Visualization operation.
- Long-duration operation.
- Recovery after interruption.
- No audio dropouts.
- No memory exhaustion.

Document any tests that cannot be performed.

---

# PART XVI — IMPLEMENTATION EXECUTION

## 52. Execute in dependency order

This is a large project. Implement it in coherent phases, but continue through the complete scope.

Do not stop after each phase merely to ask for approval.

Use the following order unless actual code dependencies justify a change.

### Phase 1 — Repository and application cleanup
- Preserve Git history.
- Verify GitHub remote.
- Add GPL-3.0.
- Remove Wi-Fi/Setup boot mode.
- Remove obsolete web administration.
- Update build configuration.
- Verify normal application.
- Update documentation.

### Phase 2 — RNG infrastructure
- RNG source interface.
- ESP32 hardware RNG.
- Source validation.
- Internal ADC investigation and implementation if safe.
- Timing jitter experiment if defensible.
- Software PRNG control.
- External RNG extension point.
- Source-selection UI.

### Phase 3 — Mindfield collection and statistics
- Sample collector.
- Sampling schedule.
- Binary recording.
- SD storage.
- Experiment metadata.
- Statistical engine.
- Baseline and control modes.
- Results interface.

### Phase 4 — First Mindfield visualizations
- Orb.
- Balance.
- Tree.
- Mandala.
- Deterministic replay.
- Performance validation.

### Phase 5 — Remaining visualization families
- River.
- Labyrinth.
- Waterfall.
- Star.
- Flame.
- Vortex.
- Pendulum.
- Lotus.

### Phase 6 — Expanded audio DSP
- Phase controls.
- Inversion.
- Golden harmonics.
- Exponential sweeps.
- Custom sweeps.
- Phase sweep.

### Phase 7 — Sessions and mixer
- Multi-stage sessions.
- Progressive descent.
- Custom session editor.
- Modular mixer.
- Guided audio.
- Crossfades.
- Breathing pacer.
- Solfeggio sequences.

### Phase 8 — Resonance Lab
- Stereo width.
- Interaural delay.
- Golden bilateral timing.
- Advanced phase experimentation.
- A/B comparison.
- Advanced UI.

### Phase 9 — Complete integration
- Mindfield and meditation session integration.
- Combined audio/RNG/SD/display testing.
- Memory profiling.
- Performance optimization.
- Long-duration hardware testing.

### Phase 10 — Public release
- Complete documentation.
- CAD source and STL publication.
- CAD preview images.
- README artwork.
- Changelog.
- Contribution instructions.
- Final license review.
- GitHub release.
- Release tag.
- Final verification.

## 53. Development process

For each phase:

1. Inspect the relevant existing implementation.
2. Identify the smallest appropriate architectural changes.
3. Implement working functionality.
4. Add tests.
5. Run the tests.
6. Build the firmware.
7. Measure resource use.
8. Update documentation.
9. Commit the completed work.
10. Continue to the next phase.

Maintain a development log documenting:
- Completed features.
- Current phase.
- Files modified.
- Tests run.
- Build results.
- Heap measurements.
- Hardware verification.
- Known limitations.

Use meaningful Git commits.

Do not create one enormous unreviewable commit.

Do not leave incomplete placeholder implementations while reporting the feature as complete.

If a feature is blocked by hardware limitations, implement the software interface and report the exact blocker.

Do not invent hardware capabilities.

## 54. Final acceptance criteria

The project is considered complete when:

### Existing Resonance
- Standard binaural operation remains functional.
- Existing noise generators remain functional.
- Existing audio playback remains functional.
- Existing presets and factory sessions remain available.
- Touchscreen operation remains reliable.
- Separate Wi-Fi/Setup boot mode is removed.
- No restart is required to access ordinary configuration.

### Audio expansion
- Independent phase and inversion work.
- Golden harmonics work.
- Advanced sweeps work.
- Progressive descent works.
- Custom sessions can be created and saved.
- Guided audio can be mixed.
- Breathing pacer works.
- Solfeggio sequences are editable.
- Resonance Lab works.
- A/B comparison works.

### Mindfield
- RNG source selection works.
- Physical entropy availability is reported accurately.
- Software PRNG is clearly labeled as a control source.
- Raw data is preserved.
- Experiment metadata is recorded.
- Statistical calculations are validated.
- Baseline and meditation conditions are supported.
- Directed and open-exploration modes work.
- Orb and Balance work.
- Growth visualizations work.
- Flow and Intensity visualizations work.
- Additional visualizations work where hardware permits.
- Results can be reviewed and exported.

### Reliability
- Existing host tests pass.
- New tests pass.
- Audio remains stable.
- Memory remains above the established safety floor.
- Long-duration experiments do not exhaust storage or memory.
- Interrupted experiments are handled safely.

### Public project
- Git history is preserved.
- GitHub repository is populated.
- GPL-3.0 is included.
- README and documentation are complete.
- CAD source is published.
- STL exports are published.
- CAD preview images are included.
- Project release is tagged and documented.

---

# FINAL INSTRUCTION

**Go for the complete Resonance evolution.**

Do not reduce this assignment to an RNG demonstration or a collection of animated graphics.

The goal is a complete, integrated product:

- A capable binaural meditation device.
- An advanced audio laboratory.
- A customizable meditation-session platform.
- A physical RNG experimentation system.
- A collection of engaging scientific visualizations.
- An open-source hardware and software project.

Preserve the simplicity of normal Resonance operation while making the advanced features genuinely useful.

The experimental system must be scientifically honest. The audio system must remain reliable. The user interface must remain usable on the CYD. The public repository must be reproducible and properly documented.

**Implement the features, test them, document them, and deliver a working product.**

## Classification

- Type: FEATURE (final; validated by `scripts/request_classification.py`, REQ-DEVOPS-045)
- Priority: NORMAL
- Source: chat
- classification:
  - proposed_type: FEATURE (override)
  - final_type: FEATURE
  - heuristic_type: FEATURE (confidence low)
  - validation: PASS
  - classifier_reason: no framing in the request statement; body suggests feature: Add, Allow, Build, Build the 
  - evidence: 'Implement' [body]; 'feature' [body]; 'integrate' [body]; 'deliver' [body]; 'features' [body]

## Inferred Intent (agent interpretation — not user statement)

Intent: implement requested capability per classification; derive requirement, implement, test, verify.

## Observed / Unknown

- Observed: see original statement above.
- Unknown: exact scope — agent to derive
- Follow-ups will be appended under ## Follow-up Context

## Follow-up Context

_No follow-up yet._

## Processing Notes

- This is the canonical persistent request artifact. Conversation history is contextual evidence; this file is authoritative.
- After intake, workflow is `requests/inbox -> processing -> intent -> requirement -> work -> implementation -> test -> verification -> documentation -> complete` per `docs/process/state-model.md` and `docs/process/autonomous-operating-model.md`.
- Next: agent claims via `scripts/claim.py`, then `scripts/preflight.py`, then requirement/work linkage.

## Traceability

- Intent: pending (created from this request)
- Requirement: pending
- Work: WORK-000046, WORK-000047
- Evidence: pending
