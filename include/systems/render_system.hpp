#pragma once

#include "ecs/component.hpp"
#include "ecs/registry.hpp"
#include <SDL3/SDL.h>

namespace breakout::systems
{

class RenderSystem
{
public:
  explicit RenderSystem(SDL_Renderer *renderer) : renderer_{renderer}
  {
  }

  void render(const ecs::Registry &registry) const
  {
    // First, render textured entities (bricks, paddle, ball)
    auto textured_entities = registry.view<ecs::TransformComponent, ecs::SpriteComponent>();

    for (ecs::Entity entity : textured_entities)
    {
      const auto *transform = registry.get_component<ecs::TransformComponent>(entity);
      const auto *sprite = registry.get_component<ecs::SpriteComponent>(entity);

      if (transform == nullptr || sprite == nullptr)
      {
        continue;
      }

      // Skip particles and shards (texture = nullptr)
      if (sprite->texture != nullptr)
      {
        draw_textured_quad(*transform, *sprite);
      }
      else
      {
        draw_colored_quad(*transform, *sprite);
      }
    }
  }

private:
  void draw_textured_quad(const ecs::TransformComponent &transform,
                          const ecs::SpriteComponent &sprite) const
  {
    const float half_width = transform.width / 2.0f;
    const float half_height = transform.height / 2.0f;

    const SDL_FRect dst_rect = {transform.position.x - half_width,
                                transform.position.y - half_height, transform.width,
                                transform.height};

    // Set tint color/alpha modulation
    SDL_SetTextureColorMod(sprite.texture, static_cast<std::uint8_t>(sprite.tint.r * 255.0F),
                           static_cast<std::uint8_t>(sprite.tint.g * 255.0F),
                           static_cast<std::uint8_t>(sprite.tint.b * 255.0F));
    SDL_SetTextureAlphaMod(sprite.texture, static_cast<std::uint8_t>(sprite.tint.a * 255.0F));

    SDL_RenderTexture(renderer_, sprite.texture, nullptr, &dst_rect);
  }

  void draw_colored_quad(const ecs::TransformComponent &transform,
                         const ecs::SpriteComponent &sprite) const
  {
    const float half_width = transform.width / 2.0f;
    const float half_height = transform.height / 2.0f;

    SDL_SetRenderDrawColor(renderer_, static_cast<std::uint8_t>(sprite.tint.r * 255.0F),
                           static_cast<std::uint8_t>(sprite.tint.g * 255.0F),
                           static_cast<std::uint8_t>(sprite.tint.b * 255.0F),
                           static_cast<std::uint8_t>(sprite.tint.a * 255.0F));

    SDL_FRect rect = {transform.position.x - half_width, transform.position.y - half_height,
                      transform.width, transform.height};
    SDL_RenderFillRect(renderer_, &rect);
  }

  SDL_Renderer *renderer_{nullptr};
};

} // namespace breakout::systems
