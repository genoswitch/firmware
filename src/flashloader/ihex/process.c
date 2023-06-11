// SPDX-License-Identifier: BSD-3-Clause (rhulme/pico-flashloader)

// lib/pico-flashloader
#include "flashloader.h"

#include "flash.h"

#include "record.h"

// breakpoints
#include "pico/stdlib.h"

// memset, memcpy
#include "pico/mem_ops.h"

// Current image has around ~67k of binary data. (07.06.2023)
// To futureproof, let's give 96k to store the buffered data.
#define FLASH_BUF_SIZE 65536 + 32768

// Buffer to hold the incoming data before flashing
static union
{
    tFlashHeader header;
    // If this buffer is overfilled, the USB transfer will fail.
    // (Is easily detectable by the host/sending device)
    // "tud_dfu_finish_flashing(DFU_STATUS_OK);" (dfu.c) will never be called,
    // So the transfer will appear to hang until it times out.
    uint8_t buffer[sizeof(tFlashHeader) + FLASH_BUF_SIZE];
} flashbuf;

uint32_t offset = 0;
uint32_t count = 0;

// is passed a pointer to rec
void processRecord(ihexRecord *rec)
{
    //__breakpoint();
    switch (rec->type)
    {
    case UHEX_TYPE_CUSTOM_DATA:
        // Check if the active sectionId is the app section id.
        if (rec->sectionId == SECTION_ID_APP)
        {
            // rhulme's implementation
            // seems sketchy, seems to copy into the buffer underneath it?
            // On the other hand, I have very little C/C++ experience
            memcpy(&flashbuf.header.data[offset], rec->data, rec->count);
            offset += rec->count;
            offset %= FLASH_BUF_SIZE;
            // if ((offset % 1024) == 0)
            //{
            //     printf("Recieved a 1k block of firmware via DFU\n");
            // }
        }
        // Section ID does not match, we can ignore this.
        // eg. flashloader code at the start of the file.
        break;
    case IHEX_TYPE_EOF:
        printf("FOUND EOF\n");
        // TODO: This should be called from dfu.c/tud_dfu_manifest_cb() instead.
        flashImage(&flashbuf.header, offset);
        break;
    // skip
    case IHEX_TYPE_EXT_LIN_ADDR:
        printf("EXT LINE\n");
        printf("NEW ADDRESS: 0x%08x\n", rec->ulba);
        break;
    }
}