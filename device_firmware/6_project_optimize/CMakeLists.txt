# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

add_compile_options(-fdiagnostics-color=always)

# Use the bundled RainMaker checkout by default, with environment overrides.
if(DEFINED ENV{RAINMAKER_PATH} AND NOT "$ENV{RAINMAKER_PATH}" STREQUAL "")
    set(RAINMAKER_PATH "$ENV{RAINMAKER_PATH}")
elseif(DEFINED ENV{RAIMAKER_PATH} AND NOT "$ENV{RAIMAKER_PATH}" STREQUAL "")
    set(RAINMAKER_PATH "$ENV{RAIMAKER_PATH}")
else()
    set(RAINMAKER_PATH "${CMAKE_CURRENT_LIST_DIR}/../third_party/esp-rainmaker")
endif()

# Add RainMaker components and other common application components
set(EXTRA_COMPONENT_DIRS ${CMAKE_CURRENT_LIST_DIR}/../components/light_driver
                        ${CMAKE_CURRENT_LIST_DIR}/../components/button
                        ${CMAKE_CURRENT_LIST_DIR}/../components/app_storage
                        ${CMAKE_CURRENT_LIST_DIR}/../components/app_wifi
                        ${RAINMAKER_PATH}/components/esp_rainmaker
                        )

set(EXCLUDE_COMPONENTS "coap" "asio")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(6_performance_optimize)
