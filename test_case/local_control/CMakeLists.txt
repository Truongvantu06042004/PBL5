cmake_minimum_required(VERSION 3.5)

add_compile_options(-fdiagnostics-color=always)


set(EXTRA_COMPONENT_DIRS
    ${CMAKE_CURRENT_LIST_DIR}/../components/light_driver
    ${CMAKE_CURRENT_LIST_DIR}/../components/button
    ${CMAKE_CURRENT_LIST_DIR}/../components/app_storage
)


include($ENV{IDF_PATH}/tools/cmake/project.cmake)

project(local_control)