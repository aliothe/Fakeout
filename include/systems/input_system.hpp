#pragma once

#include "ecs/component.hpp"
#include "ecs/registry.hpp"
#include <SDL3/SDL.h>

namespace breakout::systems
{

class InputSystem
{
public:
  void update(ecs::Registry &registry) const
  {
    SDL_PumpEvents();
    const bool *keyboard_state = SDL_GetKeyboardState(nullptr);

    auto entities = registry.view<ecs::PlayerControllerComponent, ecs::VelocityComponent>();

    constexpr float PADDLE_SPEED = 500.0f; // pixels per second

    for (ecs::Entity entity : entities)
    {
      auto *velocity = registry.get_component<ecs::VelocityComponent>(entity);
      if (velocity == nullptr)
      {
        continue;
      }

      float x_velocity = 0.0f;

      if (keyboard_state[SDL_SCANCODE_LEFT] || keyboard_state[SDL_SCANCODE_A])
      {
        x_velocity = -PADDLE_SPEED;
      }
      else if (keyboard_state[SDL_SCANCODE_RIGHT] || keyboard_state[SDL_SCANCODE_D])
      {
        x_velocity = PADDLE_SPEED;
      }
      else
      {
        x_velocity = 0.0f;
      }

      velocity->velocity.x = x_velocity;
    }
  }
};

} // namespace breakout::systems