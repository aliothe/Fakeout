#pragma once

#include "ecs/component.hpp"
#include "ecs/registry.hpp"
#include <algorithm>

namespace breakout::systems
{

class MovementSystem
{
public:
  void update(ecs::Registry &registry, float delta_time, float screen_width) const
  {
    auto entities =
        registry.view<ecs::TransformComponent, ecs::VelocityComponent, ecs::PaddleComponent>();

    for (ecs::Entity entity : entities)
    {
      auto *transform = registry.get_component<ecs::TransformComponent>(entity);
      auto *velocity = registry.get_component<ecs::VelocityComponent>(entity);

      if (transform == nullptr || velocity == nullptr)
      {
        continue;
      }

      transform->position.x += velocity->velocity.x * delta_time;

      // Keep paddle inside screen
      const float half = transform->width / 2.0f;
      transform->position.x = std::clamp(
          transform->position.x,
          half,
          screen_width - half
      );
    }
  }
};

} // namespace breakout::systems