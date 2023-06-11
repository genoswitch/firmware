// SPDX-License-Identifier: MIT

// FreeRTOS core and task libraries
#include "FreeRTOS.h"
#include "task.h"

#include "spinlock.h"

#define SPINLOCK_STACK_SIZE configMINIMAL_STACK_SIZE

// Define static tasks
#if configSUPPORT_STATIC_ALLOCATION
StackType_t spinlock_stack[USBD_STACK_SIZE];
StaticTask_t spinlock_trigger_taskdef;
#endif

TaskHandle_t xHandle;

// Spinlock function (eat up CPU time, an infinite loop)
void __not_in_flash_func(spinlock)()
{
    while (true)
    {
    }
}

// Create the spinlock task
void startSpinlock()
{
#if configSUPPORT_STATIC_ALLOCATION

    // Create a STATIC task for the critical trigger
    xHandle = xTaskCreateStaticAffinitySet(spinlock, "spinlock", SPINLOCK_STACK_SIZE, NULL, configMAX_PRIORITIES, spinlock_stack, &spinlock_trigger_taskdef, 1 << 1);

#else /* configSUPPORT_STATIC_ALLOCATION */

    // Create tasks using dynamic memory allocation
    xTaskCreateAffinitySet(spinlock, "spinlock", SPINLOCK_STACK_SIZE, NULL, configMAX_PRIORITIES, 1 << 1, &xHandle);
#endif
}

void stopSpinlock()
{
    if (xHandle != NULL)
    {
        vTaskDelete(xHandle);
    }
    // xHandle should now be null.
}