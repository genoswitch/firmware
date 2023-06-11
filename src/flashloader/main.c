// SPDX-License-Identifier: MIT

#include "main.h"

#include "crc32.h"

#include <stdio.h>

void pvRegisterFlashloaderTask(void)
{
    printf("Hello, Flashloader!\n");
    // if you do uint8_t data = .... it will NOT work! You need the asterisk.
    uint8_t *data = (uint8_t[]){
        1, 0, 0, 0, 0, 2, 0, 0, 0, 0, 3, 0, 0, 0, 0, 4};
    uint32_t res = crc32(data, 16);
    printf("FOUND RESULT: 0x%08x\n", res);
}