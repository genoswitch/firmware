// SPDX-License-Identifier: MIT

// FreeRTOS core and task libraries
#include "FreeRTOS.h"
#include "task.h"

// TinyUSB
#include "tusb.h"

// Accompanying header file
#include "tasks.h"

// WebUSB hooks
#include "webusb.h"

#ifdef INCLUDES_FLASHLOADER
#include "../../flashloader/dfu.h"
#endif

// Increase stack size when debug log is enabled
#define USBD_STACK_SIZE (3 * configMINIMAL_STACK_SIZE / 2) * (CFG_TUSB_DEBUG ? 2 : 1)
#define CDC_STACK_SZIE configMINIMAL_STACK_SIZE

// Define static tasks
#if configSUPPORT_STATIC_ALLOCATION
StackType_t usb_device_stack[USBD_STACK_SIZE];
StaticTask_t usb_device_taskdef;

StackType_t cdc_stack[CDC_STACK_SZIE];
StaticTask_t cdc_taskdef;
#endif

// remember to start the scheduler using vTaskStartScheduler(); after calling this!

// Create USB tasks.
// Tie both tasks to core 0.
// usbd and cdc must run on the same core or the cdc task will panic.
void pvCreateUsbTasks(void)
{
#if configSUPPORT_STATIC_ALLOCATION
    // Create a task for tinyusb device stack
    xTaskCreateStaticAffinitySet(usb_device_task, "usbd", USBD_STACK_SIZE, NULL, configMAX_PRIORITIES - 1, usb_device_stack, &usb_device_taskdef, 1 << 0);

    // Create CDC task
    xTaskCreateStaticAffinitySet(cdc_task, "cdc", CDC_STACK_SZIE, NULL, configMAX_PRIORITIES - 2, cdc_stack, &cdc_taskdef, 1 << 0);
#else
    // Create tasks using dynamic memory allocation
    xTaskCreateAffinitySet(usb_device_task, "usbd", USBD_STACK_SIZE, NULL, configMAX_PRIORITIES - 1, 1 << 0, NULL);
    xTaskCreateAffinitySet(cdc_task, "cdc", CDC_STACK_SZIE, NULL, configMAX_PRIORITIES - 2, 1 << 0, NULL);
#endif
}

// USB Device Driver task
// This top level thread process all usb events and invoke callbacks
void usb_device_task(void *param)
{
    // FreeRTOS supports passing paramaters to tasks, but we don't need any
    // To avoid warnings about mismatched arguments, let's just take the argument and leave it.
    (void)param;

    // init device stack on configured roothub port
    // This should be called after scheduler/kernel is started.
    // Otherwise it could cause kernel issue since USB IRQ handler does use RTOS queue API.
    tud_init(BOARD_TUD_RHPORT);

    // RTOS infinite loop
    while (1)
    {
        // Put this thread in a waiting state until there is new events.
        // Hopefully this means it shoudn't eat cpu time when nothing is happening!
        tud_task();

        // If the task processes at least one event, CDC write flush!
        // following code only run if tud_task() process at least 1 event
        tud_cdc_write_flush();
    }
}

void cdc_task(void *param)
{
    // FreeRTOS supports passing paramaters to tasks, but we don't need any
    // To avoid warnings about mismatched arguments, let's just take the argument and leave it.
    (void)param;

    if (tud_cdc_connected())
    {
        // connected and there are data available
        if (tud_cdc_available())
        {
            uint8_t buf[64];

            uint32_t count = tud_cdc_read(buf, sizeof(buf));

            // echo back to both web serial and cdc
            // echo_all(buf, count);
        }
    }
}