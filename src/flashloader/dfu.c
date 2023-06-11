// SPDX-License-Identifier: MIT

#include "tusb.h"

#include "ihex/record.h"
#include "ihex/process.h"

#include "ringbuf.h"

#include <string.h>

// DFU Callbacks
// Note that 'alt' is used as the partition number to allow for multiple partitions (FLASH/EEPROM/etc)
// At the moment we only have one "partition".
// Invoked right before tud_dfu_download_cb() (state=DFU_DNBUSY) or tud_dfu_manifest_cb() (state=DFU_MANIFEST)
// Application return timeout in milliseconds (bwPollTimeout) for the next download/manifest operation.
// During this period, USB host won't try to communicate with us.

uint32_t tud_dfu_get_timeout_cb(uint8_t alt, uint8_t state)
{
    if (state == DFU_DNBUSY)
    {
        // For this example
        // - Atl0 Flash is fast : 1   ms
        // - Alt1 EEPROM is slow: 100 ms
        return (alt == 0) ? 1 : 100;
    }
    else if (state == DFU_MANIFEST)
    {
        // since we don't buffer entire image and do any flashing in manifest stage
        return 0;
    }

    return 0;
}

ringbuf_t rb1;

// Store last ulba between blocks
uint32_t ulba = 0;

// Store last sectionId between blocks.
uint16_t sectionId;

void findMatches(int *startLineMatch, int *endLineMatch)
{
    *startLineMatch = ringbuf_findchr(rb1, ':', 0);
    *endLineMatch = ringbuf_findchr(rb1, '\n', 0);
    if (*endLineMatch < *startLineMatch && *startLineMatch != ringbuf_bytes_used(rb1))
    {
        *endLineMatch = ringbuf_findchr(rb1, '\n', 1);
    }
}

// Called when we recieve a new block from the host.
// (recieved DFU_DNLOAD (wLength>0) following by DFU_GETSTATUS (state=DFU_DNBUSY) requests)
// This callback could be returned before flashing op is complete (async).
// Once finished flashing, application must call tud_dfu_finish_flashing()
void tud_dfu_download_cb(uint8_t alt, uint16_t block_num, uint8_t const *data, uint16_t length)
{
    (void)alt;
    (void)block_num;

    if (block_num == 0)
    {
        printf("Initialized new ringbuffer...\n");
        rb1 = ringbuf_new(256 + 5);
    }

    ringbuf_memcpy_into(rb1, data, length);

    int startLineMatch;
    int endLineMatch;

    findMatches(&startLineMatch, &endLineMatch);

    while (endLineMatch < ringbuf_bytes_used(rb1))
    {

        // +1 to include the match itself.
        endLineMatch++;

        char *buf = malloc(endLineMatch);
        ringbuf_memcpy_from(buf, rb1, endLineMatch);
        // printf("CONTENTS: '%.*s'\n", endLineMatch, buf);

        ihexRecord rec;
        // Use memset to zero out the memory occupied by this struct.
        // aka. set all values to 0.
        memset(&rec, 0, sizeof(rec));

        // Set ulba to previous value, or zero.
        // If this record is an Extended Linear Address record, parseRecord will overwrite the value.
        rec.ulba = ulba;

        // Set sectionId to previous value, or zer.
        // If this record is a Universal Hex Block Start record, parseRecord will overwrite the value.
        rec.sectionId = sectionId;

        int result = parseRecord(buf, &rec);
        if (result == 0)
        {
            __breakpoint();
        }
        processRecord(&rec);

        // If the ulba is different to what is saved, update the variable.
        if (rec.ulba != ulba)
            ulba = rec.ulba;

        // If the sectionId is different to what is saved, update thhe variable
        if (rec.sectionId != sectionId)
            sectionId = rec.sectionId;

        // printf("processed record with length %i\n", rec.count);

        free(buf);

        // prep for next iteration
        findMatches(&startLineMatch, &endLineMatch);
    }
    // printf("\nProcessed DFU packet: alt %u, blockNum %u, length %u\n", alt, block_num, length);

    // flashing op for download complete without error
    tud_dfu_finish_flashing(DFU_STATUS_OK);
}

// Called when the download process is complete (new firmware has been sent to the device)
// (received DFU_DNLOAD (wLength=0) following by DFU_GETSTATUS (state=Manifest))
// Here we should do checksumming, or flashing if we previously buffered the image.
// Once we have finished flashing, we must call tud_dfu_finish_flashing()
void tud_dfu_manifest_cb(uint8_t alt)
{
    (void)alt; // Ignore paramater
    printf("Download finished!\n");

    // flashing op for manifest is complete without error
    // Application can perform checksum, should it fail, use appropriate status such as errVERIFY.
    tud_dfu_finish_flashing(DFU_STATUS_OK);
}
