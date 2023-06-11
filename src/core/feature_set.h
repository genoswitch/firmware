// SPDX-License-Identifier: MIT

// Macros for a bitset in C.
// https://stackoverflow.com/a/21001661
// -> http://c-faq.com/misc/bitsets.html

#ifndef CORE_FEATURE_SET_H_
#define CORE_FEATURE_SET_H_

#include <limits.h> /* for CHAR_BIT */

#define BITMASK(b) (1 << ((b) % CHAR_BIT))
#define BITSLOT(b) ((b) / CHAR_BIT)
#define BITSET(a, b) ((a)[BITSLOT(b)] |= BITMASK(b))
#define BITCLEAR(a, b) ((a)[BITSLOT(b)] &= ~BITMASK(b))
#define BITTEST(a, b) ((a)[BITSLOT(b)] & BITMASK(b))
#define BITNSLOTS(nb) ((nb + CHAR_BIT - 1) / CHAR_BIT)

char *getFeatureSet();

#endif /* CORE_FEATURE_SET_H_ */