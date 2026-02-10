#include "level.hpp"
#include "ecs/component.hpp"
#include <random>

namespace breakout
{

namespace
{
constexpr float PI = 3.14159F;
constexpr float TWO_PI = 2.0F * PI;
constexpr int CIRCLE_COLS_GROWTH_PER_RING = 2;
constexpr float RING_SPACING_FACTOR = 1.5F; // Vertical spacing between rings
constexpr float ARC_LENGTH_FACTOR = 1.3F;   // Arc length per brick (width + gap)
constexpr int MIN_BRICKS_PER_RING = 6;      // Minimum bricks in any ring
constexpr float ANGLE_OFFSET_FACTOR = 2.0F; // For dividing PI
constexpr float SCORE_AREA_FACTOR = 0.15F;  // Top 15% reserved for score
constexpr float PADDLE_AREA_FACTOR = 0.55F; // Bottom 45% reserved for paddle (bricks stop at 55%)
} // namespace

Level::Level(ecs::Registry &registry, const GameTextures &textures, const Config &config)
    : registry_{registry}, textures_{textures}, config_{config}
{
}

void Level::generate()
{
  // Randomly choose layout type
  std::uniform_int_distribution<int> layout_dist(0, 1);
  layout_type_ = (layout_dist(rng_) == 0) ? LayoutType::Rectangular : LayoutType::Circular;

  // Randomize dimensions within bounds
  std::uniform_int_distribution<int> rows_dist(config_.min_rows, config_.max_rows);
  std::uniform_int_distribution<int> cols_dist(config_.min_cols, config_.max_cols);

  rows_ = rows_dist(rng_);
  cols_ = cols_dist(rng_);

  // Generate the chosen layout
  switch (layout_type_)
  {
  case LayoutType::Rectangular:
    generate_rectangular();
    break;
  case LayoutType::Circular:
    generate_circular();
    break;
  default:
    generate_rectangular();
    break;
  }
}

void Level::generate_rectangular()
{
  // Calculate total width and center the wall
  float total_width = static_cast<float>(cols_) * config_.brick_width;
  float start_x = (WINDOW_WIDTH_F - total_width) / CENTER_DIVISOR;

  for (int row = 0; row < rows_; ++row)
  {
    for (int col = 0; col < cols_; ++col)
    {
      // Random brick type
      auto brick_type = static_cast<BrickType>(brick_dist_(rng_));

      if (brick_type == BrickType::Empty)
      {
        continue;
      }

      float x = start_x + static_cast<float>(col) * config_.brick_width;
      float y = config_.top_offset + static_cast<float>(row) * config_.brick_height;

      // Determine brick properties
      int hit_points = 0;
      SDL_Texture *texture = nullptr;
      ecs::BrickColor color = ecs::BrickColor::BLUE;

      if (brick_type == BrickType::Red)
      {
        hit_points = 3;
        texture = textures_.brick_red;
        color = ecs::BrickColor::RED;
      }
      else if (brick_type == BrickType::Yellow)
      {
        hit_points = 2;
        texture = textures_.brick_yellow;
        color = ecs::BrickColor::YELLOW;
      }
      else
      {
        hit_points = 1;
        texture = textures_.brick_blue;
        color = ecs::BrickColor::BLUE;
      }

      create_brick(x, y, texture, color, hit_points);
    }
  }
}

void Level::generate_circular()
{
  // Center of the circle - positioned to avoid score at top and paddle at bottom
  float center_x = WINDOW_WIDTH_F / CENTER_DIVISOR;
  // Position center at 40% down screen to clear score area at top
  constexpr float CIRCLE_CENTER_Y_FACTOR = 2.5F; // 1/2.5 = 40% down
  float center_y = WINDOW_HEIGHT_F / CIRCLE_CENTER_Y_FACTOR;

  // Calculate radius based on number of columns (circumference)
  // C = 2 * pi * r, so r = C / (2 * pi)
  // Approximate circumference as cols * brick_width
  float circumference = static_cast<float>(cols_) * config_.brick_width;
  float radius = circumference / (TWO_PI);

  // Use larger minimum radius for better spacing
  constexpr float MIN_CIRCLE_RADIUS_V2 = 120.0F;
  radius = std::max(radius, MIN_CIRCLE_RADIUS_V2);

  // Number of rings (rows) with better vertical spacing
  // Use larger spacing to prevent vertical overlap
  float ring_spacing = config_.brick_height * RING_SPACING_FACTOR;

  for (int ring = 0; ring < rows_; ++ring)
  {
    float current_radius = radius + (static_cast<float>(ring) * ring_spacing);

    // Calculate how many bricks fit in this ring
    // Arc length per brick must account for both width and gap
    float arc_length_per_brick = config_.brick_width * ARC_LENGTH_FACTOR; // Width + 30% gap
    float total_arc = TWO_PI * current_radius;
    int bricks_in_ring = static_cast<int>(total_arc / arc_length_per_brick);

    // Limit max bricks to prevent overcrowding
    int max_bricks = cols_ + (static_cast<int>(ring) * CIRCLE_COLS_GROWTH_PER_RING);
    bricks_in_ring = std::min(bricks_in_ring, max_bricks);
    // Ensure at least some bricks
    bricks_in_ring = std::max(bricks_in_ring, MIN_BRICKS_PER_RING);

    for (int i = 0; i < bricks_in_ring; ++i)
    {
      // Random brick type
      auto brick_type = static_cast<BrickType>(brick_dist_(rng_));

      if (brick_type == BrickType::Empty)
      {
        continue;
      }

      // Calculate position on circle
      float angle = static_cast<float>(i) * TWO_PI / static_cast<float>(bricks_in_ring);
      // Offset angle by -90 degrees (PI/2) so first brick is at top
      angle -= PI / ANGLE_OFFSET_FACTOR;
      float x = center_x + current_radius * std::cos(angle);
      float y = center_y + current_radius * std::sin(angle);

      // Skip if outside screen bounds or in score/paddle areas
      // Top 15% reserved for score, bottom 25% reserved for paddle
      constexpr float SCORE_AREA_BOTTOM = WINDOW_HEIGHT_F * SCORE_AREA_FACTOR;
      constexpr float PADDLE_AREA_TOP = WINDOW_HEIGHT_F * PADDLE_AREA_FACTOR;
      if (x < config_.brick_width / CENTER_DIVISOR ||
          x > WINDOW_WIDTH_F - config_.brick_width / CENTER_DIVISOR || y < SCORE_AREA_BOTTOM ||
          y > PADDLE_AREA_TOP)
      {
        continue;
      }

      // Determine brick properties
      int hit_points = 0;
      SDL_Texture *texture = nullptr;
      ecs::BrickColor color = ecs::BrickColor::BLUE;

      if (brick_type == BrickType::Red)
      {
        hit_points = 3;
        texture = textures_.brick_red;
        color = ecs::BrickColor::RED;
      }
      else if (brick_type == BrickType::Yellow)
      {
        hit_points = 2;
        texture = textures_.brick_yellow;
        color = ecs::BrickColor::YELLOW;
      }
      else
      {
        hit_points = 1;
        texture = textures_.brick_blue;
        color = ecs::BrickColor::BLUE;
      }

      create_brick(x, y, texture, color, hit_points);
    }
  }
}

void Level::create_brick(float x, float y, SDL_Texture *texture, ecs::BrickColor color,
                         int hit_points)
{
  using namespace ecs;

  auto brick = registry_.create_entity();

  registry_.add_component<TransformComponent>(
      brick, TransformComponent{{x, y}, config_.brick_width, config_.brick_height});

  registry_.add_component<SpriteComponent>(brick, SpriteComponent{texture, DEFAULT_TINT_VEC4});
  registry_.add_component<BrickComponent>(brick, BrickComponent{hit_points, color});
}

} // namespace breakout
