// SPDX-License-Identifier: MIT

// FreeRTOS core and task libraries
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "../semaphores.h"

#include "pico/stdio.h"
#include "pico/stdlib.h"
#include "hardware/adc.h"

#include "mcu_temperature.h"

#include "bulk.h"

#include "../config.h"

#define MCU_TEMP_STACK_SIZE configMINIMAL_STACK_SIZE

#if configSUPPORT_STATIC_ALLOCATION
StackType_t mcu_temp_stack[MCU_TEMP_STACK_SIZE];
StaticTask_t mcu_temp_taskdef;
#endif

void pvRegisterMcuTempTask(void)
{
#if configSUPPORT_STATIC_ALLOCATION
    xTaskCreateStatic(mcu_temp_task, "tempm", MCU_TEMP_STACK_SIZE, NULL, configMAX_PRIORITIES, mcu_temp_stack, &mcu_temp_taskdef)
#else
    xTaskCreate(mcu_temp_task, "tempp", MCU_TEMP_STACK_SIZE, NULL, configMAX_PRIORITIES, NULL);
#endif
}

#if configLOG_TEMP_STDOUT
const float conversion_factor = 3.3f / (1 << 12);
#endif

void mcu_temp_task(void *param)
{
    struct ADCTempMessage *pxPointerToxMessage;
    (void)param;

    adc_init();
    adc_set_temp_sensor_enabled(true);

    /* Block for 500ms. */
    const TickType_t xDelay = 500 / portTICK_PERIOD_MS;

    vTaskDelay(xDelay);

    while (1)
    {

        // Take the mutex, which signals the ADC as unavailable
        // If the mutex is in use, this will **block** until it is available again.
        xSemaphoreTake(xADCMutex, portMAX_DELAY);

        // Switch to the temperature input if needed
        // As other tasks could have selected different inputs!
        if (adc_get_selected_input() != 4)
        {

            adc_select_input(4);
        }

        // Get a reading from the ADC.
        uint16_t raw = adc_read();

#if configLOG_TEMP_STDOUT
        // A float has more than enough resolution for this.
        // Convert the adc value to a voltage
        float result = raw * conversion_factor;
        // Determine the temperature using a known value at a known temperature
        // Src: RP2040 Datasheet, Section 4.9.5 'Temperature Sensor'
        float temp = 27 - (result - 0.706) / 0.001721;
        // Display the temperature over stdout
        printf("MCU Temp: %f C\n", temp);
#endif

        // Construct a ADCTempMessage.
        struct ADCTempMessage msg;
        // Insert the magic byte informing the reciever that this is a valid packet.
        msg.bMagic = MAGIC_BYTE;
        // Set the length (used by usbd - sends bLength bytes)
        msg.bLength = 8;
        // Set the packet code.
        msg.bCode = 0x00;
        // Set the temp value
        msg.temp_adc = raw;
        // Create a pointer to the message
        pxPointerToxMessage = &msg;
        // Get the queue handle
        QueueHandle_t queue = getQueueHandle();
        // Add a pointer to the memory address of the message to the queue.
        // This allows us to add arbitary length datatypes to the queue.
        // Use a tickType of 0 to not block until a space if available.
        // TODO: Add handling for when the queue is full, although may be unlikely to happen.
        xQueueSend(queue, (void *)&pxPointerToxMessage, (TickType_t)0);

        // Hand back the mutex to signal the ADC's availability to other tasks.
        xSemaphoreGive(xADCMutex);

        // Delay (suspend) this task. Does not use CPU time.
        vTaskDelay(xDelay);
    }
}