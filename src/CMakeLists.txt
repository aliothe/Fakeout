# Main executable target
add_executable(breakout
    main.cpp
    sdl_window.cpp
    level.cpp
)

# Include directories
target_include_directories(breakout PRIVATE ${CMAKE_SOURCE_DIR}/include)

# Link SDL3 shared library from Conan
target_link_libraries(breakout PRIVATE SDL3::SDL3 -lGL)