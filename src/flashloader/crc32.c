// SPDX-License-Identifier: MIT

#include "hardware/dma.h"

// CRC32 calculation from a pointer using RP2040 DMA.
// (Polynomial 0x4C11DB7)
// Extensively commented with sources and references listed where appropriate.
uint32_t _internal_crc32(void *ptr, uint32_t len, uint32_t input_seed, uint32_t output_seed, bool sniff_reverse_bits, uint sniff_mode)
{
    // Initial write address
    uint32_t dest;

    // Result
    uint32_t crc;

    // Claim a free DMA channel, panic() if one is not available
    int channel = dma_claim_unused_channel(true);

    // Configure the DMA channel
    dma_channel_config config = dma_channel_get_default_config(channel);

    // (conf): Set transfer size
    // pico-examples hello_dma uses DMA_SIZE_8 (8 bits, one byte)
    // [PERMALINK] https://github.com/raspberrypi/pico-examples/blob/1c5d9aa567598e6e3eadf6d7f2d8a9342b44dab4/dma/hello_dma/hello_dma.c#L28
    // However, it appears that using DMA_SIZE_32 (32 bits, 4 bytes) is faster.
    // https://forums.raspberrypi.com/viewtopic.php?t=319315
    // This is also the setting used in usedbytes' RP2040 serial bootloader (similar to picowota)
    // [PERMALINK] https://github.com/usedbytes/rp2040-serial-bootloader/blob/f6f0d61fc03447a1401d3f9d158f35331bf0eb98/main.c#L320
    channel_config_set_transfer_data_size(&config, DMA_SIZE_32);

    // (conf): Enable read incrementing
    // This means that the DMA channel will increment from one address to the
    // next between subsequent transfers.
    // https://vanhunteradams.com/Pico/DAC/DMA_DAC.html
    // [ARCHIVED] <https://web.archive.org/web/20230304013953/https://vanhunteradams.com/Pico/DAC/DMA_DAC.html>
    channel_config_set_read_increment(&config, true);

    // (conf): Disable write incrementing
    // pico-examples hello-dma enables this, but we do not need it at the moment.
    // We are also using a different method to store the resultant value.
    // This means that the DMA channel will always write to the same memory address.
    // (see above sources)
    channel_config_set_write_increment(&config, false);

    // (conf): Allow/enable access to the DMA channel by the sniffer
    // We are using the sniffer to calculate the crc32 hash (?)
    channel_config_set_sniff_enable(&config, true);

    if (input_seed != 0)
    {
        // (conf): Seed the CRC calculation
        // For CRC32, you need to init dma_hw->sniff_data to 0xFFFFFFFF (and XOR with it afterwards)
        // Usedbytes encountered this issue and opened the below issue to solve it.
        // https://github.com/raspberrypi/pico-sdk/issues/576#issuecomment-929587218
        dma_hw->sniff_data = input_seed;
    }

    // (conf): Setup and enable the sniffer.

    // We can either use Mode 1 (requires bit-reversal), see below
    // We are using Mode 0, (IEEE802.3 polynomial) without bit reversed data.
    dma_sniffer_enable(channel, sniff_mode, true);

    if (sniff_reverse_bits)
    {
        // Using Mode 1, values that are bit-reversed versions of golang's IEEE802.3 implementation
        // Usedbytes' issue where he documented this finding:
        // https://github.com/raspberrypi/pico-sdk/issues/576#issuecomment-929608656
        // Usedbytes' code where other details are commented
        // [PERMALINK] https://github.com/usedbytes/rp2040-serial-bootloader/blob/f6f0d61fc03447a1401d3f9d158f35331bf0eb98/main.c#L328

        dma_hw->sniff_ctrl |= DMA_SNIFF_CTRL_OUT_REV_BITS;
    }
    // If we are using mode 0, we do not need to bit-reverse the data.

    // (conf): Configure the DMA channel itself.
    // Comments sourced from https://github.com/raspberrypi/pico-examples/blob/master/dma/hello_dma/hello_dma.c#L32
    dma_channel_configure(
        channel, // Channel to be configured
        &config, // Channel configuration
        &dest,   // Initial write address
        ptr,     // Initial read address (pointer in this case passed to the func)
        len / 4, // Number of transfers (divide by four as we using DMA_SIZE_32 (four bytes))
        true     // Start immediately? bool
    );

    // Wait for the calculation to finish.
    dma_channel_wait_for_finish_blocking(channel);

    // Read the result (before resetting the channel)
    // output_seed can be 0, in which case (x^0=x).
    // (Final XOR): Note that we must XOR the result with (normally) 0xFFFFFFFF (see above comments)
    crc = dma_hw->sniff_data ^ output_seed;

    // (cleanup): Disable the sniffer
    dma_sniffer_disable();

    // (cleanup): Unclaim the DMA channel, allowing it to be used by other resources
    dma_channel_unclaim(channel);

    // Return the CRC.
    return crc;
}

// - Input + Output reflection
// - Polynomial 0x4C11DB7 (default)
// - Initial seed/value 0xFFFFFFFF
// - Final XOR 0xFFFFFFFF
uint32_t crc32(void *ptr, uint32_t len)
{
    return _internal_crc32(ptr, len, 0xFFFFFFFF, 0xFFFFFFFF, true, 0x1);
}

// CRC32 calculation for the flashloader.
// (rhulme's flashloader does not use reflection or final XOR for it's crc32 checks)
// - NO Input OR Output reflection
// - Polynomial 0x4C11DB7 (default)
// - Initial seed/value 0xFFFFFFFF
// - Final XOR 0x0
uint32_t crc32_no_reflection_or_final_xor(void *ptr, uint32_t len)
{
    return _internal_crc32(ptr, len, 0xFFFFFFFF, 0, false, 0x0);
}