// SPDX-License-Identifier: MIT

// FreeRTOS
#include "FreeRTOS.h"
// FreeRTOS version macro (tskKERNEL_VERSION_NUMBER)
#include "task.h"

#include "semaphores.h"

// Standard libraries
#include "pico/stdio.h"
#include "pico/stdlib.h"

// Accompanying header file
#include "main.h"

// USB FreeRTOS tasks
#include "usb/tasks.h"

#include "tasks/bulk.h"
#include "tasks/mcu_temperature.h"

// Source control information embedded at build-time
#include "git.h"

extern void *__BUILD_INCLUDES_FLASHLOADER;

#ifdef INCLUDES_FLASHLOADER
#include "../flashloader/main.h"
// lib/pico-flashloader (FLASH_APP_UPDATED)
#include "flashloader.h"

// RP2040 Watchdog (flashloader will set a scratch register if it
// has updated something)
#include "hardware/watchdog.h"
#endif

int main(void)
{
    // Setup hardware (init stdio, etc.)
    prvSetupHardware();
#ifdef INCLUDES_FLASHLOADER
    printf("INCLUDES: Flashloader support!\n");

    // Check if we have just upgraded.
    if (watchdog_hw->scratch[0] == FLASH_APP_UPDATED)
    {
        printf("Application has just updated!\n");
        // Reset the scratch register.
        watchdog_hw->scratch[0] = 0;
    }
#endif

    const int has_flashloader = (int)&__BUILD_INCLUDES_FLASHLOADER;

    printf("FLASHLOADER: %i\n", has_flashloader);

    prvSerialDisplaySystemInfo();

    pvCreateBulkQueue();

    // Init some tasks!

    pvCreateUsbTasks();
    pvRegisterMcuTempTask();

    // Create semaphores (mutex locks)
    pvCreateSemaphores();

#ifdef INCLUDES_FLASHLOADER
    pvRegisterFlashloaderTask();
#endif

    // TinyUSB demos call this after creating tasks.
    vTaskStartScheduler();

    return 0;
}

static void prvSetupHardware(void)
{
    stdio_init_all(); // init the things uwu
}

// Display hardware and firmware information over standard output.
void prvSerialDisplaySystemInfo(void)
{
    // Determine which RTOS type we are running
    const char *rtos_name;
#if (portSUPPORT_SMP == 1)
    rtos_name = "FreeRTOS SMP";
#else
    rtos_name = "FreeRTOS";
#endif
    printf("%s, %s\n", rtos_name, tskKERNEL_VERSION_NUMBER);

#if (portSUPPORT_SMP == 1) && (configNUM_CORES == 2)
    printf("Running on both cores\n");
#endif

#if (mainRUN_ON_CORE == 1)
    printf("Kernel running on core 1\n");
#else
    printf("Kernel running on core 0\n");
#endif

    if (git_IsPopulated())
    {
        printf("Commit: %s\n", git_CommitSHA1());
        printf("Commit date: %s\n", git_CommitDate());
    }
    else
    {
        printf("This image was built without source control.\n");
    }
}