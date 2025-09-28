target_sources(app PRIVATE src/main.c
                            src/handleMain.c)

target_link_libraries(app PRIVATE ble driver)
