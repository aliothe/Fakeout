#pragma once

#include "ecs/component.hpp"
#include "ecs/registry.hpp"
#include "game_constants.hpp"
#include <cmath>
#include <numbers>
#include <random>

namespace breakout::systems
{

class ParticleSystem
{
public:
  ParticleSystem() : rng_(std::random_device{}())
  {
  }

  // Spawn destruction effects for a brick
  void spawn_destruction_effects(ecs::Registry &registry,
                                 const ecs::TransformComponent &brick_transform,
                                 ecs::BrickColor color, float impact_x, float impact_y)
  {
    spawn_shards(registry, brick_transform, color);
    spawn_particles(registry, brick_transform, color, impact_x, impact_y);
  }

  void update(ecs::Registry &registry, float delta_time)
  {
    update_shards(registry, delta_time);
    update_particles(registry, delta_time);
  }

private:
  void spawn_shards(ecs::Registry &registry, const ecs::TransformComponent &brick_transform,
                    ecs::BrickColor color)
  {
    std::uniform_int_distribution<int> shard_count_dist(6, 12);
    std::uniform_real_distribution<float> scale_dist(0.2f, 0.5f);
    std::uniform_real_distribution<float> angle_dist(0.0f, 2.0f * std::numbers::pi_v<float>);
    std::uniform_real_distribution<float> speed_dist(50.0f, 150.0f);
    std::uniform_real_distribution<float> rot_speed_dist(-5.0f, 5.0f);

    int num_shards = shard_count_dist(rng_);

    // Get brick color values
    float r, g, b;
    get_color_values(color, r, g, b);

    for (int i = 0; i < num_shards; ++i)
    {
      auto shard = registry.create_entity();
      if (shard == ecs::INVALID_ENTITY)
      {
        return;
      }

      // Random position near brick center
      std::uniform_real_distribution<float> offset_dist(-1.0f, 1.0f);
      float offset_x = offset_dist(rng_) * brick_transform.width * 0.3f;
      float offset_y = offset_dist(rng_) * brick_transform.height * 0.3f;

      float scale = scale_dist(rng_);
      float angle = angle_dist(rng_);
      float speed = speed_dist(rng_);

      ecs::Vec2 position{brick_transform.position.x + offset_x,
                         brick_transform.position.y + offset_y};

      ecs::Vec2 velocity{std::cos(angle) * speed, std::sin(angle) * speed};

      float shard_width = brick_transform.width * scale;
      float shard_height = brick_transform.height * scale;

      registry.add_component<ecs::TransformComponent>(
          shard, ecs::TransformComponent{position, shard_width, shard_height});

      registry.add_component<ecs::VelocityComponent>(shard, ecs::VelocityComponent{velocity});

      constexpr float SHARD_LIFETIME = 0.4f;
      registry.add_component<ecs::ShardComponent>(
          shard,
          ecs::ShardComponent{SHARD_LIFETIME, SHARD_LIFETIME, 0.0f, rot_speed_dist(rng_), scale});

      // Add sprite with brick color - no texture, just colored rectangle
      registry.add_component<ecs::SpriteComponent>(shard, ecs::SpriteComponent{nullptr, {r, g, b, 1.0f}});
    }
  }

  void spawn_particles(ecs::Registry &registry, const ecs::TransformComponent &brick_transform,
                       ecs::BrickColor color, float impact_x, float impact_y)
  {
    std::uniform_int_distribution<int> particle_count_dist(20, 50);
    std::uniform_real_distribution<float> angle_spread_dist(-0.5f, 0.5f); // Cone spread
    std::uniform_real_distribution<float> speed_dist(100.0f, 250.0f);
    std::uniform_real_distribution<float> size_dist(1.0f, 3.0f);

    int num_particles = particle_count_dist(rng_);

    // Get brick color values
    float r, g, b;
    get_color_values(color, r, g, b);

    // Calculate impact direction
    float dx = brick_transform.position.x - impact_x;
    float dy = brick_transform.position.y - impact_y;
    float impact_angle = std::atan2(dy, dx);

    for (int i = 0; i < num_particles; ++i)
    {
      auto particle = registry.create_entity();
      if (particle == ecs::INVALID_ENTITY)
      {
        return;
      }

      // Cone emission toward impact direction
      float angle = impact_angle + angle_spread_dist(rng_);
      float speed = speed_dist(rng_);

      ecs::Vec2 position{brick_transform.position.x, brick_transform.position.y};

      ecs::Vec2 velocity{std::cos(angle) * speed, std::sin(angle) * speed};

      float size = size_dist(rng_);

      registry.add_component<ecs::TransformComponent>(
          particle, ecs::TransformComponent{position, size, size});

      registry.add_component<ecs::VelocityComponent>(particle, ecs::VelocityComponent{velocity});

      constexpr float PARTICLE_LIFETIME = 0.5f;
      registry.add_component<ecs::ParticleComponent>(
          particle, ecs::ParticleComponent{PARTICLE_LIFETIME, PARTICLE_LIFETIME, size});

      // Add sprite with brick color
      registry.add_component<ecs::SpriteComponent>(particle,
                                                   ecs::SpriteComponent{nullptr, {r, g, b, 1.0f}});
    }
  }

  void update_shards(ecs::Registry &registry, float delta_time)
  {
    auto shards = registry.view<ecs::TransformComponent, ecs::VelocityComponent,
                                ecs::ShardComponent, ecs::SpriteComponent>();

    for (ecs::Entity shard : shards)
    {
      auto *transform = registry.get_component<ecs::TransformComponent>(shard);
      auto *velocity = registry.get_component<ecs::VelocityComponent>(shard);
      auto *shard_comp = registry.get_component<ecs::ShardComponent>(shard);
      auto *sprite = registry.get_component<ecs::SpriteComponent>(shard);

      if (!transform || !velocity || !shard_comp || !sprite)
      {
        continue;
      }

      // Update position
      transform->position.x += velocity->velocity.x * delta_time;
      transform->position.y += velocity->velocity.y * delta_time;

      // Apply gravity
      velocity->velocity.y += 200.0f * delta_time;

      // Update rotation
      shard_comp->rotation += shard_comp->rotation_speed * delta_time;

      // Update lifetime and fade
      shard_comp->lifetime -= delta_time;
      float life_ratio = shard_comp->lifetime / shard_comp->max_lifetime;
      sprite->tint.a = std::max(0.0f, life_ratio);

      // Destroy if lifetime expired
      if (shard_comp->lifetime <= 0.0f)
      {
        registry.destroy_entity(shard);
      }
    }
  }

  void update_particles(ecs::Registry &registry, float delta_time)
  {
    auto particles = registry.view<ecs::TransformComponent, ecs::VelocityComponent,
                                   ecs::ParticleComponent, ecs::SpriteComponent>();

    for (ecs::Entity particle : particles)
    {
      auto *transform = registry.get_component<ecs::TransformComponent>(particle);
      auto *velocity = registry.get_component<ecs::VelocityComponent>(particle);
      auto *particle_comp = registry.get_component<ecs::ParticleComponent>(particle);
      auto *sprite = registry.get_component<ecs::SpriteComponent>(particle);

      if (!transform || !velocity || !particle_comp || !sprite)
      {
        continue;
      }

      // Update position
      transform->position.x += velocity->velocity.x * delta_time;
      transform->position.y += velocity->velocity.y * delta_time;

      // Apply drag
      velocity->velocity.x *= 0.98f;
      velocity->velocity.y *= 0.98f;

      // Update lifetime and fade
      particle_comp->lifetime -= delta_time;
      float life_ratio = particle_comp->lifetime / particle_comp->max_lifetime;
      sprite->tint.a = std::max(0.0f, life_ratio);

      // Destroy if lifetime expired
      if (particle_comp->lifetime <= 0.0f)
      {
        registry.destroy_entity(particle);
      }
    }
  }

  static void get_color_values(ecs::BrickColor color, float &r, float &g, float &b)
  {
    switch (color)
    {
    case ecs::BrickColor::RED:
      r = PARTICLE_RED_R;
      g = PARTICLE_RED_G;
      b = PARTICLE_RED_B;
      break;
    case ecs::BrickColor::YELLOW:
      r = PARTICLE_YELLOW_R;
      g = PARTICLE_YELLOW_G;
      b = PARTICLE_YELLOW_B;
      break;
    case ecs::BrickColor::BLUE:
      r = PARTICLE_BLUE_R;
      g = PARTICLE_BLUE_G;
      b = PARTICLE_BLUE_B;
      break;
    default:
      r = g = b = 1.0f;
    }
  }

  std::mt19937 rng_;
};

} // namespace breakout::systems