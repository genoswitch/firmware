// SPDX-License-Identifier: BSD-3-Clause (rhulme/pico-flashloader)

#ifndef IHEX_PROCESS_H_
#define IHEX_PROCESS_H_

// iHexRecord
#include "record.h"

// Offset within flash of the new app image to be flashed by the flashloader
static const uint32_t FLASH_IMAGE_OFFSET = 128 * 1024;

void processRecord(ihexRecord *record);

#endif /* IHEX_PROCESS_H_ */