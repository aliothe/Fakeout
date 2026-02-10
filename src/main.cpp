#include "ecs/component.hpp"
#include "ecs/registry.hpp"
#include "game_constants.hpp"
#include "level.hpp"
#include "score_manager.hpp"
#include "sdl_window.hpp"
#include "sound_manager.hpp"
#include "systems/ball_physics_system.hpp"
#include "systems/input_system.hpp"
#include "systems/movement_system.hpp"
#include "systems/particle_system.hpp"
#include "systems/render_system.hpp"
#include "systems/score_render_system.hpp"
#include "texture_manager.hpp"
#include <SDL3/SDL.h>
#include <array>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <random>

namespace
{
using breakout::GameTextures;
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

Entity create_paddle(Registry &registry, SDL_Texture *texture)
{
  auto paddle = registry.create_entity();

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

Entity create_ball(Registry &registry, SDL_Texture *texture)
{
  auto ball = registry.create_entity();

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
} // namespace

int main()
{
  try
  {
    // SDL RAII wrapper - initialized first, destroyed last
    // This ensures SDL_Quit() is called AFTER all SDL resources are destroyed
    SDL sdl{SDL_INIT_VIDEO | SDL_INIT_AUDIO};

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

    // Create all game textures once at startup
    // Pixel data is generated at compile time via constexpr functions
    breakout::TextureManager texture_manager(renderer);
    GameTextures textures{
        texture_manager.create_paddle_texture<breakout::PADDLE_TEXTURE_WIDTH,
                                              breakout::PADDLE_TEXTURE_HEIGHT>(),
        texture_manager
            .create_ball_texture<breakout::BALL_TEXTURE_SIZE, breakout::BALL_TEXTURE_SIZE>(),
        texture_manager.create_brick_texture<breakout::BRICK_TEXTURE_WIDTH,
                                             breakout::BRICK_TEXTURE_HEIGHT, breakout::BRICK_RED_R,
                                             breakout::BRICK_RED_G, breakout::BRICK_RED_B>(),
        texture_manager.create_brick_texture<
            breakout::BRICK_TEXTURE_WIDTH, breakout::BRICK_TEXTURE_HEIGHT, breakout::BRICK_YELLOW_R,
            breakout::BRICK_YELLOW_G, breakout::BRICK_YELLOW_B>(),
        texture_manager.create_brick_texture<breakout::BRICK_TEXTURE_WIDTH,
                                             breakout::BRICK_TEXTURE_HEIGHT, breakout::BRICK_BLUE_R,
                                             breakout::BRICK_BLUE_G, breakout::BRICK_BLUE_B>()};

    Registry registry;
    breakout::SoundManager sound_manager;
    breakout::ScoreManager score_manager;
    breakout::systems::ParticleSystem particle_system;
    breakout::systems::BallPhysicsSystem ball_physics_system(sound_manager, particle_system,
                                                             score_manager);
    breakout::systems::InputSystem input_system;
    breakout::systems::MovementSystem movement_system;
    breakout::systems::RenderSystem render_system(renderer);
    breakout::systems::ScoreRenderSystem score_render_system(renderer);

    // Create game entities - textures are passed in, not recreated
    create_paddle(registry, textures.paddle);
    create_ball(registry, textures.ball);

    // Create level with randomized layout
    breakout::Level level(registry, textures);
    level.generate();

    auto last_time = std::chrono::steady_clock::now();

    while (!window.should_close())
    {
      auto current_time = std::chrono::steady_clock::now();
      float delta_time = std::chrono::duration<float>(current_time - last_time).count();
      last_time = current_time;

      window.handle_events();
      input_system.update(registry, delta_time);
      movement_system.update(registry, delta_time, breakout::WINDOW_WIDTH_F);
      ball_physics_system.update(registry, delta_time, breakout::WINDOW_WIDTH_F,
                                 breakout::WINDOW_HEIGHT_F);
      particle_system.update(registry, delta_time);

      // Check for level completion
      if (score_manager.is_level_complete(registry))
      {
        // Generate new level
        level.generate();
        std::cout << "New level generated! Score: " << score_manager.current_score() << "\n";
      }

      // Clear screen with background color
      SDL_SetRenderDrawColor(renderer, breakout::CLEAR_COLOR_SDL.r, breakout::CLEAR_COLOR_SDL.g,
                             breakout::CLEAR_COLOR_SDL.b, breakout::CLEAR_COLOR_SDL.a);
      SDL_RenderClear(renderer);

      render_system.render(registry);

      // Render score on top
      score_render_system.render(score_manager);

      window.present();
    }

    std::cout << "Window closed successfully. Final score: " << score_manager.current_score()
              << "\n";
    return EXIT_SUCCESS;
  }
  catch (const std::exception &e)
  {
    std::cerr << "Error: " << e.what() << '\n';
    return EXIT_FAILURE;
  }
}
