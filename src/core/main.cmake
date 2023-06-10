set(FREERTOS_KERNEL_PATH ../lib/freertos-smp)

# 'Subproject' name
set (MAIN core)

# Define this subproject as a library.
# This allows us to create various executable targets
# (for example, with and without the stage3 flashloader)
# by linking against this library.
add_library(${MAIN})

# Pico SDK is pulled in in the root CMakeLists.txt file.

# Pull in FreeRTOS
include(FreeRTOS_Kernel_import.cmake)

# Include git version tracking submodule
add_subdirectory(../lib/cmake-git-version-tracking git) # copy/compile to "git" folder in the build directory *^* build/git


# Init SDK
pico_sdk_init()


# RTOS config "FreeRTOSConfig.h"
# These sources should be PUBLIC so dependents linking against this library inherit these.
target_include_directories(${MAIN} PUBLIC 
    ${CMAKE_CURRENT_LIST_DIR}/rtos-config/
    ${CMAKE_CURRENT_LIST_DIR}/tinyusb-config/

)

# These sources should be PUBLIC so dependents linking against this library inherit these.
target_sources(${MAIN} PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/main.c
    ${CMAKE_CURRENT_LIST_DIR}/crit_trigger.c
    ${CMAKE_CURRENT_LIST_DIR}/feature_set.c
    ${CMAKE_CURRENT_LIST_DIR}/semaphores.c
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
# pico_malloc, pico_mem_ops - optimized versions of standard libraries.
# memory management: FreeRTOS-Kernel-Heap# required for pvPortMalloc
# tinyusb_device tinyusb_board (https://github.com/raspberrypi/pico-examples/blob/master/usb/device/dev_hid_composite/CMakeLists.txt)
target_link_libraries(${MAIN} pico_stdlib pico_malloc pico_mem_ops hardware_adc pico_unique_id FreeRTOS-Kernel FreeRTOS-Kernel-Heap4 tinyusb_device tinyusb_board cmake_git_version_tracking)

# stdio only on UART (UART0 by default, pins 1 and 2)
pico_enable_stdio_usb(${MAIN} 0)
pico_enable_stdio_uart(${MAIN} 1)
