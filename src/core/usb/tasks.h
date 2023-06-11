// SPDX-License-Identifier: MIT

#ifndef USB_TASKS_H_
#define USB_TASKS_H_

void pvCreateUsbTasks(void);

void usb_device_task(void *param);
void cdc_task(void *param);

#endif /* USB_TASKS_H_ */