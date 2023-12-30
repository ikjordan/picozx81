if (PICO_ON_DEVICE AND "${CMAKE_BUILD_TYPE}" STREQUAL "Debug" AND PICO_DEOPTIMIZED_DEBUG)
    message("scanvideo is disabled for 'Debug' builds when PICO_DEOPTIMIZED_DEBUG=1")
else()
    add_library(mod_scanvideo INTERFACE)

    pico_generate_pio_header(mod_scanvideo ${CMAKE_CURRENT_LIST_DIR}/scanvideo.pio PATH include/pico/scanvideo)

    target_sources(mod_scanvideo INTERFACE
            ${CMAKE_CURRENT_LIST_DIR}/vga_modes.c
    )

    target_include_directories(mod_scanvideo INTERFACE ${CMAKE_CURRENT_LIST_DIR}/include)
    target_link_libraries(mod_scanvideo INTERFACE pico_base_headers pico_util_buffer)
endif()
