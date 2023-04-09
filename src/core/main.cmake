set(FREERTOS_KERNEL_PATH ../lib/freertos-smp)

# 'Subproject' name
set (MAIN main)

# Pico SDK is pulled in in the root CMakeLists.txt file.

# Pull in FreeRTOS
include(FreeRTOS_Kernel_import.cmake)

# Include git version tracking submodule
add_subdirectory(../lib/cmake-git-version-tracking git) # copy/compile to "git" folder in the build directory *^* build/git


# Init SDK
pico_sdk_init()


# Add executable targets
add_executable(${MAIN} main.c)


# RTOS config "FreeRTOSConfig.h"
target_include_directories(${MAIN} PRIVATE 
    ${CMAKE_CURRENT_LIST_DIR}/rtos-config/
    ${CMAKE_CURRENT_LIST_DIR}/tinyusb-config/

)

target_sources(${MAIN} PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/main.c
    ${CMAKE_CURRENT_LIST_DIR}/usb/tasks.c
    ${CMAKE_CURRENT_LIST_DIR}/usb/vendor_request.c
    # TinyUSB functions not picked up unless we include it here (tud_vendor_control_xfer_cb)
    ${CMAKE_CURRENT_LIST_DIR}/usb/webusb.c
    ${CMAKE_CURRENT_LIST_DIR}/tinyusb-config/usb_descriptors.c
    ${CMAKE_CURRENT_LIST_DIR}/tasks/bulk.c
    ${CMAKE_CURRENT_LIST_DIR}/tasks/mcu_temperature.c
    ${CMAKE_CURRENT_LIST_DIR}/config.h
    )

# Link to libraries  (after sdk init)
# pico_stdlib needed as FreeRTOS uses panic_unsupported
# memory management: FreeRTOS-Kernel-Heap# required for pvPortMalloc
# tinyusb_device tinyusb_board (https://github.com/raspberrypi/pico-examples/blob/master/usb/device/dev_hid_composite/CMakeLists.txt)
target_link_libraries(${MAIN} pico_stdlib hardware_adc pico_unique_id FreeRTOS-Kernel FreeRTOS-Kernel-Heap4 tinyusb_device tinyusb_board cmake_git_version_tracking)

# stdio only on UART (UART0 by default, pins 1 and 2)
pico_enable_stdio_usb(${MAIN} 0)
pico_enable_stdio_uart(${MAIN} 1)

# Create extra files, such as .uf2
pico_add_extra_outputs(${MAIN})

# Set main_uf2 var to we know the file to combine (see combined.cmake)
set(MAIN_UF2 ${CMAKE_CURRENT_BINARY_DIR}/${MAIN}.uf2)

# Based on lib/pico-flashloader/CMakeLists.txt#L86
# Use a separate linker script for the application to make sure it is built
# to run at the right location (after the flashloader).
set_linker_script(${MAIN} application.ld)