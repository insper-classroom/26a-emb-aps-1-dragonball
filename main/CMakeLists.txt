# add_executable(pico_emb main.c)

add_executable(pico_emb
    main.c

    # ILI9341 driver
    tft_lcd_ili9341/ili9341/ili9341.c

    # GFX engine
    tft_lcd_ili9341/gfx/gfx_ili9341.c

    # Touch
    tft_lcd_ili9341/touch_resistive/touch_resistive.c
)

set_target_properties(pico_emb PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# pull in common dependencies
target_link_libraries(pico_emb pico_stdlib)



target_include_directories(pico_emb PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/tft_lcd_ili9341/gfx
    ${CMAKE_CURRENT_LIST_DIR}/tft_lcd_ili9341/ili9341
    ${CMAKE_CURRENT_LIST_DIR}/tft_lcd_ili9341/touch_resistive
)


target_link_libraries(pico_emb
    pico_stdlib
    pico_multicore
    hardware_spi
    hardware_adc
)

# create map/bin/hex/uf2 file etc.
pico_add_extra_outputs(pico_emb)