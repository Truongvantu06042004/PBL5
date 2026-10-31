# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

add_compile_options(-fdiagnostics-color=always)

# Locate ESP RainMaker. Prefer the correctly-spelled environment variable, but
# keep support for the legacy RAIMAKER_PATH name used by the original example.
if(DEFINED ENV{RAINMAKER_PATH} AND NOT "$ENV{RAINMAKER_PATH}" STREQUAL "")
    set(RAINMAKER_PATH "$ENV{RAINMAKER_PATH}")
elseif(DEFINED ENV{RAIMAKER_PATH} AND NOT "$ENV{RAIMAKER_PATH}" STREQUAL "")
    set(RAINMAKER_PATH "$ENV{RAIMAKER_PATH}")
elseif(EXISTS "${CMAKE_CURRENT_LIST_DIR}/../third_party/esp-rainmaker")
    set(RAINMAKER_PATH "${CMAKE_CURRENT_LIST_DIR}/../third_party/esp-rainmaker")
else()
    message(FATAL_ERROR
        "ESP RainMaker was not found. Clone it into device_firmware/third_party/esp-rainmaker "
        "or set RAINMAKER_PATH to the esp-rainmaker repository.")
endif()

# Add RainMaker components and other common application components
set(EXTRA_COMPONENT_DIRS ${CMAKE_CURRENT_LIST_DIR}/../components/light_driver
                        ${CMAKE_CURRENT_LIST_DIR}/../components/button
                        ${CMAKE_CURRENT_LIST_DIR}/../components/app_storage
                        ${CMAKE_CURRENT_LIST_DIR}/../components/app_wifi
                        # ESP RainMaker v1.17+ declares its supporting
                        # components through idf_component.yml. ESP-IDF's
                        # component manager downloads them automatically.
                        ${RAINMAKER_PATH}/components/esp_rainmaker
                        )

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# Increment this before creating each OTA image (for example: 1.0.0 -> 1.0.1).
# RainMaker rejects a cloud OTA image with the same version as the running one.
set(PROJECT_VER "1.0.0")
project(5_rainmaker)
