#pragma once

#include <SDL3/SDL.h>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

namespace breakout
{

class Texture
{
public:
  Texture() = default;
  explicit Texture(SDL_Texture *texture) : texture_{texture}
  {
  }
  ~Texture()
  {
    cleanup();
  }

  Texture(const Texture &) = delete;
  Texture &operator=(const Texture &) = delete;

  Texture(Texture &&other) noexcept : texture_{other.texture_}
  {
    other.texture_ = nullptr;
  }

  Texture &operator=(Texture &&other) noexcept
  {
    if (this != &other)
    {
      cleanup();
      texture_ = other.texture_;
      other.texture_ = nullptr;
    }
    return *this;
  }

  [[nodiscard]] SDL_Texture *get() const
  {
    return texture_;
  }
  [[nodiscard]] bool valid() const
  {
    return texture_ != nullptr;
  }

private:
  void cleanup() noexcept
  {
    if (texture_ != nullptr)
    {
      SDL_DestroyTexture(texture_);
      texture_ = nullptr;
    }
  }

  SDL_Texture *texture_{nullptr};
};

class TextureManager
{
public:
  explicit TextureManager(SDL_Renderer *renderer) : renderer_{renderer}
  {
  }

  [[nodiscard]] SDL_Texture *create_paddle_texture(int width, int height)
  {
    std::vector<std::uint8_t> pixels = generate_paddle_pixels(width, height);
    Texture texture = create_texture_from_pixels(width, height, pixels.data());
    SDL_Texture *sdl_texture = texture.get();
    if (sdl_texture == nullptr)
    {
      throw std::runtime_error("Failed to create paddle texture: " + std::string(SDL_GetError()));
    }
    textures_.push_back(std::move(texture));
    return sdl_texture;
  }

  [[nodiscard]] SDL_Texture *create_ball_texture(int width, int height)
  {
    std::vector<std::uint8_t> pixels = generate_ball_pixels(width, height);
    Texture texture = create_texture_from_pixels(width, height, pixels.data());
    SDL_Texture *sdl_texture = texture.get();
    if (sdl_texture == nullptr)
    {
      throw std::runtime_error("Failed to create ball texture: " + std::string(SDL_GetError()));
    }
    textures_.push_back(std::move(texture));
    return sdl_texture;
  }

  [[nodiscard]] SDL_Texture *create_brick_texture(int width, int height, std::uint8_t r,
                                                  std::uint8_t g, std::uint8_t b)
  {
    std::vector<std::uint8_t> pixels = generate_brick_pixels(width, height, r, g, b);
    Texture texture = create_texture_from_pixels(width, height, pixels.data());
    SDL_Texture *sdl_texture = texture.get();
    if (sdl_texture == nullptr)
    {
      throw std::runtime_error("Failed to create brick texture: " + std::string(SDL_GetError()));
    }
    textures_.push_back(std::move(texture));
    return sdl_texture;
  }

private:
  [[nodiscard]] std::vector<std::uint8_t> generate_paddle_pixels(int width, int height) const
  {
    std::vector<std::uint8_t> pixels(static_cast<std::size_t>(width * height * 4));

    const int bevel_size = 2;
    const int corner_radius = 8;

    for (int y = 0; y < height; ++y)
    {
      for (int x = 0; x < width; ++x)
      {
        std::size_t idx = static_cast<std::size_t>((y * width + x) * 4);

        // Check if pixel is in a rounded corner
        bool in_corner = false;
        float corner_dist = 0.0f;

        // Top-left corner
        if (x < corner_radius && y < corner_radius)
        {
          float dx = static_cast<float>(x - corner_radius);
          float dy = static_cast<float>(y - corner_radius);
          corner_dist = std::sqrt(dx * dx + dy * dy);
          if (corner_dist > corner_radius)
          {
            in_corner = true;
          }
        }
        // Top-right corner
        else if (x >= width - corner_radius && y < corner_radius)
        {
          float dx = static_cast<float>(x - (width - corner_radius - 1));
          float dy = static_cast<float>(y - corner_radius);
          corner_dist = std::sqrt(dx * dx + dy * dy);
          if (corner_dist > corner_radius)
          {
            in_corner = true;
          }
        }
        // Bottom-left corner
        else if (x < corner_radius && y >= height - corner_radius)
        {
          float dx = static_cast<float>(x - corner_radius);
          float dy = static_cast<float>(y - (height - corner_radius - 1));
          corner_dist = std::sqrt(dx * dx + dy * dy);
          if (corner_dist > corner_radius)
          {
            in_corner = true;
          }
        }
        // Bottom-right corner
        else if (x >= width - corner_radius && y >= height - corner_radius)
        {
          float dx = static_cast<float>(x - (width - corner_radius - 1));
          float dy = static_cast<float>(y - (height - corner_radius - 1));
          corner_dist = std::sqrt(dx * dx + dy * dy);
          if (corner_dist > corner_radius)
          {
            in_corner = true;
          }
        }

        std::uint8_t r, g, b, a;

        if (in_corner)
        {
          // Transparent outside rounded corners
          r = g = b = 0;
          a = 0;
        }
        else
        {
          bool is_top = y < bevel_size;
          bool is_bottom = y >= height - bevel_size;
          bool is_left = x < bevel_size;
          bool is_right = x >= width - bevel_size;

          if (is_top || is_left)
          {
            r = g = b = 200;
            a = 255;
          }
          else if (is_bottom || is_right)
          {
            r = g = b = 80;
            a = 255;
          }
          else
          {
            float gradient =
                static_cast<float>(y - bevel_size) / static_cast<float>(height - 2 * bevel_size);
            std::uint8_t value = static_cast<std::uint8_t>(140 - gradient * 40);
            r = g = b = value;
            a = 255;
          }
        }

        pixels[idx] = r;
        pixels[idx + 1] = g;
        pixels[idx + 2] = b;
        pixels[idx + 3] = a;
      }
    }

    return pixels;
  }

  [[nodiscard]] std::vector<std::uint8_t> generate_ball_pixels(int width, int height) const
  {
    std::vector<std::uint8_t> pixels(static_cast<std::size_t>(width * height * 4));

    const float center_x = width / 2.0f;
    const float center_y = height / 2.0f;
    const float radius = std::min(center_x, center_y) - 1.0f;

    for (int y = 0; y < height; ++y)
    {
      for (int x = 0; x < width; ++x)
      {
        std::size_t idx = static_cast<std::size_t>((y * width + x) * 4);

        float dx = static_cast<float>(x) - center_x;
        float dy = static_cast<float>(y) - center_y;
        float distance = std::sqrt(dx * dx + dy * dy);

        if (distance <= radius)
        {
          // White ball with gradient
          float gradient = 1.0f - (distance / radius) * 0.3f;
          std::uint8_t value = static_cast<std::uint8_t>(255 * gradient);
          pixels[idx] = value;
          pixels[idx + 1] = value;
          pixels[idx + 2] = value;
          pixels[idx + 3] = 255;
        }
        else
        {
          // Transparent outside ball
          pixels[idx] = 0;
          pixels[idx + 1] = 0;
          pixels[idx + 2] = 0;
          pixels[idx + 3] = 0;
        }
      }
    }

    return pixels;
  }

  [[nodiscard]] std::vector<std::uint8_t> generate_brick_pixels(int width, int height,
                                                                std::uint8_t base_r,
                                                                std::uint8_t base_g,
                                                                std::uint8_t base_b) const
  {
    std::vector<std::uint8_t> pixels(static_cast<std::size_t>(width * height * 4));

    const int bevel_size = 2;
    const int corner_radius = 4;

    for (int y = 0; y < height; ++y)
    {
      for (int x = 0; x < width; ++x)
      {
        std::size_t idx = static_cast<std::size_t>((y * width + x) * 4);

        // Check if pixel is in a rounded corner
        bool in_corner = false;

        // Top-left corner
        if (x < corner_radius && y < corner_radius)
        {
          float dx = static_cast<float>(x - corner_radius);
          float dy = static_cast<float>(y - corner_radius);
          float corner_dist = std::sqrt(dx * dx + dy * dy);
          if (corner_dist > corner_radius)
          {
            in_corner = true;
          }
        }
        // Top-right corner
        else if (x >= width - corner_radius && y < corner_radius)
        {
          float dx = static_cast<float>(x - (width - corner_radius - 1));
          float dy = static_cast<float>(y - corner_radius);
          float corner_dist = std::sqrt(dx * dx + dy * dy);
          if (corner_dist > corner_radius)
          {
            in_corner = true;
          }
        }
        // Bottom-left corner
        else if (x < corner_radius && y >= height - corner_radius)
        {
          float dx = static_cast<float>(x - corner_radius);
          float dy = static_cast<float>(y - (height - corner_radius - 1));
          float corner_dist = std::sqrt(dx * dx + dy * dy);
          if (corner_dist > corner_radius)
          {
            in_corner = true;
          }
        }
        // Bottom-right corner
        else if (x >= width - corner_radius && y >= height - corner_radius)
        {
          float dx = static_cast<float>(x - (width - corner_radius - 1));
          float dy = static_cast<float>(y - (height - corner_radius - 1));
          float corner_dist = std::sqrt(dx * dx + dy * dy);
          if (corner_dist > corner_radius)
          {
            in_corner = true;
          }
        }

        std::uint8_t r, g, b, a;

        if (in_corner)
        {
          r = g = b = 0;
          a = 0;
        }
        else
        {
          bool is_top = y < bevel_size;
          bool is_bottom = y >= height - bevel_size;
          bool is_left = x < bevel_size;
          bool is_right = x >= width - bevel_size;

          // Lighten for top/left (highlight)
          if (is_top || is_left)
          {
            r = static_cast<std::uint8_t>(std::min(255, static_cast<int>(base_r * 1.3f)));
            g = static_cast<std::uint8_t>(std::min(255, static_cast<int>(base_g * 1.3f)));
            b = static_cast<std::uint8_t>(std::min(255, static_cast<int>(base_b * 1.3f)));
            a = 255;
          }
          // Darken for bottom/right (shadow)
          else if (is_bottom || is_right)
          {
            r = static_cast<std::uint8_t>(base_r * 0.6f);
            g = static_cast<std::uint8_t>(base_g * 0.6f);
            b = static_cast<std::uint8_t>(base_b * 0.6f);
            a = 255;
          }
          // Base color for center
          else
          {
            r = base_r;
            g = base_g;
            b = base_b;
            a = 255;
          }
        }

        pixels[idx] = r;
        pixels[idx + 1] = g;
        pixels[idx + 2] = b;
        pixels[idx + 3] = a;
      }
    }

    return pixels;
  }

  [[nodiscard]] Texture create_texture_from_pixels(int width, int height,
                                                   const std::uint8_t *pixels) const
  {
    // Create surface and copy pixel data (SDL_CreateSurfaceFrom requires non-const void*)
    SDL_Surface *surface = SDL_CreateSurface(width, height, SDL_PIXELFORMAT_RGBA32);
    if (surface == nullptr)
    {
      throw std::runtime_error("Failed to create SDL surface: " + std::string(SDL_GetError()));
    }

    // Copy pixel data to surface
    std::memcpy(surface->pixels, pixels, static_cast<std::size_t>(width * height * 4));

    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer_, surface);
    SDL_DestroySurface(surface);

    if (texture == nullptr)
    {
      throw std::runtime_error("Failed to create SDL texture: " + std::string(SDL_GetError()));
    }

    return Texture{texture};
  }

  SDL_Renderer *renderer_{nullptr};
  std::vector<Texture> textures_;
};

} // namespace breakout
