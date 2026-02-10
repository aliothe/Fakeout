#pragma once

#include "ecs/component.hpp"
#include "ecs/registry.hpp"
#include "game_constants.hpp"
#include <SDL3/SDL.h>
#include <cmath>
#include <cstdlib>
#include <random>

namespace breakout
{

// Texture holder for all game textures
struct GameTextures
{
  SDL_Texture *paddle;
  SDL_Texture *ball;
  SDL_Texture *brick_red;
  SDL_Texture *brick_yellow;
  SDL_Texture *brick_blue;
};

enum class LayoutType
{
  Rectangular,
  Circular
};

class Level
{
public:
  // Configuration for level generation
  struct Config
  {
    int min_rows;
    int max_rows;
    int min_cols;
    int max_cols;
    float brick_width;
    float brick_height;
    float top_offset;

    Config()
        : min_rows(4), max_rows(8), min_cols(8), max_cols(12), brick_width(60.0f),
          brick_height(20.0f), top_offset(100.0f) // Moved down to avoid score
    {
    }
  };

  Level(ecs::Registry &registry, const GameTextures &textures, const Config &config = Config{});

  // Generate the level with randomized parameters
  void generate();

  // Get current layout type
  [[nodiscard]] LayoutType layout_type() const
  {
    return layout_type_;
  }

  // Get actual dimensions used
  [[nodiscard]] int rows() const
  {
    return rows_;
  }
  [[nodiscard]] int cols() const
  {
    return cols_;
  }

private:
  void generate_rectangular();
  void generate_circular();
  void create_brick(float x, float y, SDL_Texture *texture, ecs::BrickColor color, int hit_points);

  ecs::Registry &registry_;
  const GameTextures &textures_;
  Config config_;

  std::mt19937 rng_{std::random_device{}()};
  std::discrete_distribution<int> brick_dist_{BRICK_WEIGHT_EMPTY, BRICK_WEIGHT_RED,
                                              BRICK_WEIGHT_YELLOW, BRICK_WEIGHT_BLUE};

  LayoutType layout_type_{LayoutType::Rectangular};
  int rows_{0};
  int cols_{0};
};

} // namespace breakout
