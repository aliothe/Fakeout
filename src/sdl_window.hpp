#pragma once

#include <SDL3/SDL.h>
#include <stdexcept>
#include <string>
#include <string_view>

class SDLWindow
{
private:
  SDL_Window *window_{nullptr};
  SDL_GLContext context_{nullptr};
  bool should_close_{false};

public:
  explicit SDLWindow(std::string_view title, int width, int height);
  ~SDLWindow();

  // Rule of Five
  SDLWindow(const SDLWindow &) = delete;
  SDLWindow &operator=(const SDLWindow &) = delete;
  SDLWindow(SDLWindow &&other) noexcept;
  SDLWindow &operator=(SDLWindow &&other) noexcept;

  void swap_buffers() const;
  [[nodiscard]] bool should_close() const;
  void handle_events();

private:
  void cleanup() noexcept;
};

class SDL
{
private:
  bool initialized_{false};

public:
  explicit SDL(Uint32 flags);
  ~SDL();

  // Rule of Five
  SDL(const SDL &) = delete;
  SDL &operator=(const SDL &) = delete;
  SDL(SDL &&other) noexcept;
  SDL &operator=(SDL &&other) noexcept;

private:
  void cleanup() noexcept;
};