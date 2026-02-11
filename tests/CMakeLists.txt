# Test executable target
add_executable(test_main
    test_main.cpp
    ${CMAKE_SOURCE_DIR}/src/level.cpp
)

# Find Catch2 from Conan
find_package(Catch2 REQUIRED)

# Include project headers
target_include_directories(test_main PRIVATE ${CMAKE_SOURCE_DIR}/include)

# Link Catch2 and SDL3 (needed for component types and SoundManager)
target_link_libraries(test_main PRIVATE Catch2::Catch2WithMain SDL3::SDL3)

# Disable clang-tidy for test code (magic numbers, test macros, etc.)
set_target_properties(test_main PROPERTIES CXX_CLANG_TIDY "")

# Enable testing
enable_testing()

# Add test
add_test(NAME breakout_unit_tests COMMAND test_main)

# Run tests automatically when test_main is built
add_custom_command(
    TARGET test_main
    POST_BUILD
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running unit tests after building test_main..."
    VERBATIM
)
