#pragma once

#include "ecs/component.hpp"
#include "ecs/registry.hpp"
#include <SDL3/SDL.h>
#include <string>

namespace breakout
{

class ScoreManager
{
public:
  void add_score(int points)
  {
    current_score_ += points * 10;
    if (current_score_ > high_score_)
    {
      high_score_ = current_score_;
    }
  }

  [[nodiscard]] int current_score() const
  {
    return current_score_;
  }
  [[nodiscard]] int high_score() const
  {
    return high_score_;
  }
  void reset()
  {
    current_score_ = 0;
  }

  // Check if all bricks are destroyed
  [[nodiscard]] bool is_level_complete(ecs::Registry &registry) const
  {
    auto bricks = registry.view<ecs::BrickComponent>();
    return bricks.empty();
  }

private:
  int current_score_{0};
  int high_score_{0};
};

} // namespace breakout
