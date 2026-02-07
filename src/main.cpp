#include "ecs/component.hpp"
#include "ecs/registry.hpp"
#include "sdl_window.hpp"
#include "sound_manager.hpp"
#include "systems/ball_physics_system.hpp"
#include "systems/input_system.hpp"
#include "systems/movement_system.hpp"
#include "systems/particle_system.hpp"
#include "systems/render_system.hpp"
#include "texture_manager.hpp"
#include <SDL3/SDL_opengl.h>
#include <array>
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <random>

namespace
{
constexpr int WINDOW_WIDTH = 800;
constexpr int WINDOW_HEIGHT = 600;
constexpr std::array<float, 4> CLEAR_COLOR{0.1F, 0.2F, 0.3F, 1.0F};
constexpr float PADDLE_WIDTH = 100.0F;
constexpr float PADDLE_HEIGHT = 20.0F;
constexpr float PADDLE_Y_OFFSET = 50.0F;
constexpr int TEXTURE_WIDTH = 100;
constexpr int TEXTURE_HEIGHT = 20;
constexpr float CENTER_FACTOR = 2.0F;

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

  auto paddle_texture = texture_manager.create_paddle_texture(TEXTURE_WIDTH, TEXTURE_HEIGHT);
  GLuint texture_id = paddle_texture.id();

  static std::vector<breakout::Texture> textures;
  textures.push_back(std::move(paddle_texture));

  registry.add_component<TransformComponent>(
      paddle, TransformComponent{{WINDOW_WIDTH / CENTER_FACTOR,
                                  WINDOW_HEIGHT - PADDLE_Y_OFFSET - PADDLE_HEIGHT / CENTER_FACTOR},
                                 PADDLE_WIDTH,
                                 PADDLE_HEIGHT});

  registry.add_component<SpriteComponent>(paddle,
                                          SpriteComponent{texture_id, {1.0F, 1.0F, 1.0F, 1.0F}});

  registry.add_component<VelocityComponent>(paddle, VelocityComponent{{0.0F, 0.0F}});

  registry.add_component<PaddleComponent>(paddle, PaddleComponent{});
  registry.add_component<PlayerControllerComponent>(paddle, PlayerControllerComponent{});

  return paddle;
}

constexpr float BALL_SIZE = 16.0F;
constexpr int BALL_TEXTURE_SIZE = 16;
constexpr float BALL_INITIAL_VELOCITY_X = 200.0F;
constexpr float BALL_INITIAL_VELOCITY_Y = 300.0F;

Entity create_ball(Registry &registry, breakout::TextureManager &texture_manager)
{
  auto ball = registry.create_entity();

  auto ball_texture = texture_manager.create_ball_texture(BALL_TEXTURE_SIZE, BALL_TEXTURE_SIZE);
  GLuint texture_id = ball_texture.id();

  static std::vector<breakout::Texture> textures;
  textures.push_back(std::move(ball_texture));

  // Start ball in center, moving down at an angle
  registry.add_component<TransformComponent>(
      ball, TransformComponent{{WINDOW_WIDTH / CENTER_FACTOR, WINDOW_HEIGHT / CENTER_FACTOR},
                               BALL_SIZE,
                               BALL_SIZE});

  registry.add_component<SpriteComponent>(ball,
                                          SpriteComponent{texture_id, {1.0F, 1.0F, 1.0F, 1.0F}});

  // Initial velocity: moving down and to the right
  registry.add_component<VelocityComponent>(
      ball, VelocityComponent{{BALL_INITIAL_VELOCITY_X, BALL_INITIAL_VELOCITY_Y}});

  registry.add_component<BallComponent>(ball, BallComponent{});

  return ball;
}

constexpr float BRICK_WIDTH = 60.0F;
constexpr float BRICK_HEIGHT = 20.0F;
constexpr int BRICK_TEXTURE_WIDTH = 60;
constexpr int BRICK_TEXTURE_HEIGHT = 20;
constexpr int BRICK_ROWS = 6;
constexpr int BRICK_COLS = 10;
constexpr float BRICK_WALL_TOP_OFFSET = 60.0F;

// Brick colors (RGB values)
constexpr std::uint8_t RED_R = 220;
constexpr std::uint8_t RED_G = 50;
constexpr std::uint8_t RED_B = 50;
constexpr std::uint8_t YELLOW_R = 220;
constexpr std::uint8_t YELLOW_G = 200;
constexpr std::uint8_t YELLOW_B = 50;
constexpr std::uint8_t BLUE_R = 50;
constexpr std::uint8_t BLUE_G = 100;
constexpr std::uint8_t BLUE_B = 220;

// Brick distribution weights (total = 10)
constexpr int BRICK_DIST_MAX = 9;
constexpr int BRICK_EMPTY_THRESHOLD = 4; // 0-3: empty (40%)
constexpr int BRICK_RED_VALUE = 4;       // 4: red (10%)
constexpr int BRICK_YELLOW_MIN = 5;      // 5-6: yellow (20%)
constexpr int BRICK_YELLOW_MAX = 7;      // 7-9: blue (30%)

void create_brick_wall(Registry &registry, breakout::TextureManager &texture_manager)
{
  // Pre-generate textures for each brick type
  auto red_texture = texture_manager.create_brick_texture(BRICK_TEXTURE_WIDTH, BRICK_TEXTURE_HEIGHT,
                                                          RED_R, RED_G, RED_B);
  auto yellow_texture = texture_manager.create_brick_texture(
      BRICK_TEXTURE_WIDTH, BRICK_TEXTURE_HEIGHT, YELLOW_R, YELLOW_G, YELLOW_B);
  auto blue_texture = texture_manager.create_brick_texture(
      BRICK_TEXTURE_WIDTH, BRICK_TEXTURE_HEIGHT, BLUE_R, BLUE_G, BLUE_B);

  static std::vector<breakout::Texture> textures;
  textures.push_back(std::move(red_texture));
  textures.push_back(std::move(yellow_texture));
  textures.push_back(std::move(blue_texture));

  GLuint red_id = textures[0].id();
  GLuint yellow_id = textures[1].id();
  GLuint blue_id = textures[2].id();

  // Calculate total width and starting X position to center the wall
  float total_width = static_cast<float>(BRICK_COLS) * BRICK_WIDTH;
  float start_x = (WINDOW_WIDTH - total_width) / CENTER_FACTOR + BRICK_WIDTH / CENTER_FACTOR;

  // Random number generation with weighted distribution
  // Weights: Empty=4, Red=1, Yellow=2, Blue=3 (total=10)
  // Probabilities: Empty=40%, Red=10%, Yellow=20%, Blue=30%
  std::mt19937 rng(static_cast<unsigned int>(std::time(nullptr)));
  std::uniform_int_distribution<int> brick_type_dist(0, BRICK_DIST_MAX);

  for (int row = 0; row < BRICK_ROWS; ++row)
  {
    for (int col = 0; col < BRICK_COLS; ++col)
    {
      // Randomly decide if this cell should have a brick and what type
      int brick_type = brick_type_dist(rng);

      // Empty cell check (40% probability), skip creating a brick
      if (brick_type < BRICK_EMPTY_THRESHOLD)
      {
        continue;
      }

      auto brick = registry.create_entity();

      float x = start_x + static_cast<float>(col) * BRICK_WIDTH;
      float y = BRICK_WALL_TOP_OFFSET + static_cast<float>(row) * BRICK_HEIGHT;

      registry.add_component<TransformComponent>(
          brick, TransformComponent{{x, y}, BRICK_WIDTH, BRICK_HEIGHT});

      // Determine brick type based on weighted random selection
      int hit_points = 0;
      GLuint texture_id = 0;
      BrickColor color = BrickColor::BLUE;

      if (brick_type == BRICK_RED_VALUE)
      {
        // Red bricks (3 hits) - 10% probability
        hit_points = 3;
        texture_id = red_id;
        color = BrickColor::RED;
      }
      else if (brick_type >= BRICK_YELLOW_MIN && brick_type < BRICK_YELLOW_MAX)
      {
        // Yellow bricks (2 hits) - 20% probability (values 5 and 6)
        hit_points = 2;
        texture_id = yellow_id;
        color = BrickColor::YELLOW;
      }
      else
      {
        // Blue bricks (1 hit) - 30% probability (values 7-9)
        hit_points = 1;
        texture_id = blue_id;
        color = BrickColor::BLUE;
      }

      registry.add_component<SpriteComponent>(
          brick, SpriteComponent{texture_id, {1.0F, 1.0F, 1.0F, 1.0F}});
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
                               (error ? error : "unknown error"));
    }

    SDLWindow window("Breakout - Move paddle to bounce ball", WINDOW_WIDTH, WINDOW_HEIGHT);

    Registry registry;
    breakout::TextureManager texture_manager;
    breakout::SoundManager sound_manager;
    breakout::systems::ParticleSystem particle_system;
    breakout::systems::BallPhysicsSystem ball_physics_system(sound_manager, particle_system);
    breakout::systems::InputSystem input_system;
    breakout::systems::MovementSystem movement_system;
    breakout::systems::RenderSystem render_system;

    create_paddle(registry, texture_manager);
    create_ball(registry, texture_manager);
    create_brick_wall(registry, texture_manager);

    render_system.setup_ortho_projection(0.0F, static_cast<float>(WINDOW_WIDTH),
                                         static_cast<float>(WINDOW_HEIGHT), 0.0F);

    auto last_time = std::chrono::steady_clock::now();

    while (!window.should_close())
    {
      auto current_time = std::chrono::steady_clock::now();
      float delta_time = std::chrono::duration<float>(current_time - last_time).count();
      last_time = current_time;

      window.handle_events();
      input_system.update(registry);
      movement_system.update(registry, delta_time, static_cast<float>(WINDOW_WIDTH));
      ball_physics_system.update(registry, delta_time, static_cast<float>(WINDOW_WIDTH),
                                 static_cast<float>(WINDOW_HEIGHT));
      particle_system.update(registry, delta_time);

      glClearColor(CLEAR_COLOR[0], CLEAR_COLOR[1], CLEAR_COLOR[2], CLEAR_COLOR[3]);
      glClear(GL_COLOR_BUFFER_BIT);

      render_system.render(registry);

      window.swap_buffers();
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