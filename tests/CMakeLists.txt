cmake_minimum_required(VERSION 3.22)

project(mcu_ioc_host_tests C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

enable_testing()

add_library(app_logic_under_test
    ../Core/App/app_logic.c
    ../Core/App/app_text.c
)

target_include_directories(app_logic_under_test PUBLIC
    ../Core/App
)

target_compile_options(app_logic_under_test PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -Wconversion
    -Wsign-conversion
    -Wshadow
    -Wdouble-promotion
    -Wformat=2
    -Wundef
    -Wstrict-prototypes
    -Wmissing-prototypes
)

add_executable(test_app_logic
    test_app_logic.c
)

target_link_libraries(test_app_logic PRIVATE
    app_logic_under_test
)

target_compile_options(test_app_logic PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -Wconversion
    -Wsign-conversion
    -Wshadow
    -Wdouble-promotion
    -Wformat=2
    -Wundef
    -Wstrict-prototypes
    -Wmissing-prototypes
)

add_test(NAME app_logic COMMAND test_app_logic)
