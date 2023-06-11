// SPDX-License-Identifier: BSD-3-Clause (rhulme/pico-flashloader)

#ifndef IHEX_RECORD_H_
#define IHEX_RECORD_H_

#include <stdint.h>

enum
{
    IHEX_TYPE_DATA,
    IHEX_TYPE_EOF,
    IHEX_TYPE_END_SEG_ADDR,
    IHEX_TYPE_START_SEG_ADDR,
    IHEX_TYPE_EXT_LIN_ADDR,
    IHEX_TYPE_START_LIN_ADDR,
    // BBC micro:bit Universal Hex record types.
    // https://tech.microbit.org/software/spec-universal-hex/#universal-hex-new-record-types
    UHEX_TYPE_BLOCK_START = 0x0A,
    UHEX_TYPE_BLOCK_END = 0x0B,
    UHEX_TYPE_PADDED_DATA = 0x0C,
    UHEX_TYPE_CUSTOM_DATA = 0x0D,
    UHEX_TYPE_OTHER_DATA = 0x0E
};

enum
{
    SECTION_ID_APP = 0xffa1,
    SECTION_ID_FLASHLOADER = 0xffa0
};

typedef struct
{
    uint8_t count;
    uint16_t addr;
    uint8_t type;
    uint32_t ulba;      // TODO: Is this uint32 or uint16?
    uint16_t sectionId; // Universal Hex Board ID. We are using it as a section ID.
    uint8_t data[256];  // rhulme (max size 256bytes)
} ihexRecord;

int parseRecord(const char *line, ihexRecord *record);

#endif /* IHEX_RECORD_H_ */