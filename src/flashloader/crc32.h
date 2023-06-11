// SPDX-License-Identifier: MIT

#ifndef FLASHLOADER_CRC32_H_
#define FLASHLOADER_CRC32_H_

#include <stdint.h>

uint32_t crc32(void *ptr, uint32_t len);

uint32_t crc32_no_reflection_or_final_xor(void *ptr, uint32_t len);

#endif /* FLASHLOADER_CRC32_H_ */