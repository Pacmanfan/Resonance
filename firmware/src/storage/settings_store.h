// Device settings in non-volatile storage (NVS), so that the device works
// without an SD card. The record is validated by the portable core every
// time it is loaded (RES-CFG-002).
#pragma once

#include "rc_settings.h"

namespace storage {

enum class LoadResult : uint8_t {
    Ok,        // a stored record was loaded unchanged
    Repaired,  // a stored record was loaded and had invalid fields corrected
    Defaults,  // nothing usable was stored; factory defaults are in use
};

// Loads the settings into `out`. Never fails: `out` is always valid. A record
// of settings version 3 is read once, rewritten in the current form (which
// drops the Wi-Fi credentials it held) and reported as Repaired.
LoadResult load(rc::Settings& out);

// Validates `s` and writes it. Returns false when storage refused the write.
bool save(rc::Settings& s);

// Names of Bluetooth devices that were connected, kept against their
// address so that a device which reconnects by itself can be shown by name.
// Only the name is stored here; the pairing keys stay in the Bluetooth
// stack's own storage (RES-BT-003).
void saveDeviceName(const uint8_t address[6], const char* name);
// Writes the stored name into `out`; returns false, with `out` empty, when
// none is stored.
bool loadDeviceName(const uint8_t address[6], char* out, size_t capacity);

// The number of the next Mindfield experiment: counted here, not on the card,
// so that two cards never give two experiments the same number.
uint32_t takeExperimentId();

// Radio start guard. Starting a radio draws a current surge; if the supply
// cannot deliver it the board loses power and would restart into the same
// surge for ever. markRadioStarting() counts an attempt just before a radio
// is started and markRadioStable() clears the count once it has run for a
// few seconds. radioStartKeepsFailing() is true after kRadioStartAttempts
// attempts in a row that did not survive. One or two are tolerated, because
// a reset from the USB adapter or the reset button during those seconds
// looks exactly like a power loss.
constexpr uint8_t kRadioStartAttempts = 3;
bool radioStartKeepsFailing();
void markRadioStarting();
void markRadioStable();

}  // namespace storage
