// SPDX-License-Identifier: MIT

#ifndef TASKS_MCU_TEMPERATURE_H_
#define TASKS_MCU_TEMPERATURE_H_

void pvRegisterMcuTempTask(void);
void mcu_temp_task(void *param);

// Defined in header file, make it static so it is only defined once!
// https://stackoverflow.com/a/17764724
static struct ADCTempMessage // Len: 8
{
    uint32_t bMagic;   // 4 bytes
    uint8_t bLength;   // 1 byte
    uint8_t bCode;     // 1 byte
    uint16_t temp_adc; // 2 bytes
} temp_message;

#endif /* TASKS_MCU_TEMPERATURE_H_ */