# Tạo thư viện độc lập cho component
add_library(ble)
target_sources(ble PRIVATE
                  src/ble_manager.c
)

# Public headers cho người khác include
target_include_directories(ble PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Private headers chỉ nội bộ component
target_include_directories(ble PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Nếu phụ thuộc Zephyr, link với zephyr-interface
target_link_libraries(ble PUBLIC zephyr_interface)