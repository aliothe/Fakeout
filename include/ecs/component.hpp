#pragma once

#include <SDL3/SDL.h>
#include <array>
#include <cstdint>

namespace breakout::ecs
{

using Entity = std::uint32_t;
constexpr Entity MAX_ENTITIES = 10000;
constexpr Entity INVALID_ENTITY = static_cast<Entity>(-1);

struct Vec2
{
  float x;
  float y;
};

struct Vec4
{
  float r;
  float g;
  float b;
  float a;
};

struct TransformComponent
{
  Vec2 position;
  float width;
  float height;
};

struct SpriteComponent
{
  SDL_Texture *texture;
  Vec4 tint;
};

struct VelocityComponent
{
  Vec2 velocity;
};

struct PaddleComponent
{
};

struct PlayerControllerComponent
{
};

struct BallComponent
{
};

enum class BrickColor
{
  RED,
  YELLOW,
  BLUE
};

struct BrickComponent
{
  int hit_points;
  BrickColor color;
};

// Particle system components for destruction effects
struct ParticleComponent
{
  float lifetime;     // Total lifetime in seconds
  float max_lifetime; // Initial lifetime for fade calculation
  float size;         // Particle size
};

struct ShardComponent
{
  float lifetime;
  float max_lifetime;
  float rotation;       // Current rotation angle
  float rotation_speed; // Rotation velocity
  float scale;          // Size scale (0.2-0.5)
};

struct LifetimeComponent
{
  float remaining;    // Time remaining
  float max_lifetime; // For calculating fade
};

} // namespace breakout::ecs