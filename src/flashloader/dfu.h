// SPDX-License-Identifier: MIT

#ifndef FLASHLOADER_DFU_H_
#define FLASHLOADER_DFU_H_

#include <stdint.h>

uint32_t tud_dfu_get_timeout_cb(uint8_t alt, uint8_t state);

void tud_dfu_download_cb(uint8_t alt, uint16_t block_num, uint8_t const *data, uint16_t length);

void tud_dfu_manifest_cb(uint8_t alt);

#endif /* FLASHLOADER_DFU_H_ */