#include "ecs/component.hpp"
#include "ecs/registry.hpp"
#include "game_constants.hpp"
#include "sdl_window.hpp"
#include "sound_manager.hpp"
#include "systems/ball_physics_system.hpp"
#include "systems/input_system.hpp"
#include "systems/movement_system.hpp"
#include "systems/particle_system.hpp"
#include "systems/render_system.hpp"
#include "texture_manager.hpp"
#include <SDL3/SDL.h>
#include <array>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <random>

namespace
{
using breakout::BrickType;
using breakout::ecs::BallComponent;
using breakout::ecs::BrickColor;
using breakout::ecs::BrickComponent;
using breakout::ecs::Entity;
using breakout::ecs::PaddleComponent;
using breakout::ecs::PlayerControllerComponent;
using breakout::ecs::Registry;
using breakout::ecs::SpriteComponent;
using breakout::ecs::TransformComponent;
using breakout::ecs::VelocityComponent;

Entity create_paddle(Registry &registry, breakout::TextureManager &texture_manager)
{
  auto paddle = registry.create_entity();

  SDL_Texture *texture = texture_manager.create_paddle_texture(breakout::PADDLE_TEXTURE_WIDTH,
                                                               breakout::PADDLE_TEXTURE_HEIGHT);

  registry.add_component<TransformComponent>(
      paddle, TransformComponent{{breakout::PADDLE_CENTER_X, breakout::PADDLE_CENTER_Y},
                                 breakout::PADDLE_WIDTH,
                                 breakout::PADDLE_HEIGHT});

  registry.add_component<SpriteComponent>(paddle,
                                          SpriteComponent{texture, breakout::DEFAULT_TINT_VEC4});

  registry.add_component<VelocityComponent>(
      paddle, VelocityComponent{{breakout::ZERO_VELOCITY[0], breakout::ZERO_VELOCITY[1]}});

  registry.add_component<PaddleComponent>(paddle, PaddleComponent{});
  registry.add_component<PlayerControllerComponent>(paddle, PlayerControllerComponent{});

  return paddle;
}

Entity create_ball(Registry &registry, breakout::TextureManager &texture_manager)
{
  auto ball = registry.create_entity();

  SDL_Texture *texture =
      texture_manager.create_ball_texture(breakout::BALL_TEXTURE_SIZE, breakout::BALL_TEXTURE_SIZE);

  // Start ball in center, moving down at an angle
  registry.add_component<TransformComponent>(
      ball, TransformComponent{{breakout::BALL_CENTER_X, breakout::BALL_CENTER_Y},
                               breakout::BALL_SIZE,
                               breakout::BALL_SIZE});

  registry.add_component<SpriteComponent>(ball,
                                          SpriteComponent{texture, breakout::DEFAULT_TINT_VEC4});

  // Initial velocity: moving down and to the right
  registry.add_component<VelocityComponent>(
      ball,
      VelocityComponent{{breakout::BALL_INITIAL_VELOCITY_X, breakout::BALL_INITIAL_VELOCITY_Y}});

  registry.add_component<BallComponent>(ball, BallComponent{});

  return ball;
}

void create_brick_wall(Registry &registry, breakout::TextureManager &texture_manager)
{
  // Pre-generate textures for each brick type
  SDL_Texture *red_texture = texture_manager.create_brick_texture(
      breakout::BRICK_TEXTURE_WIDTH, breakout::BRICK_TEXTURE_HEIGHT, breakout::BRICK_RED_R,
      breakout::BRICK_RED_G, breakout::BRICK_RED_B);
  SDL_Texture *yellow_texture = texture_manager.create_brick_texture(
      breakout::BRICK_TEXTURE_WIDTH, breakout::BRICK_TEXTURE_HEIGHT, breakout::BRICK_YELLOW_R,
      breakout::BRICK_YELLOW_G, breakout::BRICK_YELLOW_B);
  SDL_Texture *blue_texture = texture_manager.create_brick_texture(
      breakout::BRICK_TEXTURE_WIDTH, breakout::BRICK_TEXTURE_HEIGHT, breakout::BRICK_BLUE_R,
      breakout::BRICK_BLUE_G, breakout::BRICK_BLUE_B);

  // Calculate total width and starting X position to center the wall
  float total_width = breakout::BRICK_COLS_F * breakout::BRICK_WIDTH;
  float start_x = (breakout::WINDOW_WIDTH_F - total_width) / breakout::CENTER_DIVISOR;

  // Random number generation with weighted distribution using std::discrete_distribution
  // Weights: Empty=4, Red=1, Yellow=2, Blue=3
  // Probabilities: Empty=40%, Red=10%, Yellow=20%, Blue=30%
  std::random_device rd;
  std::mt19937 rng(rd());
  std::discrete_distribution<int> brick_dist{
      breakout::BRICK_WEIGHT_EMPTY, breakout::BRICK_WEIGHT_RED, breakout::BRICK_WEIGHT_YELLOW,
      breakout::BRICK_WEIGHT_BLUE};

  for (int row = 0; row < breakout::BRICK_ROWS; ++row)
  {
    for (int col = 0; col < breakout::BRICK_COLS; ++col)
    {
      // Randomly decide brick type using weighted distribution
      auto brick_type = static_cast<BrickType>(brick_dist(rng));

      // Empty cell check (40% probability), skip creating a brick
      if (brick_type == BrickType::Empty)
      {
        continue;
      }

      auto brick = registry.create_entity();

      float x = start_x + static_cast<float>(col) * breakout::BRICK_WIDTH;
      float y = breakout::BRICK_WALL_TOP_OFFSET + static_cast<float>(row) * breakout::BRICK_HEIGHT;

      registry.add_component<TransformComponent>(
          brick, TransformComponent{{x, y}, breakout::BRICK_WIDTH, breakout::BRICK_HEIGHT});

      // Determine brick properties based on weighted random selection
      int hit_points = 0;
      SDL_Texture *texture = nullptr;
      BrickColor color = BrickColor::BLUE;

      if (brick_type == BrickType::Red)
      {
        // Red bricks (3 hits) - 10% probability
        hit_points = 3;
        texture = red_texture;
        color = BrickColor::RED;
      }
      else if (brick_type == BrickType::Yellow)
      {
        // Yellow bricks (2 hits) - 20% probability
        hit_points = 2;
        texture = yellow_texture;
        color = BrickColor::YELLOW;
      }
      else
      {
        // Blue bricks (1 hit) - 30% probability
        hit_points = 1;
        texture = blue_texture;
        color = BrickColor::BLUE;
      }

      registry.add_component<SpriteComponent>(
          brick, SpriteComponent{texture, breakout::DEFAULT_TINT_VEC4});
      registry.add_component<BrickComponent>(brick, BrickComponent{hit_points, color});
    }
  }
}
} // namespace

int main()
{
  try
  {
    // Initialize SDL with video and audio subsystems
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO))
    {
      const char *error = SDL_GetError();
      throw std::runtime_error(std::string("Failed to initialize SDL: ") +
                               (error != nullptr ? error : "unknown error"));
    }

    SDLWindow window("Breakout - Move paddle to bounce ball", breakout::WINDOW_WIDTH,
                     breakout::WINDOW_HEIGHT);

    SDL_Renderer *renderer = window.get_renderer();
    if (renderer == nullptr)
    {
      throw std::runtime_error("Failed to obtain SDL renderer: " + std::string(SDL_GetError()));
    }

    // Enable vsync for smooth rendering
    if (!SDL_SetRenderVSync(renderer, 1))
    {
      // VSync not supported or failed, continue without it
      std::cerr << "Warning: Failed to enable vsync: " << SDL_GetError() << '\n';
    }

    Registry registry;
    breakout::TextureManager texture_manager(renderer);
    breakout::SoundManager sound_manager;
    breakout::systems::ParticleSystem particle_system;
    breakout::systems::BallPhysicsSystem ball_physics_system(sound_manager, particle_system);
    breakout::systems::InputSystem input_system;
    breakout::systems::MovementSystem movement_system;
    breakout::systems::RenderSystem render_system(renderer);

    create_paddle(registry, texture_manager);
    create_ball(registry, texture_manager);
    create_brick_wall(registry, texture_manager);

    auto last_time = std::chrono::steady_clock::now();

    while (!window.should_close())
    {
      auto current_time = std::chrono::steady_clock::now();
      float delta_time = std::chrono::duration<float>(current_time - last_time).count();
      last_time = current_time;

      window.handle_events();
      input_system.update(registry);
      movement_system.update(registry, delta_time, breakout::WINDOW_WIDTH_F);
      ball_physics_system.update(registry, delta_time, breakout::WINDOW_WIDTH_F,
                                 breakout::WINDOW_HEIGHT_F);
      particle_system.update(registry, delta_time);

      // Clear screen with background color
      SDL_SetRenderDrawColor(renderer, breakout::CLEAR_COLOR_SDL.r, breakout::CLEAR_COLOR_SDL.g,
                             breakout::CLEAR_COLOR_SDL.b, breakout::CLEAR_COLOR_SDL.a);
      SDL_RenderClear(renderer);

      render_system.render(registry);

      window.present();
    }

    std::cout << "Window closed successfully.\n";
    SDL_Quit();
    return EXIT_SUCCESS;
  }
  catch (const std::exception &e)
  {
    std::cerr << "Error: " << e.what() << '\n';
    SDL_Quit();
    return EXIT_FAILURE;
  }
}
