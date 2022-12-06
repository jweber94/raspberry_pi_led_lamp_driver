cmake_minimum_required(VERSION 3.16.3)

project("printer_lamp_driver_interaction")

set(SOURCE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/dbus_interaction.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/config_parser.cpp
)

# setup conan
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CONAN_SYSTEM_INCLUDES ON)
include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

add_executable(driver_interaction ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp ${SOURCE})
target_include_directories(driver_interaction PUBLIC ./include ${CONAN_INCLUDE_DIRS})
target_link_libraries( driver_interaction
    ${CONAN_LIBS}
)

target_compile_features(driver_interaction PRIVATE cxx_std_17)

if (BUILD_TEST)
    message("Testing enabled")
    enable_testing()
    add_subdirectory(tests)
endif()