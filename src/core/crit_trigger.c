// FreeRTOS core and task libraries
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "crit_trigger.h"

#define CRIT_TRIGGER_STACK_SIZE configMINIMAL_STACK_SIZE

// Define static tasks
#if configSUPPORT_STATIC_ALLOCATION
StackType_t crit_trigger_stack[USBD_STACK_SIZE];
StaticTask_t crit_trigger_taskdef;
#endif
int core = 1;

#if configNUM_CORES > 1

/* Declare a variable of type QueueHandle_t to hold the handle of the queue being created. */
QueueHandle_t xMessageQueue;

bool shouldSpinlock = false;

void __not_in_flash_func(spinlock)()
{
    printf("spinlock\n");
    while (shouldSpinlock)
    {
    }
    printf("exit spinlock\n");
    vTaskDelete(NULL);
}

void crit_trigger_device_task(void *param)
{
    // FreeRTOS supports passing paramaters to tasks, but we don't need any
    // To avoid warnings about mismatched arguments, let's just take the argument and leave it.
    (void)param;

    bool shouldEnterCritical;

    while (1)
    {
        // This task will suspend (yield) until data is available
        if (xQueueReceive(xMessageQueue, &(shouldEnterCritical), portMAX_DELAY) == pdTRUE)
        {
            // Have successfully recieved data.
            printf("RECIEVED DATA: %i\n", shouldEnterCritical);

            if (shouldEnterCritical)
            {
                // taskENTER_CRITICAL();
                shouldSpinlock = true;
                printf("crit/core 1: Entered RTOS Critical Section\n");
                // spinlock();
                xTaskCreateAffinitySet(spinlock, "spinlock", CRIT_TRIGGER_STACK_SIZE, NULL, configMAX_PRIORITIES - 1, 1 << 1, NULL);
            }
            else
            {
                shouldSpinlock = false;
                printf("crit/core 1: Exited RTOS Critcal Section\n");
            }
        }
    }
}

void prvCreateTasks(void)
{
    // Register crit trigger task
#if configSUPPORT_STATIC_ALLOCATION

    // Create a STATIC task for the critical trigger
    xTaskCreateStaticAffinitySet(crit_trigger_device_task, "crit_trigger", CRIT_TRIGGER_STACK_SIZE, NULL, configMAX_PRIORITIES, crit_trigger_stack, &crit_trigger_device_taskdef, 1 << 1);

#else /* configSUPPORT_STATIC_ALLOCATION */

    // Create tasks using dynamic memory allocation
    xTaskCreateAffinitySet(crit_trigger_device_task, "crit_trigger", CRIT_TRIGGER_STACK_SIZE, NULL, configMAX_PRIORITIES, 1 << 1, NULL);

#endif /* configSUPPORT_STATIC_ALLOCATION */
}
#endif /* else: configNUM_CORES > 1 */

void pvSetupCritTrigger(void)
{
#if configNUM_CORES > 1
    printf("Setting up crit trigger\n");
    xMessageQueue = xQueueCreate(5, sizeof(bool));
    if (xMessageQueue == NULL)
    {
        printf("Error creating crit trigger queue.\n");
    }
    else
    {
        printf("Crit trigger queue created, registering tasks...\n");
        prvCreateTasks();
    }
#else /* configNUM_CORES > 1 */
    printf("Only running on 1 core. Crit Trigger not required.\n");
#endif
}