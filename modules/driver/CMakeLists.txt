# Tạo thư viện độc lập cho component
add_library(driver)
target_sources(driver PRIVATE
  src/nvs/nvs.c
)

# Public headers cho người khác include
target_include_directories(driver PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Private headers chỉ nội bộ component
target_include_directories(driver PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Nếu phụ thuộc Zephyr, link với zephyr-interface
target_link_libraries(driver PUBLIC zephyr_interface)