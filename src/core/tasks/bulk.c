// SPDX-License-Identifier: MIT

// FreeRTOS core, task and queue libraries
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

// TinyUSB
#include "tusb.h"

// Pico stdio and standard lib
#include "pico/stdio.h"
#include "pico/stdlib.h"

// Accompanying header file
#include "bulk.h"

/* Declare a variable of type QueueHandle_t to hold the handle of the queue being created. */
QueueHandle_t xPointerQueue;

#define MCU_BULK_QUEUE_STACK_SIZE configMINIMAL_STACK_SIZE

#if configSUPPORT_STATIC_ALLOCATION
StackType_t mcu_temp_stack[MCU_BULK_QUEUE_STACK_SIZE];
StaticTask_t mcu_temp_taskdef;
#endif

void pvRegisterBulkQueueTask(void)
{
#if configSUPPORT_STATIC_ALLOCATION
    xTaskCreateStatic(bulk_queue_task, "bulkqd", MCU_BULK_QUEUE_STACK_SIZE, NULL, configMAX_PRIORITIES, mcu_temp_stack, &mcu_temp_taskdef)
#else
    xTaskCreate(bulk_queue_task, "bulkqd", MCU_BULK_QUEUE_STACK_SIZE, NULL, configMAX_PRIORITIES, NULL);
#endif
}

QueueHandle_t getQueueHandle(void)
{
    return xPointerQueue;
}

void pvCreateBulkQueue(void)
{

    /* Create a queue that can hold a maximum of 5 pointers, in this case character pointers. */
    xPointerQueue = xQueueCreate(
        // The number of items the queue can hold
        10,
        // Size of each item only big enough for a pointer
        sizeof(&base_message));

    if (xPointerQueue == NULL) // 0 == NULL
    {
        // Queue not created
        printf("Queue not created.");
    }
    else
    {
        // Queue created successfully
        pvRegisterBulkQueueTask();
    }
}

void bulk_queue_task(void *param)
{
    (void)param; // prevent unused var warning

    // Create a char to store the pointer from the buffer object in the queue
    struct BaseMessage xRecievedStruct, *pxRecievedPointer;

    while (1)
    {
        // This task will be SUSPENDED until data is available
        if (xQueueReceive(xPointerQueue, &(pxRecievedPointer), portMAX_DELAY) == pdTRUE)
        {
            // We have data and have recieved it successfully
            tud_vendor_write(pxRecievedPointer, pxRecievedPointer->bLength);
            tud_vendor_flush();
        }
        else
        {
            // Error
        }
    }
}