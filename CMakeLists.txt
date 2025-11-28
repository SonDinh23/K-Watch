cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

set(DTC_OVERLAY_FILE "app.overlay")

project(Firmware)

target_sources(app PRIVATE 
    src/ui/watchface/face_main/face_main.c
    src/ui/watchface/face_information/face_information.c
    src/ui/watchface/face_digital.c
    src/ui/watchface/face_manager.c
    src/ui/driver/KW_GFX.c
    src/ui/driver/KW_GFX_font.c
    src/ui/driver/LPM013M126A.c

    src/drivers/rtc/rv8263.c

    src/ble/ble_peripheral.c
    src/ble/ble_information.c
    src/ble/ble_ancs.c
    src/main.c
)
