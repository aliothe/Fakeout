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
  // Set OpenGL attributes
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

  window_ = SDL_CreateWindow(std::string(title).c_str(), width, height, SDL_WINDOW_OPENGL);

  if (!window_)
  {
    std::cerr << "Window creation failed!\n";
    throw std::runtime_error("Failed to create SDL window: " + std::string(SDL_GetError()));
  }

  context_ = SDL_GL_CreateContext(window_);
  if (!context_)
  {
    SDL_DestroyWindow(window_);
    window_ = nullptr;
    throw std::runtime_error("Failed to create OpenGL context: " + std::string(SDL_GetError()));
  }

  // Enable VSync for smooth rendering (-1 = adaptive, 1 = standard)
  if (SDL_GL_SetSwapInterval(-1) != 0)
  {
    SDL_GL_SetSwapInterval(1);
  }
}

SDLWindow::~SDLWindow()
{
  cleanup();
}

SDLWindow::SDLWindow(SDLWindow &&other) noexcept
    : window_(other.window_), context_(other.context_), should_close_(other.should_close_)
{
  other.window_ = nullptr;
  other.context_ = nullptr;
  other.should_close_ = false;
}

SDLWindow &SDLWindow::operator=(SDLWindow &&other) noexcept
{
  if (this != &other)
  {
    cleanup();
    window_ = other.window_;
    context_ = other.context_;
    should_close_ = other.should_close_;
    other.window_ = nullptr;
    other.context_ = nullptr;
    other.should_close_ = false;
  }
  return *this;
}

void SDLWindow::cleanup() noexcept
{
  if (context_)
  {
    SDL_GL_DestroyContext(context_);
    context_ = nullptr;
  }
  if (window_)
  {
    SDL_DestroyWindow(window_);
    window_ = nullptr;
  }
}

void SDLWindow::swap_buffers() const
{
  if (window_)
  {
    SDL_GL_SwapWindow(window_);
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