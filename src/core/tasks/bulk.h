// SPDX-License-Identifier: MIT

#ifndef TASKS_BULK_H_
#define TASKS_BULK_H_

// FreeRTOS core and queue libraries for QueueHandle_t typedef
#include "FreeRTOS.h"
#include "queue.h"

void pvCreateBulkQueue(void);
void bulk_queue_task(void *param);
QueueHandle_t getQueueHandle(void);

#define MAGIC_BYTE 0xdd24433b // Bear in mind this is little endian so will show up backwards!

// Defined in header file, make it static so it is only defined once!
// https://stackoverflow.com/a/17764724
static struct BaseMessage
{
    uint32_t bMagic;
    uint8_t bLength;
    uint8_t bCode;
} base_message;

#endif /* TASKS_BULK_H_ */