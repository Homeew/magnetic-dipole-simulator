cmake_minimum_required(VERSION 3.14)
project(magnetic-dipole-simulator VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_options(-Wall -Wextra -Wpedantic)

include_directories(include)

add_library(dipole_lib
    src/sensor_array.cpp
    src/lm_solver.cpp
)
target_include_directories(dipole_lib PUBLIC include)
set_target_properties(dipole_lib PROPERTIES POSITION_INDEPENDENT_CODE ON)
add_executable(dipole_sim src/main.cpp)
target_link_libraries(dipole_sim PRIVATE dipole_lib)

include(FetchContent)

FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v2.13.10
)
FetchContent_MakeAvailable(Catch2)

FetchContent_Declare(
    pybind11
    GIT_REPOSITORY https://github.com/pybind/pybind11.git
    GIT_TAG        v2.11.1
)
FetchContent_MakeAvailable(pybind11)

pybind11_add_module(dipole_py python/bindings.cpp)
target_link_libraries(dipole_py PRIVATE dipole_lib)
target_include_directories(dipole_py PRIVATE ${CMAKE_SOURCE_DIR}/include)

enable_testing()

add_executable(tests tests/test_dipole.cpp)
target_link_libraries(tests PRIVATE dipole_lib Catch2::Catch2)
target_include_directories(tests PRIVATE include)
add_test(NAME dipole_tests COMMAND tests)
