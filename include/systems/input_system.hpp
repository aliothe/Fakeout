#pragma once

#include "ecs/component.hpp"
#include "ecs/registry.hpp"
#include "game_constants.hpp"
#include <SDL3/SDL.h>

namespace breakout::systems
{

class InputSystem
{
public:
    void update(ecs::Registry &registry, float delta_time) const
    {
        SDL_PumpEvents();
        const bool *keyboard_state = SDL_GetKeyboardState(nullptr);

        // Tune these values
        constexpr float MAX_PADDLE_SPEED   = 700.0f;    // pixels per second
        constexpr float ACCELERATION       = 3200.0f;   // how fast we reach max speed
        constexpr float DECELERATION       = 3800.0f;   // how fast we stop
        constexpr float INSTANT_STOP_THRESHOLD = 0.05f; // small velocity → snap to 0

        auto entities = registry.view<ecs::PlayerControllerComponent,
                                      ecs::VelocityComponent>();  // we need position for bounds later

        for (ecs::Entity entity : entities)
        {
            auto *velocity  = registry.get_component<ecs::VelocityComponent>(entity);

            if (velocity == nullptr) continue;

            bool left_pressed  = keyboard_state[SDL_SCANCODE_LEFT]  || keyboard_state[SDL_SCANCODE_A];
            bool right_pressed = keyboard_state[SDL_SCANCODE_RIGHT] || keyboard_state[SDL_SCANCODE_D];

            float target_velocity_x = 0.0f;

            if (left_pressed && !right_pressed)
            {
                target_velocity_x = -MAX_PADDLE_SPEED;
            }
            else if (right_pressed && !left_pressed)
            {
                target_velocity_x = +MAX_PADDLE_SPEED;
            }
            // else: both or none → target = 0

            // If opposite direction is pressed → snap to zero first (instant stop/reverse)
            if ((velocity->velocity.x > 0 && left_pressed) ||
                (velocity->velocity.x < 0 && right_pressed))
            {
                velocity->velocity.x = 0.0f;
            }

            // Apply acceleration / deceleration
            float accel = (target_velocity_x > 0) ? ACCELERATION : -ACCELERATION;
            if (std::abs(target_velocity_x) <= EPSILON)
            {
                accel = (velocity->velocity.x > 0) ? -DECELERATION : DECELERATION;
            }

            velocity->velocity.x += accel * delta_time;

            // Clamp to max speed
            if (velocity->velocity.x > MAX_PADDLE_SPEED)
                velocity->velocity.x = MAX_PADDLE_SPEED;
            else if (velocity->velocity.x < -MAX_PADDLE_SPEED)
                velocity->velocity.x = -MAX_PADDLE_SPEED;

            // Snap to zero when very slow (prevents micro-oscillation)
            if (std::abs(velocity->velocity.x) < INSTANT_STOP_THRESHOLD)
            {
                velocity->velocity.x = 0.0f;
            }
        }
    }
};

} // namespace breakout::systems

