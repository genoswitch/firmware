#ifndef CORE_CRIT_TRIGGER_H_
#define CORE_CRIT_TRIGGER_H_

// FreeRTOS queue header (QueueHandle_t)
#include "queue.h"

void pvSetupCritTrigger(void);

extern QueueHandle_t xMessageQueue;

#endif /* CORE_CRIT_TRIGGER_H_ */
