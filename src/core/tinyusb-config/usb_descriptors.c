// SPDX-License-Identifier: MIT (TinyUSB)

// Src: https://github.com/hathach/tinyusb/blob/ea098aeda1c2b9fe467041be325e55049e304ee4/examples/device/webusb_serial/src/usb_descriptors.c

/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Ha Thach (tinyusb.org)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include "tusb.h"
#include "usb_descriptors.h"

#ifdef INCLUDES_FLASHLOADER
#include "class/dfu/dfu_device.h"
#endif

/* A combination of interfaces must have a unique product id, since PC will save device driver after the first plug.
 * Same VID/PID with different interface e.g MSC (first), then CDC (later) will possibly cause system error on PC.
 *
 * Auto ProductID layout's Bitmap:
 *   [MSB]       MIDI | HID | MSC | CDC          [LSB]
 */
#define _PID_MAP(itf, n) ((CFG_TUD_##itf) << (n))
#define USB_PID (0x4000 | _PID_MAP(CDC, 0) | _PID_MAP(MSC, 1) | _PID_MAP(HID, 2) | \
                 _PID_MAP(MIDI, 3) | _PID_MAP(VENDOR, 4))

//--------------------------------------------------------------------+
// Device Descriptors
//--------------------------------------------------------------------+
tusb_desc_device_t const desc_device =
    {
        .bLength = sizeof(tusb_desc_device_t),
        .bDescriptorType = TUSB_DESC_DEVICE,
        .bcdUSB = 0x0210, // at least 2.1 or 3.x for BOS & webUSB

        // Use Interface Association Descriptor (IAD) for CDC
        // As required by USB Specs IAD's subclass must be common class (2) and protocol must be IAD (1)
        .bDeviceClass = TUSB_CLASS_MISC,
        .bDeviceSubClass = MISC_SUBCLASS_COMMON,
        .bDeviceProtocol = MISC_PROTOCOL_IAD,
        .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,

        .idVendor = 0xCafe,
        .idProduct = USB_PID,
        .bcdDevice = 0x0100,

        .iManufacturer = 0x01,
        .iProduct = 0x02,
        .iSerialNumber = 0x03,

        .bNumConfigurations = 0x01};

// Invoked when received GET DEVICE DESCRIPTOR
// Application return pointer to descriptor
uint8_t const *tud_descriptor_device_cb(void)
{
    return (uint8_t const *)&desc_device;
}

//--------------------------------------------------------------------+
// Configuration Descriptor
//--------------------------------------------------------------------+
enum
{
    ITF_NUM_CDC = 0,
    ITF_NUM_CDC_DATA,
    ITF_NUM_VENDOR,
#ifdef INCLUDES_FLASHLOADER
    ITF_NUM_DFU_MODE,
#endif
    ITF_NUM_TOTAL
};

#ifndef INCLUDES_FLASHLOADER
#define CONFIG_TOTAL_LEN (TUD_CONFIG_DESC_LEN + TUD_CDC_DESC_LEN + TUD_VENDOR_DESC_LEN)
#else
// Flashloader included, include DFU descriptor length, define alt_count and FUNC_ATTRS

// DFU alternate interface count - one for each 'partition'
#define ALT_COUNT 1

// DFU supported 'functions'
// TODO: Upload will likely not be used (it's device -> host)
#define FUNC_ATTRS (DFU_ATTR_CAN_UPLOAD | DFU_ATTR_CAN_DOWNLOAD | DFU_ATTR_MANIFESTATION_TOLERANT)

#define CONFIG_TOTAL_LEN (TUD_CONFIG_DESC_LEN + TUD_CDC_DESC_LEN + TUD_VENDOR_DESC_LEN + TUD_DFU_DESC_LEN(ALT_COUNT))
#endif

#if CFG_TUSB_MCU == OPT_MCU_LPC175X_6X || CFG_TUSB_MCU == OPT_MCU_LPC177X_8X || CFG_TUSB_MCU == OPT_MCU_LPC40XX
// LPC 17xx and 40xx endpoint type (bulk/interrupt/iso) are fixed by its number
// 0 control, 1 In, 2 Bulk, 3 Iso, 4 In etc ...
#define EPNUM_CDC_IN 2
#define EPNUM_CDC_OUT 2
#define EPNUM_VENDOR_IN 5
#define EPNUM_VENDOR_OUT 5
#elif CFG_TUSB_MCU == OPT_MCU_SAMG || CFG_TUSB_MCU == OPT_MCU_SAMX7X
// SAMG & SAME70 don't support a same endpoint number with different direction IN and OUT
//    e.g EP1 OUT & EP1 IN cannot exist together
#define EPNUM_CDC_IN 2
#define EPNUM_CDC_OUT 3
#define EPNUM_VENDOR_IN 4
#define EPNUM_VENDOR_OUT 5
#else
#define EPNUM_CDC_IN 2
#define EPNUM_CDC_OUT 2
#define EPNUM_VENDOR_IN 3
#define EPNUM_VENDOR_OUT 3
#endif

uint8_t const desc_configuration[] =
    {
        // Config number, interface count, string index, total length, attribute, power in mA
        TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN, 0x00, 100),

        // Interface number, string index, EP notification address and size, EP data address (out, in) and size.
        TUD_CDC_DESCRIPTOR(ITF_NUM_CDC, 4, 0x81, 8, EPNUM_CDC_OUT, 0x80 | EPNUM_CDC_IN, TUD_OPT_HIGH_SPEED ? 512 : 64),

        // Interface number, string index, EP Out & IN address, EP size
        TUD_VENDOR_DESCRIPTOR(ITF_NUM_VENDOR, 5, EPNUM_VENDOR_OUT, 0x80 | EPNUM_VENDOR_IN, TUD_OPT_HIGH_SPEED ? 512 : 64),

// NOTE: If this is placed before TUD_VENDOR_DESCRIPTOR tinyusb panics.
// It displays the warning: "WARN: starting new transfer on already active ep 0 out"
// Before panicking with the note: "ep 0 out was already available"
// It may be because the iface number is higher? :shrug:
// The following issues/discussions seem to have encountered this but no fix has been identified.
// https://github.com/hathach/tinyusb/issues/1741
// (converted to discussion): https://github.com/hathach/tinyusb/discussions/1764
// https://github.com/adafruit/circuitpython/issues/6983
// https://github.com/hathach/tinyusb/discussions/962
// https://github.com/raspberrypi/tinyusb/issues/9
#ifdef INCLUDES_FLASHLOADER
        // Interface number, Alternate count, starting string index, attributes, detach timeout, transfer size
        TUD_DFU_DESCRIPTOR(ITF_NUM_DFU_MODE, ALT_COUNT, 6, FUNC_ATTRS, 1000, CFG_TUD_DFU_XFER_BUFSIZE),
#endif
};

// Invoked when received GET CONFIGURATION DESCRIPTOR
// Application return pointer to descriptor
// Descriptor contents must exist long enough for transfer to complete
uint8_t const *tud_descriptor_configuration_cb(uint8_t index)
{
    (void)index; // for multiple configurations
    return desc_configuration;
}

//--------------------------------------------------------------------+
// BOS Descriptor
//--------------------------------------------------------------------+

/* Microsoft OS 2.0 registry property descriptor
Per MS requirements https://msdn.microsoft.com/en-us/library/windows/hardware/hh450799(v=vs.85).aspx
device should create DeviceInterfaceGUIDs. It can be done by driver and
in case of real PnP solution device should expose MS "Microsoft OS 2.0
registry property descriptor". Such descriptor can insert any record
into Windows registry per device/configuration/interface. In our case it
will insert "DeviceInterfaceGUIDs" multistring property.

GUID is freshly generated and should be OK to use.

https://developers.google.com/web/fundamentals/native-hardware/build-for-webusb/
(Section Microsoft OS compatibility descriptors)
*/

//--------------------------------------------------------------------+
// BOS Descriptor: Size breakdown
//--------------------------------------------------------------------+

// A descriptor with one interface has the following components with their sizes indicated.
// 178	Header
//      10	(Header definition)
//      168	Configuration subset header
//		    8	(Header Definittion)
//		    160	Function Subset Header
//			    8	(Header Definittion)
//			    20	MS OS 2.0 Compatible ID descriptor
//			    132	MS OS 2.0 registry property desc
// Or, in hexadecimal:
// 0xB2	Header
//      0x0A	(Header definition)
//      0xA8	Configuration subset header
//          0x08	(Header Definittion)
//          0xA0	Function Subset Header
//			    0x08	(Header Definittion)
//			    0x14	MS OS 2.0 Compatible ID descriptor
//			    0x84	MS OS 2.0 registry property desc
// For the sake of shorter defines, let's abbreviate "MS_OS_20_DESC_[xyz]_LEN" to "MO2DL_[xyz]".
// Defines for the sizes of different parts of the descriptor.
#define MO2DL_REGISTRY_PROPERTY_DESCRIPTOR 0x84
#define MO2DL_COMPATIBLE_ID_DESCRIPTOR 0x14
// This define includes the header size itself as well as the size of it's child components
#define MO2DL_FUNCTION_SUBSET_HEADER (0x08 + MO2DL_COMPATIBLE_ID_DESCRIPTOR + MO2DL_REGISTRY_PROPERTY_DESCRIPTOR)
// The following defines DO NOT include the sizes of their components as multiple components are needed for our use case.
// I have prefixed these defines with "EXCL" for "exclusive of" the component sizes.
#define MO2DL_EXCL_CONFIGURATION_SUBSET_HEADER 0x08 // You will need to add the correct amount of MO2DL_FUNCTION_SUBSET_HEADER.
#define MO2DL_EXCL_HEADER 0x0A                      // You will need to add the correct amount of both MO2DL_FUNCTION_SUBSET_HEADER and MO2DL_EXCL_CONFIGURATION_SUBSET_HEADER.

//--------------------------------------------------------------------+
// BOS Descriptor: Configuration
//--------------------------------------------------------------------+

#ifndef INCLUDES_FLASHLOADER

#define CONFIGURATION_SUBSET_HEADER_SIZE (MO2DL_FUNCTION_SUBSET_HEADER + MO2DL_EXCL_CONFIGURATION_SUBSET_HEADER)

// Standalone configuration
#define MS_OS_20_DESC_LEN (                                                                                     \
    MO2DL_EXCL_HEADER /* Microsoft OS 2.0 descriptor header (not including child components) */ +               \
    MO2DL_EXCL_CONFIGURATION_SUBSET_HEADER /* Configuration subset header (not including child components) */ + \
    MO2DL_FUNCTION_SUBSET_HEADER /* Function subset header + contents [including child components, only one needed for vendor-specific interface] */)
#else

#define CONFIGURATION_SUBSET_HEADER_SIZE (MO2DL_FUNCTION_SUBSET_HEADER + MO2DL_FUNCTION_SUBSET_HEADER + MO2DL_EXCL_CONFIGURATION_SUBSET_HEADER)

// Flashloader configuration
#define MS_OS_20_DESC_LEN (                                                                                     \
    MO2DL_EXCL_HEADER /* Microsoft OS 2.0 descriptor header (not including child components) */ +               \
    MO2DL_EXCL_CONFIGURATION_SUBSET_HEADER /* Configuration subset header (not including child components) */ + \
    MO2DL_FUNCTION_SUBSET_HEADER /* Function subset header + contents (vendor specific interface) */ +          \
    MO2DL_FUNCTION_SUBSET_HEADER /* Function subset header + contents (DFU interface) */                        \
)

#endif

#define BOS_TOTAL_LEN (TUD_BOS_DESC_LEN + TUD_BOS_WEBUSB_DESC_LEN + TUD_BOS_MICROSOFT_OS_DESC_LEN)

// BOS Descriptor is required for webUSB
uint8_t const desc_bos[] =
    {
        // total length, number of device caps
        TUD_BOS_DESCRIPTOR(BOS_TOTAL_LEN, 2),

        // Vendor Code, iLandingPage
        TUD_BOS_WEBUSB_DESCRIPTOR(VENDOR_REQUEST_WEBUSB, 1),

        // Microsoft OS 2.0 descriptor
        TUD_BOS_MS_OS_20_DESCRIPTOR(MS_OS_20_DESC_LEN, VENDOR_REQUEST_MICROSOFT)};

uint8_t const *tud_descriptor_bos_cb(void)
{
    return desc_bos;
}

uint8_t const desc_ms_os_20[] =
    {
        // Set header: length, type, windows version, total length
        U16_TO_U8S_LE(0x000A), U16_TO_U8S_LE(MS_OS_20_SET_HEADER_DESCRIPTOR), U32_TO_U8S_LE(0x06030000), U16_TO_U8S_LE(MS_OS_20_DESC_LEN),

        // Configuration subset header: length, type, configuration index, reserved, configuration total length
        U16_TO_U8S_LE(0x0008), U16_TO_U8S_LE(MS_OS_20_SUBSET_HEADER_CONFIGURATION), 0, 0, U16_TO_U8S_LE(CONFIGURATION_SUBSET_HEADER_SIZE),

        // Function Subset header: length, type, first interface, reserved, subset length
        U16_TO_U8S_LE(0x0008), U16_TO_U8S_LE(MS_OS_20_SUBSET_HEADER_FUNCTION), ITF_NUM_VENDOR, 0, U16_TO_U8S_LE(MO2DL_FUNCTION_SUBSET_HEADER),

        // MS OS 2.0 Compatible ID descriptor: length, type, compatible ID, sub compatible ID
        U16_TO_U8S_LE(0x0014), U16_TO_U8S_LE(MS_OS_20_FEATURE_COMPATBLE_ID), 'W', 'I', 'N', 'U', 'S', 'B', 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // sub-compatible

        // MS OS 2.0 Registry property descriptor: length, type
        U16_TO_U8S_LE(MO2DL_REGISTRY_PROPERTY_DESCRIPTOR), U16_TO_U8S_LE(MS_OS_20_FEATURE_REG_PROPERTY),
        U16_TO_U8S_LE(0x0007), U16_TO_U8S_LE(0x002A), // wPropertyDataType, wPropertyNameLength and PropertyName "DeviceInterfaceGUIDs\0" in UTF-16
        'D', 0x00, 'e', 0x00, 'v', 0x00, 'i', 0x00, 'c', 0x00, 'e', 0x00, 'I', 0x00, 'n', 0x00, 't', 0x00, 'e', 0x00,
        'r', 0x00, 'f', 0x00, 'a', 0x00, 'c', 0x00, 'e', 0x00, 'G', 0x00, 'U', 0x00, 'I', 0x00, 'D', 0x00, 's', 0x00, 0x00, 0x00,
        U16_TO_U8S_LE(0x0050), // wPropertyDataLength
                               // bPropertyData: “{975F44D9-0D08-43FD-8B3E-127CA8AFFF9D}”.
        '{', 0x00, '9', 0x00, '7', 0x00, '5', 0x00, 'F', 0x00, '4', 0x00, '4', 0x00, 'D', 0x00, '9', 0x00, '-', 0x00,
        '0', 0x00, 'D', 0x00, '0', 0x00, '8', 0x00, '-', 0x00, '4', 0x00, '3', 0x00, 'F', 0x00, 'D', 0x00, '-', 0x00,
        '8', 0x00, 'B', 0x00, '3', 0x00, 'E', 0x00, '-', 0x00, '1', 0x00, '2', 0x00, '7', 0x00, 'C', 0x00, 'A', 0x00,
        '8', 0x00, 'A', 0x00, 'F', 0x00, 'F', 0x00, 'F', 0x00, '9', 0x00, 'D', 0x00, '}', 0x00, 0x00, 0x00, 0x00, 0x00,

#ifdef INCLUDES_FLASHLOADER
        // Function Subset header: length, type, first interface, reserved, subset length
        U16_TO_U8S_LE(0x0008), U16_TO_U8S_LE(MS_OS_20_SUBSET_HEADER_FUNCTION), ITF_NUM_DFU_MODE, 0, U16_TO_U8S_LE(MO2DL_FUNCTION_SUBSET_HEADER),

        // MS OS 2.0 Compatible ID descriptor: length, type, compatible ID, sub compatible ID
        U16_TO_U8S_LE(0x0014), U16_TO_U8S_LE(MS_OS_20_FEATURE_COMPATBLE_ID), 'W', 'I', 'N', 'U', 'S', 'B', 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // sub-compatible

        // MS OS 2.0 Registry property descriptor: length, type
        U16_TO_U8S_LE(MO2DL_REGISTRY_PROPERTY_DESCRIPTOR), U16_TO_U8S_LE(MS_OS_20_FEATURE_REG_PROPERTY),
        U16_TO_U8S_LE(0x0007), U16_TO_U8S_LE(0x002A), // wPropertyDataType, wPropertyNameLength and PropertyName "DeviceInterfaceGUIDs\0" in UTF-16
        'D', 0x00, 'e', 0x00, 'v', 0x00, 'i', 0x00, 'c', 0x00, 'e', 0x00, 'I', 0x00, 'n', 0x00, 't', 0x00, 'e', 0x00,
        'r', 0x00, 'f', 0x00, 'a', 0x00, 'c', 0x00, 'e', 0x00, 'G', 0x00, 'U', 0x00, 'I', 0x00, 'D', 0x00, 's', 0x00, 0x00, 0x00,
        U16_TO_U8S_LE(0x0050), // wPropertyDataLength
                               // bPropertyData: “{975F44D9-0D08-43FD-8B3E-127CA8AFFF9D}”.
        '{', 0x00, '9', 0x00, '7', 0x00, '5', 0x00, 'F', 0x00, '4', 0x00, '4', 0x00, 'D', 0x00, '9', 0x00, '-', 0x00,
        '0', 0x00, 'D', 0x00, '0', 0x00, '8', 0x00, '-', 0x00, '4', 0x00, '3', 0x00, 'F', 0x00, 'D', 0x00, '-', 0x00,
        '8', 0x00, 'B', 0x00, '3', 0x00, 'E', 0x00, '-', 0x00, '1', 0x00, '2', 0x00, '7', 0x00, 'C', 0x00, 'A', 0x00,
        '8', 0x00, 'A', 0x00, 'F', 0x00, 'F', 0x00, 'F', 0x00, '9', 0x00, 'D', 0x00, '}', 0x00, 0x00, 0x00, 0x00, 0x00
#endif
};

TU_VERIFY_STATIC(sizeof(desc_ms_os_20) == MS_OS_20_DESC_LEN, "Incorrect size");

//--------------------------------------------------------------------+
// String Descriptors
//--------------------------------------------------------------------+

// array of pointer to string descriptors
char const *string_desc_arr[] =
    {
        (const char[]){0x09, 0x04},        // 0: is supported language is English (0x0409)
        "City of London UK",               // 1: Manufacturer
        "Genoswitch Measurement Platform", // 2: Product
        "123456",                          // 3: Serials, should use chip ID
        "Genoswitch MP CDC",               // 4: CDC Interface
        "Genoswitch MP WebUSB"             // 5: Vendor Interface
#ifdef INCLUDES_FLASHLOADER
        "Genoswitch MP DFU" // 6: DFU Interface
#endif
};

static uint16_t _desc_str[32];

// Invoked when received GET STRING DESCRIPTOR request
// Application return pointer to descriptor, whose contents must exist long enough for transfer to complete
uint16_t const *tud_descriptor_string_cb(uint8_t index, uint16_t langid)
{
    (void)langid;

    uint8_t chr_count;

    if (index == 0)
    {
        memcpy(&_desc_str[1], string_desc_arr[0], 2);
        chr_count = 1;
    }
    else
    {
        // Note: the 0xEE index string is a Microsoft OS 1.0 Descriptors.
        // https://docs.microsoft.com/en-us/windows-hardware/drivers/usbcon/microsoft-defined-usb-descriptors

        if (!(index < sizeof(string_desc_arr) / sizeof(string_desc_arr[0])))
            return NULL;

        const char *str = string_desc_arr[index];

        // Cap at max char
        chr_count = (uint8_t)strlen(str);
        if (chr_count > 31)
            chr_count = 31;

        // Convert ASCII string into UTF-16
        for (uint8_t i = 0; i < chr_count; i++)
        {
            _desc_str[1 + i] = str[i];
        }
    }

    // first byte is length (including header), second byte is string type
    _desc_str[0] = (uint16_t)((TUSB_DESC_STRING << 8) | (2 * chr_count + 2));

    return _desc_str;
}