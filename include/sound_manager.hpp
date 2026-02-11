#pragma once

#include <SDL3/SDL.h>
#include <cmath>
#include <memory>
#include <numbers>
#include <random>
#include <stdexcept>
#include <vector>

namespace breakout
{

class SoundManager
{
public:
  SoundManager()
  {
    // Create audio spec for our generated sound
    SDL_AudioSpec spec{};
    spec.freq = 44100;
    spec.format = SDL_AUDIO_S16;
    spec.channels = 1;

    // Open audio device with stream
    stream_ = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, nullptr, nullptr);
    if (stream_ == nullptr)
    {
      throw std::runtime_error(std::string("Failed to open audio stream: ") + SDL_GetError());
    }

    // Resume audio stream (starts paused by default)
    SDL_ResumeAudioStreamDevice(stream_);

    // Generate the ping sound
    generate_ping_sound();

    // Generate brick breaking sounds
    generate_brick_sounds();
  }

  ~SoundManager()
  {
    if (stream_ != nullptr)
    {
      SDL_DestroyAudioStream(stream_);
    }
  }

  // Rule of Five - disable copying, allow moving
  SoundManager(const SoundManager &) = delete;
  SoundManager &operator=(const SoundManager &) = delete;
  SoundManager(SoundManager &&other) noexcept
      : stream_(other.stream_), ping_sound_(std::move(other.ping_sound_)),
        red_break_sound_(std::move(other.red_break_sound_)),
        yellow_break_sound_(std::move(other.yellow_break_sound_)),
        blue_break_sound_(std::move(other.blue_break_sound_))
  {
    other.stream_ = nullptr;
  }

  SoundManager &operator=(SoundManager &&other) noexcept
  {
    if (this != &other)
    {
      if (stream_ != nullptr)
      {
        SDL_DestroyAudioStream(stream_);
      }
      stream_ = other.stream_;
      ping_sound_ = std::move(other.ping_sound_);
      red_break_sound_ = std::move(other.red_break_sound_);
      yellow_break_sound_ = std::move(other.yellow_break_sound_);
      blue_break_sound_ = std::move(other.blue_break_sound_);
      other.stream_ = nullptr;
    }
    return *this;
  }

  void play_ping()
  {
    if (stream_ != nullptr && !ping_sound_.empty())
    {
      // Clear any pending audio to prevent buildup
      SDL_ClearAudioStream(stream_);
      // Put the ping sound data into the stream
      SDL_PutAudioStreamData(stream_, ping_sound_.data(),
                             static_cast<int>(ping_sound_.size() * sizeof(std::int16_t)));
    }
  }

  // Play brick breaking sounds based on type
  void play_brick_break_red()
  {
    play_sound(red_break_sound_);
  }

  void play_brick_break_yellow()
  {
    play_sound(yellow_break_sound_);
  }

  void play_brick_break_blue()
  {
    play_sound(blue_break_sound_);
  }

private:
  void play_sound(const std::vector<std::int16_t> &sound)
  {
    if (stream_ != nullptr && !sound.empty())
    {
      SDL_ClearAudioStream(stream_);
      SDL_PutAudioStreamData(stream_, sound.data(),
                             static_cast<int>(sound.size() * sizeof(std::int16_t)));
    }
  }

  void generate_ping_sound()
  {
    ping_sound_ = generate_tone(1200.0f, 0.15f, 20.0f);
  }

  void generate_brick_sounds()
  {
    // Red brick (3 hits) - Deep, heavy crunch
    red_break_sound_ = generate_break_sound(300.0f, 0.3f, 0.8f);

    // Yellow brick (2 hits) - Medium crack
    yellow_break_sound_ = generate_break_sound(600.0f, 0.25f, 0.6f);

    // Blue brick (1 hit) - Light, high-pitched break
    blue_break_sound_ = generate_break_sound(900.0f, 0.2f, 0.4f);
  }

  std::vector<std::int16_t> generate_tone(float frequency, float duration, float decay_rate)
  {
    const int sample_rate = 44100;
    const auto num_samples = static_cast<std::size_t>(sample_rate * duration);
    std::vector<std::int16_t> sound;
    sound.reserve(num_samples);

    for (std::size_t i = 0; i < num_samples; ++i)
    {
      float t = static_cast<float>(i) / static_cast<float>(sample_rate);
      float envelope = std::exp(-decay_rate * t);
      float sample = std::sin(2.0f * std::numbers::pi_v<float> * frequency * t) * envelope;

      // Add a higher harmonic for more "metallic" quality
      sample += 0.3f * std::sin(2.0f * std::numbers::pi_v<float> * frequency * 2.5f * t) * envelope;

      constexpr float max_amplitude = 0.5f;
      std::int16_t sample_int = static_cast<std::int16_t>(sample * max_amplitude * 32767.0f);
      sound.push_back(sample_int);
    }

    return sound;
  }

  std::vector<std::int16_t> generate_break_sound(float base_freq, float duration,
                                                 float noise_amount)
  {
    const int sample_rate = 44100;
    const auto num_samples = static_cast<std::size_t>(sample_rate * duration);
    std::vector<std::int16_t> sound;
    sound.reserve(num_samples);

    // Random number generator for noise
    std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution<float> noise_dist(-1.0f, 1.0f);

    for (std::size_t i = 0; i < num_samples; ++i)
    {
      float t = static_cast<float>(i) / static_cast<float>(sample_rate);

      // Frequency sweep (drop for breaking effect)
      float freq = base_freq * (1.0f - t / duration * 0.5f);

      // Main tone
      float envelope = std::exp(-10.0f * t);
      float sample = std::sin(2.0f * std::numbers::pi_v<float> * freq * t) * envelope;

      // Add noise for crunch effect
      sample += noise_dist(rng) * noise_amount * envelope;

      // Add second harmonic for body
      sample += 0.5f * std::sin(2.0f * std::numbers::pi_v<float> * freq * 2.0f * t) * envelope;

      constexpr float max_amplitude = 0.6f;
      std::int16_t sample_int = static_cast<std::int16_t>(sample * max_amplitude * 32767.0f);
      sound.push_back(sample_int);
    }

    return sound;
  }

  SDL_AudioStream *stream_{nullptr};
  std::vector<std::int16_t> ping_sound_;
  std::vector<std::int16_t> red_break_sound_;
  std::vector<std::int16_t> yellow_break_sound_;
  std::vector<std::int16_t> blue_break_sound_;
};

} // namespace breakout