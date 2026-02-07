#pragma once

#include "ecs/component.hpp"
#include "ecs/registry.hpp"

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

      const float half_width = transform->width / 2.0f;
      const float min_x = half_width;
      const float max_x = screen_width - half_width;

      if (transform->position.x < min_x)
      {
        transform->position.x = min_x;
      }
      else if (transform->position.x > max_x)
      {
        transform->position.x = max_x;
      }
    }
  }
};

} // namespace breakout::systems