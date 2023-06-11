// SPDX-License-Identifier: MIT

// malloc, free (multi-core safe!)
#include "pico/malloc.h"
// memset, memcpy
#include "pico/mem_ops.h"

#include "vendor_request.h"

#include "pico/unique_id.h"
#include "tusb.h"

#include "git.h"

#include "../feature_set.h"

// pico build info: __flash_binary end value from the linker
#include "pico/binary_info.h"
extern char __flash_binary_end;

const req_version version = {
    .bLength = 3,
    .bCode = CUSTOM_REQUEST_VERSION,
    .bVersion = 0x7b // 123
};

const req_unknown_value unknown = {
    .bLength = 2,
    .bCode = CUSTOM_REQUEST_UNKNOWN};

bool handle_custom_vendor_req(uint8_t rhport, uint8_t stage, tusb_control_request_t const *request)
{
    switch (request->wValue)
    {
    case CUSTOM_REQUEST_VERSION:
        return tud_control_xfer(rhport, request, (void *)(uintptr_t)&version, version.bLength);
    case CUSTOM_REQUEST_BUILD_INFO:
    {
        // Add brackets to declare a variable as the first statement in a case (https://stackoverflow.com/a/2036822)
        const char *sha = git_CommitSHA1();
        const char *date = git_CommitDate();

        const req_build_info build_info = {
            .bLength = 67, // bLength (1), bCode (1), sha1 (40), date (25). We could also use strlen(sha) here?
            .bCode = CUSTOM_REQUEST_BUILD_INFO,
            // sha1 and date are set using strcpy.
        };
        // "arrays in C are not assignable, you need to use strcpy." https://stackoverflow.com/a/1292591
        strcpy(build_info.sha1, sha);
        strcpy(build_info.date, date);
        return tud_control_xfer(rhport, request, (void *)(uintptr_t)&build_info, build_info.bLength);
    }
    case CUSTOM_REQUEST_BOARD_ID:
    {
        const pico_unique_board_id_t pico_board_id;
        pico_get_unique_board_id(&pico_board_id); // pass a pointer to the struct

        // Construct a req_build_info struct from the response
        const req_board_id board_id = {
            .bLength = 10, // 1, 1, 8
            .bCode = CUSTOM_REQUEST_BOARD_ID,
            //.bId = &pico_board_id.id,
        };
        memcpy(board_id.bId, pico_board_id.id, 8);

        return tud_control_xfer(rhport, request, (void *)(uintptr_t)&board_id, board_id.bLength);
    }
    case CUSTOM_REQUEST_FEATURE_SET:
    {
        char *features = getFeatureSet();
        const req_feature_set feature_set = {
            .bLength = 3,
            .bCode = CUSTOM_REQUEST_FEATURE_SET,
            .bFeatures = *features,
        };
        free(features);
        return tud_control_xfer(rhport, request, (void *)(uintptr_t)&feature_set, feature_set.bLength);
    }
    case CUSTOM_REQUEST_FLASH_BINARY_END:
    {
        const req_flash_binary_end flash_binary_end = {
            .bLength = 6,
            .bCode = CUSTOM_REQUEST_FLASH_BINARY_END,
            .bEndAddress = (uint32_t)&__flash_binary_end,
        };
        return tud_control_xfer(rhport, request, (void *)(uintptr_t)&flash_binary_end, flash_binary_end.bLength);
    }
    default:
        // Unknown value
        return tud_control_xfer(rhport, request, (void *)(uintptr_t)&unknown, unknown.bLength);
    }
}