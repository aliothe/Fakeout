#pragma once

#include <array>
#include <cstdint>

namespace breakout
{

inline namespace constants
{

// Window dimensions
constexpr int WINDOW_WIDTH = 800;
constexpr int WINDOW_HEIGHT = 600;

// Clear color (RGBA)
constexpr float CLEAR_COLOR_R = 0.1F;
constexpr float CLEAR_COLOR_G = 0.2F;
constexpr float CLEAR_COLOR_B = 0.3F;
constexpr float CLEAR_COLOR_A = 1.0F;
constexpr std::array<float, 4> CLEAR_COLOR{CLEAR_COLOR_R, CLEAR_COLOR_G, CLEAR_COLOR_B,
                                           CLEAR_COLOR_A};

// Paddle dimensions and positioning
constexpr float PADDLE_WIDTH = 100.0F;
constexpr float PADDLE_HEIGHT = 20.0F;
constexpr float PADDLE_Y_OFFSET = 50.0F;
constexpr int PADDLE_TEXTURE_WIDTH = 100;
constexpr int PADDLE_TEXTURE_HEIGHT = 20;
constexpr float PADDLE_CENTER_X = WINDOW_WIDTH / 2.0F;
constexpr float PADDLE_CENTER_Y = WINDOW_HEIGHT - PADDLE_Y_OFFSET - PADDLE_HEIGHT / 2.0F;

// Ball dimensions and positioning
constexpr float BALL_SIZE = 16.0F;
constexpr int BALL_TEXTURE_SIZE = 16;
constexpr float BALL_INITIAL_VELOCITY_X = 200.0F;
constexpr float BALL_INITIAL_VELOCITY_Y = 300.0F;
constexpr float BALL_CENTER_X = WINDOW_WIDTH / 2.0F;
constexpr float BALL_CENTER_Y = WINDOW_HEIGHT / 2.0F;

// Brick dimensions and layout
constexpr float BRICK_WIDTH = 60.0F;
constexpr float BRICK_HEIGHT = 20.0F;
constexpr int BRICK_TEXTURE_WIDTH = 60;
constexpr int BRICK_TEXTURE_HEIGHT = 20;
constexpr int BRICK_ROWS = 6;
constexpr int BRICK_COLS = 10;
constexpr float BRICK_WALL_TOP_OFFSET = 60.0F;

// Brick colors (RGB values)
constexpr std::uint8_t BRICK_RED_R = 220;
constexpr std::uint8_t BRICK_RED_G = 50;
constexpr std::uint8_t BRICK_RED_B = 50;

constexpr std::uint8_t BRICK_YELLOW_R = 220;
constexpr std::uint8_t BRICK_YELLOW_G = 200;
constexpr std::uint8_t BRICK_YELLOW_B = 50;

constexpr std::uint8_t BRICK_BLUE_R = 50;
constexpr std::uint8_t BRICK_BLUE_G = 100;
constexpr std::uint8_t BRICK_BLUE_B = 220;

// Brick generation weights for std::discrete_distribution
// Weights: Empty=4, Red=1, Yellow=2, Blue=3 (total=10)
// Probabilities: Empty=40%, Red=10%, Yellow=20%, Blue=30%
constexpr int BRICK_WEIGHT_EMPTY = 4;
constexpr int BRICK_WEIGHT_RED = 1;
constexpr int BRICK_WEIGHT_YELLOW = 2;
constexpr int BRICK_WEIGHT_BLUE = 3;

// Sprite tint colors (RGBA)
constexpr std::array<float, 4> DEFAULT_TINT{1.0F, 1.0F, 1.0F, 1.0F};

// Zero velocity for stationary objects
constexpr std::array<float, 2> ZERO_VELOCITY{0.0F, 0.0F};

// Halving factor for centering calculations
constexpr float HALF_FACTOR = 2.0F;

} // namespace constants

} // namespace breakout
