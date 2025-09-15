cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(embedded)

target_sources(app PRIVATE 
                            src/main.c
                            src/Drivers/Screens/LPM013M126A/LPM013M126A.c
                            src/Drivers/Screens/WatchAnalog/WatchAnalog.c)
