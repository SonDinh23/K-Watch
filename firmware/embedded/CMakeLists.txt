cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(embedded)

target_sources(app PRIVATE src/main.c src/app.c)

add_subdirectory(src/lib/drivers)
add_subdirectory(src/lib/mgmt_screen)
add_subdirectory(src/lib/mgmt_system)