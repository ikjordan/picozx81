cmake_minimum_required(VERSION 3.13)

# Pull in PICO SDK (must be before project)
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)
if (PICO_SDK_VERSION_STRING VERSION_LESS "1.2.0")
message(FATAL_ERROR "Require at least Raspberry Pi Pico SDK version 1.2.0")
endif()

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_add_subdirectory(pico_scanvideo)
pico_add_subdirectory(pico_scanvideo_dpi)
