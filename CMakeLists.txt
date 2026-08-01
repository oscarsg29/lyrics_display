cmake_minimum_required(VERSION 3.22)

#
# This file is generated only once,
# and is not re-generated if converter is called multiple times.
#
# User is free to modify the file as much as necessary
#

# Setup compiler settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)


# Define the build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# Set the project name
set(CMAKE_PROJECT_NAME mcu_ioc)

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Core project settings
project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

# Enable CMake support for ASM and C languages
enable_language(C ASM)

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME})

# Add STM32CubeMX generated sources
add_subdirectory(cmake/stm32cubemx)

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined library search paths
)

# Add sources to executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user sources here
    Core/App/app_logic.c
    Core/SSD1306/ssd1306.c
    Core/SSD1306/ssd1306_fonts.c
    Core/Src/sd_diskio.c
    Middlewares/Third_Party/FatFs/src/ff.c
    Middlewares/Third_Party/FatFs/src/option/ffunicode.c
)

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined include paths
    Core/App
    Core/SSD1306
    Middlewares/Third_Party/FatFs/src
)

# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined symbols
)

# Remove wrong libob.a library dependency when using cpp files
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx

    # Add user defined libraries
)

# Host-side unit tests. These are built with the native compiler in a separate
# build tree because the main firmware target uses the ARM embedded toolchain.
add_custom_target(unit_tests
    COMMAND ${CMAKE_COMMAND} -S ${CMAKE_SOURCE_DIR}/tests -B ${CMAKE_SOURCE_DIR}/build/host-tests
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_SOURCE_DIR}/build/host-tests
    COMMAND ${CMAKE_CTEST_COMMAND} --test-dir ${CMAKE_SOURCE_DIR}/build/host-tests --verbose
    USES_TERMINAL
    COMMENT "Configuring, building, and running host unit tests"
)
