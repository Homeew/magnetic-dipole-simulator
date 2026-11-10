cmake_minimum_required(VERSION 3.14)   # 3.14+ needed for FetchContent
project(magnetic-dipole-simulator
        VERSION 1.0
        LANGUAGES CXX)

# ── C++ standard ─────────────────────────────────────────────────────────
set(CMAKE_CXX_STANDARD 17)           # use C++17
set(CMAKE_CXX_STANDARD_REQUIRED ON)  # error if compiler doesn't support it
set(CMAKE_CXX_EXTENSIONS OFF)        # no compiler-specific extensions

# ── Compiler warnings ─────────────────────────────────────────────────────
add_compile_options(-Wall -Wextra -Wpedantic)

# ── Include directories ───────────────────────────────────────────────────
# tell the compiler where to find our header files
include_directories(include)

# ── Library: shared code used by both the main app and tests ──────────────
add_library(dipole_lib
    src/sensor_array.cpp   # ring / grid / sphere geometry
    src/lm_solver.cpp      # Levenberg-Marquardt solver
)
target_include_directories(dipole_lib PUBLIC include)

# ── Main executable ───────────────────────────────────────────────────────
add_executable(dipole_sim src/main.cpp)
target_link_libraries(dipole_sim PRIVATE dipole_lib)

# ── Tests (Catch2 downloaded automatically) ───────────────────────────────
include(FetchContent)   # built-in CMake module for downloading dependencies

FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v2.13.10   # stable release of Catch2 v2
)
FetchContent_MakeAvailable(Catch2)   # download and configure Catch2

# enable CTest (run tests with: ctest)
enable_testing()

add_executable(tests tests/test_dipole.cpp)
target_link_libraries(tests PRIVATE dipole_lib Catch2::Catch2)
target_include_directories(tests PRIVATE include)

add_test(NAME dipole_tests COMMAND tests)
