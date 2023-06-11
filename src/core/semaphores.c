// SPDX-License-Identifier: MIT

#include "semaphores.h"

SemaphoreHandle_t xADCMutex;

void pvCreateSemaphores(void)
{
    xADCMutex = xSemaphoreCreateMutex();
}