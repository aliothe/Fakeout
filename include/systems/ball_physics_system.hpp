#pragma once

#include "ecs/component.hpp"
#include "ecs/registry.hpp"
#include "game_constants.hpp"
#include "particle_system.hpp"
#include "score_manager.hpp"
#include "sound_manager.hpp"
#include <cmath>
#include <numbers>

namespace breakout::systems
{

class BallPhysicsSystem
{
public:
  BallPhysicsSystem(SoundManager &sound_manager, ParticleSystem &particle_system,
                    ScoreManager &score_manager)
      : sound_manager_(sound_manager), particle_system_(particle_system),
        score_manager_(score_manager)
  {
  }

  void update(ecs::Registry &registry, float delta_time, float screen_width, float screen_height)
  {
    auto balls =
        registry.view<ecs::TransformComponent, ecs::VelocityComponent, ecs::BallComponent>();

    for (ecs::Entity ball : balls)
    {
      auto *transform = registry.get_component<ecs::TransformComponent>(ball);
      auto *velocity = registry.get_component<ecs::VelocityComponent>(ball);

      if (transform == nullptr || velocity == nullptr)
      {
        continue;
      }

      // Move the ball
      transform->position.x += velocity->velocity.x * delta_time;
      transform->position.y += velocity->velocity.y * delta_time;

      // Wall collisions
      handle_wall_collisions(*transform, *velocity, screen_width, screen_height);

      // Paddle collision
      handle_paddle_collision(registry, *transform, *velocity);

      // Brick collision
      handle_brick_collision(registry, *transform, *velocity);
    }
  }

private:
  void handle_wall_collisions(ecs::TransformComponent &transform, ecs::VelocityComponent &velocity,
                              float screen_width, float screen_height)
  {
    const float ball_radius = transform.width / 2.0f;

    // Left wall
    if (transform.position.x - ball_radius < 0.0f)
    {
      transform.position.x = ball_radius;
      velocity.velocity.x = -velocity.velocity.x;
      sound_manager_.play_ping();
    }
    // Right wall
    else if (transform.position.x + ball_radius > screen_width)
    {
      transform.position.x = screen_width - ball_radius;
      velocity.velocity.x = -velocity.velocity.x;
      sound_manager_.play_ping();
    }

    // Top wall
    if (transform.position.y - ball_radius < 0.0f)
    {
      transform.position.y = ball_radius;
      velocity.velocity.y = -velocity.velocity.y;
      sound_manager_.play_ping();
    }

    // Bottom - ball falls off (game over condition could go here)
    if (transform.position.y > screen_height + ball_radius)
    {
      // Reset ball to center for now
      transform.position.x = screen_width / 2.0f;
      transform.position.y = screen_height / 2.0f;
      velocity.velocity.y = -std::abs(velocity.velocity.y);
    }
  }

  void handle_paddle_collision(ecs::Registry &registry, ecs::TransformComponent &ball_transform,
                               ecs::VelocityComponent &ball_velocity)
  {
    auto paddles =
        registry.view<ecs::TransformComponent, ecs::PaddleComponent, ecs::VelocityComponent>();

    for (ecs::Entity paddle : paddles)
    {
      auto *paddle_transform = registry.get_component<ecs::TransformComponent>(paddle);
      auto *paddle_velocity = registry.get_component<ecs::VelocityComponent>(paddle);

      if (paddle_transform == nullptr || paddle_velocity == nullptr)
      {
        continue;
      }

      if (check_collision(ball_transform, *paddle_transform))
      {
        resolve_paddle_collision(ball_transform, ball_velocity, *paddle_transform,
                                 *paddle_velocity);
        sound_manager_.play_ping();
      }
    }
  }

  [[nodiscard]] bool check_collision(const ecs::TransformComponent &ball,
                                     const ecs::TransformComponent &paddle) const
  {
    const float ball_radius = ball.width / 2.0f;
    const float ball_left = ball.position.x - ball_radius;
    const float ball_right = ball.position.x + ball_radius;
    const float ball_top = ball.position.y - ball_radius;
    const float ball_bottom = ball.position.y + ball_radius;

    const float paddle_left = paddle.position.x - paddle.width / 2.0f;
    const float paddle_right = paddle.position.x + paddle.width / 2.0f;
    const float paddle_top = paddle.position.y - paddle.height / 2.0f;
    const float paddle_bottom = paddle.position.y + paddle.height / 2.0f;

    return ball_right > paddle_left && ball_left < paddle_right && ball_bottom > paddle_top &&
           ball_top < paddle_bottom;
  }

  void resolve_paddle_collision(ecs::TransformComponent &ball_transform,
                                ecs::VelocityComponent &ball_velocity,
                                const ecs::TransformComponent &paddle_transform,
                                const ecs::VelocityComponent &paddle_velocity)
  {
    // Calculate where on the paddle the ball hit (-1.0 to 1.0)
    float hit_position =
        (ball_transform.position.x - paddle_transform.position.x) / (paddle_transform.width / 2.0f);
    hit_position = std::max(-1.0f, std::min(1.0f, hit_position)); // Clamp

    // Base bounce speed
    constexpr float BASE_SPEED = 400.0f;

    // Calculate new velocity with angle based on hit position
    // Hit center = bounce straight up, hit edge = bounce at angle
    constexpr float MAX_BOUNCE_ANGLE = 60.0f * std::numbers::pi_v<float> / 180.0f; // 60 degrees in radians
    float bounce_angle = hit_position * MAX_BOUNCE_ANGLE;

    // Set new velocity
    ball_velocity.velocity.x = BASE_SPEED * std::sin(bounce_angle);
    ball_velocity.velocity.y = -BASE_SPEED * std::cos(bounce_angle);

    // Add paddle movement influence ("English" or spin)
    // If paddle is moving, add some of that velocity to the ball
    constexpr float PADDLE_INFLUENCE = 0.5f;
    ball_velocity.velocity.x += paddle_velocity.velocity.x * PADDLE_INFLUENCE;

    // Move ball above paddle to prevent sticking
    ball_transform.position.y =
        paddle_transform.position.y - paddle_transform.height / 2.0f - ball_transform.width / 2.0f;
  }

  void handle_brick_collision(ecs::Registry &registry, ecs::TransformComponent &ball_transform,
                              ecs::VelocityComponent &ball_velocity)
  {
    auto bricks = registry.view<ecs::TransformComponent, ecs::BrickComponent>();

    for (ecs::Entity brick : bricks)
    {
      auto *brick_transform = registry.get_component<ecs::TransformComponent>(brick);
      auto *brick_component = registry.get_component<ecs::BrickComponent>(brick);

      if (brick_transform == nullptr || brick_component == nullptr)
      {
        continue;
      }

      if (check_brick_collision(ball_transform, *brick_transform))
      {
        resolve_brick_collision(ball_transform, ball_velocity, *brick_transform);

        // Reduce hit points
        brick_component->hit_points--;

        // Play sound on hit
        sound_manager_.play_ping();

        // Destroy brick if hit points depleted
        if (brick_component->hit_points <= 0)
        {
          // Add score based on brick type (hit points = score)
          int points = 0;
          switch (brick_component->color)
          {
          case ecs::BrickColor::RED:
            points = 3;
            sound_manager_.play_brick_break_red();
            break;
          case ecs::BrickColor::YELLOW:
            points = 2;
            sound_manager_.play_brick_break_yellow();
            break;
          case ecs::BrickColor::BLUE:
            points = 1;
            sound_manager_.play_brick_break_blue();
            break;
          default:
            points = 1;
            sound_manager_.play_ping();
            break;
          }
          score_manager_.add_score(points);

          // Spawn destruction effects
          particle_system_.spawn_destruction_effects(
              registry, *brick_transform, brick_component->color, ball_transform.position.x,
              ball_transform.position.y);

          registry.destroy_entity(brick);
        }

        // Only handle one brick collision per frame to prevent issues
        break;
      }
    }
  }

  [[nodiscard]] bool check_brick_collision(const ecs::TransformComponent &ball,
                                           const ecs::TransformComponent &brick) const
  {
    const float ball_radius = ball.width / 2.0f;
    const float ball_left = ball.position.x - ball_radius;
    const float ball_right = ball.position.x + ball_radius;
    const float ball_top = ball.position.y - ball_radius;
    const float ball_bottom = ball.position.y + ball_radius;

    const float brick_left = brick.position.x - brick.width / 2.0f;
    const float brick_right = brick.position.x + brick.width / 2.0f;
    const float brick_top = brick.position.y - brick.height / 2.0f;
    const float brick_bottom = brick.position.y + brick.height / 2.0f;

    return ball_right > brick_left && ball_left < brick_right && ball_bottom > brick_top &&
           ball_top < brick_bottom;
  }

  void resolve_brick_collision(ecs::TransformComponent &ball_transform,
                               ecs::VelocityComponent &ball_velocity,
                               const ecs::TransformComponent &brick_transform)
  {
    const float ball_radius = ball_transform.width / 2.0f;
    const float ball_left = ball_transform.position.x - ball_radius;
    const float ball_right = ball_transform.position.x + ball_radius;
    const float ball_top = ball_transform.position.y - ball_radius;
    const float ball_bottom = ball_transform.position.y + ball_radius;

    const float brick_left = brick_transform.position.x - brick_transform.width / 2.0f;
    const float brick_right = brick_transform.position.x + brick_transform.width / 2.0f;
    const float brick_top = brick_transform.position.y - brick_transform.height / 2.0f;
    const float brick_bottom = brick_transform.position.y + brick_transform.height / 2.0f;

    // Determine which side was hit based on previous position
    // Simple approach: check overlap amounts
    float overlap_left = ball_right - brick_left;
    float overlap_right = brick_right - ball_left;
    float overlap_top = ball_bottom - brick_top;
    float overlap_bottom = brick_bottom - ball_top;

    // Find smallest overlap to determine bounce direction
    float min_overlap = std::min({overlap_left, overlap_right, overlap_top, overlap_bottom});

    // Use tolerance-based comparison for floating point
    if (std::abs(min_overlap - overlap_left) < EPSILON ||
        std::abs(min_overlap - overlap_right) < EPSILON)
    {
      // Hit from left or right
      ball_velocity.velocity.x = -ball_velocity.velocity.x;
    }
    else
    {
      // Hit from top or bottom
      ball_velocity.velocity.y = -ball_velocity.velocity.y;
    }
  }

  SoundManager &sound_manager_;
  ParticleSystem &particle_system_;
  ScoreManager &score_manager_;
};

} // namespace breakout::systems