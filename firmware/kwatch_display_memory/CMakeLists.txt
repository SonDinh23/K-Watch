cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(kwatch_display_memory)

target_sources(app PRIVATE 
                                src/driver_display_memory/LPM013M126A.c
                                src/Kwatch_GFX.c
                                src/main.c)
