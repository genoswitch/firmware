// SPDX-License-Identifier: MIT

// malloc
#include "pico/malloc.h"

// memset
#include "pico/mem_ops.h"

#include "feature_set.h"

extern void *__BUILD_INCLUDES_FLASHLOADER;

// The 'features' is a uint8 (currently) where each bit represents a feature.
// Currently the features are:
// Index 0 - Flashloader availability.
// Remember to call free()! Or memory leak.
char *getFeatureSet()
{
    // Use malloc to allocate an address for the bitarray
    // <https://stackoverflow.com/questions/12380758/error-function-returns-address-of-local-variable>
    // (Otherwise it's a local variable which will not persist outside of this call)
    char *bitarray = malloc(sizeof(char));
    // Use the macro to declare a bitarray of 8 bits (1 byte)
    bitarray[BITNSLOTS(8)];
    // Set all bits to 0
    memset(bitarray, 0, BITNSLOTS(8));

    // Set first bit if flashloader is available
    if ((int)&__BUILD_INCLUDES_FLASHLOADER == 1)
    {
        BITSET(bitarray, 0);
    }

    // Return the bitarray.
    return bitarray;
}