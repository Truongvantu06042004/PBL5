cmake_minimum_required(VERSION 3.5)

set(EXTRA_COMPONENT_DIRS $ENV{IDF_PATH}/examples/common_components/protocol_examples_common)

set(PROJECT_VER "4.0")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

project(advanced_https_ota)