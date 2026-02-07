#pragma once

#include "ecs/component.hpp"
#include "ecs/registry.hpp"
#include <SDL3/SDL_opengl.h>

namespace breakout::systems
{

class RenderSystem
{
public:
  void setup_ortho_projection(float left, float right, float bottom, float top) const
  {
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(left, right, bottom, top, -1.0f, 1.0f);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
  }

  void render(ecs::Registry &registry) const
  {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // First, render textured entities (bricks, paddle, ball)
    auto textured_entities = registry.view<ecs::TransformComponent, ecs::SpriteComponent>();
    glEnable(GL_TEXTURE_2D);

    for (ecs::Entity entity : textured_entities)
    {
      const auto *transform = registry.get_component<ecs::TransformComponent>(entity);
      const auto *sprite = registry.get_component<ecs::SpriteComponent>(entity);

      if (transform == nullptr || sprite == nullptr)
      {
        continue;
      }

      // Skip particles and shards (texture_id = 0)
      if (sprite->texture_id == 0)
      {
        continue;
      }

      draw_textured_quad(*transform, *sprite);
    }

    glDisable(GL_TEXTURE_2D);

    // Then, render untextured entities (particles and shards)
    for (ecs::Entity entity : textured_entities)
    {
      const auto *transform = registry.get_component<ecs::TransformComponent>(entity);
      const auto *sprite = registry.get_component<ecs::SpriteComponent>(entity);

      if (transform == nullptr || sprite == nullptr)
      {
        continue;
      }

      // Only render particles and shards (texture_id = 0)
      if (sprite->texture_id != 0)
      {
        continue;
      }

      draw_colored_quad(*transform, *sprite, registry, entity);
    }

    glDisable(GL_BLEND);
  }

private:
  void draw_textured_quad(const ecs::TransformComponent &transform,
                          const ecs::SpriteComponent &sprite) const
  {
    const float half_width = transform.width / 2.0f;
    const float half_height = transform.height / 2.0f;

    const float left = transform.position.x - half_width;
    const float right = transform.position.x + half_width;
    const float bottom = transform.position.y - half_height;
    const float top = transform.position.y + half_height;

    glBindTexture(GL_TEXTURE_2D, sprite.texture_id);

    glColor4f(sprite.tint.r, sprite.tint.g, sprite.tint.b, sprite.tint.a);

    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 0.0f);
    glVertex2f(left, top);
    glTexCoord2f(1.0f, 0.0f);
    glVertex2f(right, top);
    glTexCoord2f(1.0f, 1.0f);
    glVertex2f(right, bottom);
    glTexCoord2f(0.0f, 1.0f);
    glVertex2f(left, bottom);
    glEnd();

    glBindTexture(GL_TEXTURE_2D, 0);
  }

  void draw_colored_quad(const ecs::TransformComponent &transform,
                         const ecs::SpriteComponent &sprite, ecs::Registry &registry,
                         ecs::Entity entity) const
  {
    const float half_width = transform.width / 2.0f;
    const float half_height = transform.height / 2.0f;

    glColor4f(sprite.tint.r, sprite.tint.g, sprite.tint.b, sprite.tint.a);

    // Check if this is a shard (has rotation)
    auto *shard = registry.get_component<ecs::ShardComponent>(entity);

    if (shard != nullptr)
    {
      // Save current matrix and apply rotation for shards
      glPushMatrix();
      glTranslatef(transform.position.x, transform.position.y, 0.0f);
      glRotatef(shard->rotation * 180.0f / 3.14159f, 0.0f, 0.0f, 1.0f);

      glBegin(GL_QUADS);
      glVertex2f(-half_width, -half_height);
      glVertex2f(half_width, -half_height);
      glVertex2f(half_width, half_height);
      glVertex2f(-half_width, half_height);
      glEnd();

      glPopMatrix();
    }
    else
    {
      // Regular colored quad for particles (no rotation)
      const float left = transform.position.x - half_width;
      const float right = transform.position.x + half_width;
      const float bottom = transform.position.y - half_height;
      const float top = transform.position.y + half_height;

      glBegin(GL_QUADS);
      glVertex2f(left, top);
      glVertex2f(right, top);
      glVertex2f(right, bottom);
      glVertex2f(left, bottom);
      glEnd();
    }
  }
};

} // namespace breakout::systems