// SPDX-License-Identifier: MIT

#ifndef CORE_SEMAPHORES_H_
#define CORE_SEMAPHORES_H_

#include "FreeRTOS.h"
#include "semphr.h"

extern SemaphoreHandle_t xADCMutex;

void pvCreateSemaphores(void);

#endif /* CORE_SEMAPHORES_H_ */