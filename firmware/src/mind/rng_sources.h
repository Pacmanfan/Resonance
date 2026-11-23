// The random-number sources of this board (RES-MIND-001). Each is an
// rc::RngSource; what each one is, and is not, is said in its status line and
// in docs/MINDFIELD.md.
#pragma once

#include <stdint.h>

#include "rc_rng.h"

namespace rngsources {

constexpr uint8_t kCount = 6;

// Sources in the order they are listed: hardware generator, internal ADC,
// ADC noise input, timing jitter, external, software control.
rc::RngSource& at(uint8_t index);
// Index of the source with this id; kCount when there is none.
uint8_t indexOf(rc::RngId id);
// The software source, to set its seed.
rc::SoftwareRng& software();

}  // namespace rngsources
