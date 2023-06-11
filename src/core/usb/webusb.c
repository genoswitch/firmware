// SPDX-License-Identifier: MIT

#include "tusb.h"

#include "vendor_request.h"

// include desc_ms_os_20 variable, VENDOR_REQUEST_WEBUSB, VENDOR_REQUEST_MICROSOFT constants
#include "usb_descriptors.h"

#define URL "example.tinyusb.org/webusb-serial/index.html?hi=true"

// The definitions in this file are required for the device to properly enumerate as a WinUSB/WebUSB device under Windows.
// These definitions are features in the TinyUSB example code.

// https://wicg.github.io/webusb/#url-descriptor

const tusb_desc_webusb_url_t desc_url =
    {
        .bLength = 3 + sizeof(URL) - 1,
        // Types:
        // 0-2: (Reserved)
        // 3: WEBUSB_URL
        .bDescriptorType = 3,
        // Schemes:
        // 0: 'http://'
        // 1: 'https://'
        // 2: '' (entire URL inc scheme is in the URL field)
        .bScheme = 1,
        .url = URL};

// Original source: TinyUSB WebUSB example (main.c)
// WebUSB use vendor class
// Invoked when a control transfer occurred on an interface of this class
// Driver response accordingly to the request and the transfer stage (setup/data/ack)
// return false to stall control endpoint (e.g unsupported request)
bool tud_vendor_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const *request)
{
    // nothing to with DATA & ACK stage
    if (stage != CONTROL_STAGE_SETUP)
        return true;

    switch (request->bmRequestType_bit.type)
    {
    case TUSB_REQ_TYPE_VENDOR:
        switch (request->bRequest)
        {
        case VENDOR_REQUEST_WEBUSB:
            // match vendor request in BOS descriptor
            // Get landing page url
            return tud_control_xfer(rhport, request, (void *)(uintptr_t)&desc_url, desc_url.bLength);

        case VENDOR_REQUEST_MICROSOFT:
            if (request->wIndex == 7)
            {
                // Get Microsoft OS 2.0 compatible descriptor
                uint16_t total_len;
                memcpy(&total_len, desc_ms_os_20 + 8, 2);

                return tud_control_xfer(rhport, request, (void *)(uintptr_t)desc_ms_os_20, total_len);
            }
            else
            {
                return false;
            }

        case VENDOR_REQUEST_CUSTOM:
            if (request->wIndex == 3) // Vendor specific endpoint (Available to claim using WebUSB)
            {
                return handle_custom_vendor_req(rhport, stage, request);
            }
            else
            {
                return false;
            }

        default:
            break;
        }
        break;

    case TUSB_REQ_TYPE_CLASS:
        if (request->bRequest == 0x22)
        {

            // response with status OK
            return tud_control_status(rhport, request);
        }
        break;

    default:
        break;
    }

    // stall unknown request
    return false;
}
