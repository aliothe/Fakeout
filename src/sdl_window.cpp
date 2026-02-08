#include "sdl_window.hpp"
#include <iostream>

// SDL class implementation
SDL::SDL(Uint32 flags)
{
  if (SDL_Init(flags) != 0)
  {
    const char *error = SDL_GetError();
    std::cerr << "SDL Initialization Failed!\n";
    std::cerr << "SDL Error: " << (error ? error : "Unknown error") << '\n';
    std::cerr << "SDL Version: " << SDL_GetRevision() << '\n';
    std::cerr << "Available Video Drivers:\n";
    for (int i = 0; i < SDL_GetNumVideoDrivers(); ++i)
    {
      std::cerr << "  " << SDL_GetVideoDriver(i) << '\n';
    }
    std::cerr << "SDL_Init flags: " << flags << "\n";
    throw std::runtime_error("SDL initialization failed: " +
                             std::string(error ? error : "Unknown error"));
  }
  initialized_ = true;
}

SDL::~SDL()
{
  cleanup();
}

SDL::SDL(SDL &&other) noexcept : initialized_(other.initialized_)
{
  other.initialized_ = false;
}

SDL &SDL::operator=(SDL &&other) noexcept
{
  if (this != &other)
  {
    cleanup();
    initialized_ = other.initialized_;
    other.initialized_ = false;
  }
  return *this;
}

void SDL::cleanup() noexcept
{
  if (initialized_)
  {
    SDL_Quit();
    initialized_ = false;
  }
}

// SDLWindow class implementation
SDLWindow::SDLWindow(std::string_view title, int width, int height)
{
  window_ = SDL_CreateWindow(std::string(title).c_str(), width, height, 0);

  if (!window_)
  {
    std::cerr << "Window creation failed!\n";
    throw std::runtime_error("Failed to create SDL window: " + std::string(SDL_GetError()));
  }

  renderer_ = SDL_CreateRenderer(window_, nullptr);
  if (!renderer_)
  {
    SDL_DestroyWindow(window_);
    window_ = nullptr;
    throw std::runtime_error("Failed to create SDL renderer: " + std::string(SDL_GetError()));
  }
}

SDLWindow::~SDLWindow()
{
  cleanup();
}

SDLWindow::SDLWindow(SDLWindow &&other) noexcept
    : window_(other.window_), renderer_(other.renderer_), should_close_(other.should_close_)
{
  other.window_ = nullptr;
  other.renderer_ = nullptr;
  other.should_close_ = false;
}

SDLWindow &SDLWindow::operator=(SDLWindow &&other) noexcept
{
  if (this != &other)
  {
    cleanup();
    window_ = other.window_;
    renderer_ = other.renderer_;
    should_close_ = other.should_close_;
    other.window_ = nullptr;
    other.renderer_ = nullptr;
    other.should_close_ = false;
  }
  return *this;
}

void SDLWindow::cleanup() noexcept
{
  if (renderer_)
  {
    SDL_DestroyRenderer(renderer_);
    renderer_ = nullptr;
  }
  if (window_)
  {
    SDL_DestroyWindow(window_);
    window_ = nullptr;
  }
}

void SDLWindow::present() const
{
  if (renderer_)
  {
    SDL_RenderPresent(renderer_);
  }
}

bool SDLWindow::should_close() const
{
  return should_close_;
}

void SDLWindow::handle_events()
{
  SDL_Event event;
  while (SDL_PollEvent(&event))
  {
    switch (event.type)
    {
    case SDL_EVENT_QUIT:
      should_close_ = true;
      break;
    case SDL_EVENT_KEY_DOWN:
      if (event.key.key == SDLK_ESCAPE)
      {
        should_close_ = true;
      }
      break;
    default:
      break;
    }
  }
}

SDL_Renderer *SDLWindow::get_renderer() const
{
  return renderer_;
}
