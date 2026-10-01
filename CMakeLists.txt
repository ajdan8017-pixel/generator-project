cmake_minimum_required(VERSION 3.16)
project(generator LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_compile_options(-Wall -Wextra -O2)

add_executable(generator
    src/main.cpp
    src/template_parser.cpp
    src/json_parser.cpp
    src/csv_parser.cpp
    src/module_loader.cpp
    src/data_provider.cpp
    src/function_registry.cpp
)
target_include_directories(generator PRIVATE src)
target_link_libraries(generator PRIVATE ${CMAKE_DL_LIBS})

add_library(math MODULE modules/math.cpp)
set_target_properties(math PROPERTIES PREFIX "lib" OUTPUT_NAME "math"
                      LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/modules")

add_library(csv MODULE modules/csv.cpp src/csv_parser.cpp)
set_target_properties(csv PROPERTIES PREFIX "lib" OUTPUT_NAME "csv"
                     LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/modules")

enable_testing()
add_executable(test_parser tests/test_parser.cpp src/template_parser.cpp)
target_include_directories(test_parser PRIVATE src)
add_test(NAME parser COMMAND test_parser)

add_executable(test_json tests/test_json.cpp src/json_parser.cpp)
target_include_directories(test_json PRIVATE src)
add_test(NAME json COMMAND test_json)
