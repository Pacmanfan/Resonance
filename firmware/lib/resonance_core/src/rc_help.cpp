#include "rc_help.h"

namespace rc {

namespace {

struct Topic {
    const char* title;
    const char* text;
};

const Topic kTopics[kHelpTopics] = {
    {"START HERE",
     "# What can sound\n"
     "Four things, alone or together:\n"
     "TONE - two tones, one in each ear. Switched on and off on HOME. Its pitch is set on FREQ.\n"
     "GOLD - two extra tones on top. Set under ADV, ADVANCED, LAYER MIXER. It sounds only while the tone is on.\n"
     "NOISE - white, pink or brown. The bottom row of FREQ.\n"
     "FILE - a recording from the card. Chosen on SND.\n"
     "\n"
     "# What is on now\n"
     "The last line of HOME names what is sounding, or says Silent.\n"
     "\n"
     "# To stop each one\n"
     "Tone and gold: the middle button on HOME, until it says OFF.\n"
     "Noise: tap its button on FREQ until it says OFF.\n"
     "File: PAUSE on SND.\n"
     "A session: STOP on SESS.\n"
     "\n"
     "# Help\n"
     "Tap ? at the top right of any screen for help on that screen. ALL TOPICS lists every page."},
    {"HOME",
     "# The rings\n"
     "They pulse with the beat between the two tones. The numbers show the beat, and the tone in the left (L) and right (R) ear.\n"
     "\n"
     "# Sound\n"
     "The middle button turns the tone ON or OFF. Minus and plus set the volume. Nothing sounds by itself after switching on.\n"
     "\n"
     "# Bluetooth\n"
     "The BT button shows the headset or speaker in use. Tap it to choose one: put the headset in pairing mode and tap its name.\n"
     "\n"
     "# Last lines\n"
     "The card or a running session, then what is sounding now."},
    {"FREQUENCY",
     "# Two numbers\n"
     "PRIMARY is the tone in the left ear, 20 to 1000 Hz. BEAT is how much higher the right ear is, 0.5 to 40 Hz. Each ear hears a steady tone; the beat is the difference between them, so use headphones.\n"
     "\n"
     "# Changing them\n"
     "The small buttons step up and down; hold one to keep stepping. The arrows beside a value go to the previous or next preset, and the name of the preset appears above. A preset is only a starting value.\n"
     "\n"
     "# Noise\n"
     "The bottom row. The middle button goes round OFF, WHITE, PINK, BROWN. Minus and plus set its level."},
    {"SOUND",
     "# Files from the card\n"
     "Put WAV or MP3 files in the folder RESONANCE/AUDIO on the card, in folders if you like. The card is read when the device starts.\n"
     "\n"
     "# Playing\n"
     "Tap a folder to open it, UP to go back, a file to play it. The arrows turn the page of a long list. When a file ends the next one in the folder follows.\n"
     "\n"
     "# Controls\n"
     "PAUSE and PLAY, and minus and plus for the level of the file. A file plays together with the tone and the noise."},
    {"SESSIONS",
     "# What a session is\n"
     "A timed run that turns the tone on, moves the beat as programmed, and fades out at the end.\n"
     "\n"
     "# Choosing\n"
     "Program goes round a plain timer and the built-in programs. Length sets how long it runs. Fade out sets the fade at the end.\n"
     "\n"
     "# Running\n"
     "START begins. While it runs you can PAUSE, RESUME or STOP, and the stage and time left are shown.\n"
     "\n"
     "# Your own\n"
     "BUILD opens the session builder."},
    {"ADV TAB",
     "# Brightness\n"
     "Minus and plus. The screen dims by itself after a while; touch it to wake it.\n"
     "\n"
     "# ADVANCED\n"
     "Layer mixer, breathing, session builder, sequence and Resonance Lab.\n"
     "\n"
     "# MINDFIELD\n"
     "Experiments with random numbers. See its help.\n"
     "\n"
     "# Below\n"
     "The firmware version and free memory.\n"
     "\n"
     "# Getting back\n"
     "On every screen under ADV, tap the < at the top left to go back one step."},
    {"LAYER MIXER",
     "# Four layers\n"
     "TONE, GOLD, NOISE and SOUND (the file). Tap a name to switch that layer on or off. Minus and plus set its level.\n"
     "\n"
     "# Golden 1 and 2\n"
     "Two extra tones above the primary, at 1.618 and 2.618 times its pitch. Off at first. They belong to the GOLD layer.\n"
     "\n"
     "# Fade\n"
     "How long a layer takes to come and go, and how one file gives way to the next.\n"
     "\n"
     "A layer switched off here stays silent whatever the other screens say. These settings are kept on the card."},
    {"BREATHING",
     "# The pacer\n"
     "A bar that fills as you breathe in and empties as you breathe out. Set the seconds for Inhale, Hold, Exhale and the second Hold; a hold of 0 is left out.\n"
     "\n"
     "# In sound\n"
     "At 0 the pacer is only seen. Above 0 the tone and the noise swell and sink with it by that much.\n"
     "\n"
     "# On and off\n"
     "The button at the bottom starts and stops it. While it runs a thin bar also shows on HOME."},
    {"BUILDER",
     "# Stages\n"
     "A session is a list of up to ten stages. NEW starts an empty one. ADD and REMOVE change the list, EARLIER and LATER move the chosen stage, EDIT opens it.\n"
     "\n"
     "# A stage\n"
     "Its length, its primary and beat, how it moves there (Shape) and how long the move takes (Move in). The buttons below take what is set on the device now - golden tones, noise, breathing, the file playing - into the stage; tap again to drop it.\n"
     "\n"
     "# Keeping it\n"
     "SAVE and LOAD use twelve places on the card. START runs it."},
    {"SEQUENCE",
     "# A row of pitches\n"
     "Up to ten primary frequencies, each held for its own time. It starts as the Solfeggio set and every value can be changed.\n"
     "\n"
     "# Editing\n"
     "Choose a step with the arrows, set its frequency and length, ADD or REMOVE steps. Glide makes each step slide to the next instead of jumping. RESET brings the first set back.\n"
     "\n"
     "# Using it\n"
     "USE AS SESSION turns it into a session in the builder, where it can be started and saved. SAVE and LOAD keep sequences on the card."},
    {"THE LAB",
     "# Experiments with the tone\n"
     "They act on the tone, gold and noise, not on a file.\n"
     "Width: 100 is two separate ears, 0 is the same in both.\n"
     "Delay us: one ear later than the other, in millionths of a second.\n"
     "Phase L and R, INV L and R: shift or turn over one ear's wave.\n"
     "SWEEP: turns the right ear's phase once round in 30 seconds.\n"
     "The last two rows: the sound moves from ear to ear with that period and depth; GOLDEN or EVEN sets the timing.\n"
     "\n"
     "# A and B\n"
     "Two settings to compare. Tap A or B to hear one. MATCH turns the louder one down so that they sound alike in level."},
    {"MINDFIELD",
     "# What it is\n"
     "An instrument that records the output of a random-number source while you meditate, or do not, and works out whether it differs from chance. It is an experiment. Nothing here shows that the mind can change random numbers, and a picture that moves is not a result.\n"
     "\n"
     "# One experiment\n"
     "1. SOURCE: where the numbers come from.\n"
     "2. SET UP EXPERIMENT: what you will try, for how long, and the picture.\n"
     "3. START. The picture runs; STOP ends it early.\n"
     "4. The RESULT appears. DONE closes it and stops all sound.\n"
     "\n"
     "# First\n"
     "Record a Baseline: the same length, doing nothing. Later runs are compared with it.\n"
     "\n"
     "# Sound\n"
     "Mindfield starts no sound. Start a tone or a session first if you want one."},
    {"RNG SOURCE",
     "# The list\n"
     "Only a source that can be used is a button. The others say why they cannot.\n"
     "\n"
     "ESP32 hardware RNG - the chip's own generator. The one to use.\n"
     "Timing jitter - an experiment in itself; its numbers are not independent, so do not use it for results.\n"
     "Software PRNG - arithmetic, not random. A control: it shows what chance looks like.\n"
     "\n"
     "# Below the list\n"
     "The kind of source, its highest rate, and whether its output is physical entropy. The source is written into every experiment."},
    {"EXPERIMENT",
     "# Mode\n"
     "Open: you only watch. Directed: you choose a Target first, more ones or more zeros, and the result is counted towards it.\n"
     "\n"
     "# Condition\n"
     "What you will be doing: Baseline (nothing), Meditation, Control (something else), Sham. It is recorded, not checked.\n"
     "\n"
     "# Rate and Length\n"
     "Samples a second, and how long. Longer runs can show smaller differences.\n"
     "\n"
     "# Picture\n"
     "Tap to go through the twelve.\n"
     "\n"
     "Nothing here can be changed once the experiment has started."},
    {"PICTURES",
     "# Score pictures\n"
     "Orb, Balance, Star, Flame, Pendulum, Vortex show one number: how far the count of ones is from half, towards the target if there is one. The marked middle is what chance expects; the edge is a rare event. Chance alone moves them about the middle all the time.\n"
     "\n"
     "# Pattern pictures\n"
     "Tree, Mandala, Lotus, River, Waterfall, Labyrinth grow from the numbers, one step a second. They show randomness and are not a score.\n"
     "\n"
     "# Under the picture\n"
     "Time, samples, and REC when it is being written to the card. PAUSE holds it, STOP ends it. No score is shown while it runs: it is on the result."},
    {"RESULT",
     "# First page\n"
     "The source, what was tried, the number of samples, then Ones: the share of 1 bits, beside the 0.5 that chance expects, with its 95% range. If 0.5 lies inside the range, nothing was found.\n"
     "z is the same in standard units: within about 2 either way is ordinary. p is how often chance alone gives at least this much. To target counts it towards your target.\n"
     "\n"
     "# MORE\n"
     "Checks on the numbers themselves, the one-second windows, and the comparison with your last Baseline from the same source. That comparison is the one that matters.\n"
     "\n"
     "# Buttons\n"
     "DONE closes the result and stops all sound. REPLAY plays the recorded numbers again through the picture.\n"
     "\n"
     "One result means little. Chance gives scores like these every day."},
    {"HISTORY",
     "# The list\n"
     "Every experiment on the card, newest first: its number, source (HW, SW, EXP), condition and z. Nothing is left out, whatever it showed.\n"
     "\n"
     "# Buttons\n"
     "Tap one to open its result. NEWER and OLDER turn the page.\n"
     "EXPORT writes all of them as a table, SUMMARY.CSV, in the MINDFIELD folder on the card, under EXPORTS.\n"
     "RECOVER makes a result for a run that was cut off, for example by switching off.\n"
     "\n"
     "The raw numbers of every run stay on the card, under EXPERIMENTS in the same folder."},
};

}  // namespace

const char* helpTitle(uint8_t topic) { return topic < kHelpTopics ? kTopics[topic].title : ""; }
const char* helpText(uint8_t topic) { return topic < kHelpTopics ? kTopics[topic].text : ""; }

}  // namespace rc
