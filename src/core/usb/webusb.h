// SPDX-License-Identifier: MIT

#ifndef USB_WEBUSB_H_
#define USB_WEBUSB_H_

bool tud_vendor_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const *request);

#endif /* USB_WEBUSB_H_ */