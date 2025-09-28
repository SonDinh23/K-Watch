# cmake_minimum_required(VERSION 3.20.0)
# find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

# project(Embedded)

# target_sources(app PRIVATE src/main.c)


cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(Embedded)

add_subdirectory(app)
add_subdirectory(modules/ble)
add_subdirectory(modules/driver)