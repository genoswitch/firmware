#include <string.h>
#include "pico/stdlib.h"
#include "hardware/sync.h"
#include "hardware/flash.h"
#include "hardware/watchdog.h"
#include "hardware/structs/watchdog.h"

#include "record.h"
#include "process.h"
#include "../crc32.h"

// lib/pico-flashloader
#include "flashloader.h"

// Store the given image in flash then reboot into the flashloader to replace
// the current application with the new image.
void flashImage(tFlashHeader *header, uint32_t length)
{
    // Calculate length of header plus length of data
    uint32_t totalLength = sizeof(tFlashHeader) + length;

    // Round erase length up to next 4096 byte boundary
    // (The pico's flash works in 4k blocks)
    uint32_t eraseLength = (totalLength + 4095) & 0xfffff000;

    uint32_t status;

    header->magic1 = FLASH_MAGIC1;
    header->magic2 = FLASH_MAGIC2;
    header->length = length;
    header->crc32 = crc32_no_reflection_or_final_xor(header->data, length);

    uart_puts(PICO_DEFAULT_UART_INSTANCE, "Storing new image in flash and then rebooting\r\n");

    status = save_and_disable_interrupts();

    flash_range_erase(FLASH_IMAGE_OFFSET, eraseLength);
    flash_range_program(FLASH_IMAGE_OFFSET, (uint8_t *)header, totalLength);

    restore_interrupts(status);
    uart_puts(PICO_DEFAULT_UART_INSTANCE, "Rebooting into flashloader in 1 second\r\n");

    // Set up watchdog scratch registers so that the flashloader knows
    // what to do after the reset
    watchdog_hw->scratch[0] = FLASH_MAGIC1;
    watchdog_hw->scratch[1] = XIP_BASE + FLASH_IMAGE_OFFSET;

    watchdog_reboot(0x00000000, 0x00000000, 1000);

    // Wait for the reset
    while (true)
        // Effectively a NOP
        tight_loop_contents();
}