cmake_minimum_required(VERSION 3.26)
project(breakout LANGUAGES CXX)

# C++23 Standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build Type Configuration
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# Professional Compiler Flags (-Werror mandatory)
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    add_compile_options(
        -Wall -Wextra -Wpedantic -Werror
        -Wswitch-default -Wfloat-equal -Wsign-conversion
        -Wold-style-cast -Wcast-align
    )
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    add_compile_options(
        -Wall -Wextra -Wpedantic -Werror
        -Wswitch-default -Wfloat-equal
        -Wold-style-cast -Wcast-align
    )
endif()

# Linker flags for static linking
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++")

# clang-tidy Integration (mandatory fixes)
set(CMAKE_CXX_CLANG_TIDY
    clang-tidy;
    -checks=modernize-*,cppcoreguidelines-*,performance-*,bugprone-*,readability-*;
    -warnings-as-errors=*
)

# Find SDL3 using Conan
find_package(SDL3 REQUIRED)

# Set global output directory for all executables
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

# Enable testing for the project
enable_testing()

# Conan integration
list(APPEND CMAKE_MODULE_PATH "${CMAKE_BINARY_DIR}")
list(APPEND CMAKE_PREFIX_PATH "${CMAKE_BINARY_DIR}")

# Add subdirectories
add_subdirectory(src)
add_subdirectory(tests)

# Add custom target to run tests after build (manual trigger)
add_custom_target(run_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running unit tests..."
    VERBATIM
)

# Convenience aliases
add_custom_target(run_unit_tests DEPENDS run_tests)
add_custom_target(validate DEPENDS run_tests)

