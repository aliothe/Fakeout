#pragma once

#include "game_constants.hpp"
#include "score_manager.hpp"
#include <SDL3/SDL.h>
#include <string>

namespace breakout::systems
{

class ScoreRenderSystem
{
public:
  explicit ScoreRenderSystem(SDL_Renderer *renderer) : renderer_{renderer}
  {
  }

  void render(const ScoreManager &score_manager)
  {
    // Draw score at top center of screen with white color
    int score = score_manager.current_score();
    std::string score_text = std::to_string(score);

    // Position at top center (5% down from top)
    constexpr float SCORE_TOP_OFFSET_FACTOR = 0.05F;
    constexpr float center_x = WINDOW_WIDTH_F / 2.0f;
    constexpr float top_y = WINDOW_HEIGHT_F * SCORE_TOP_OFFSET_FACTOR;

    // Simple 7-segment display style rendering
    // Each digit is drawn using rectangles
    float digit_width = 20.0f;
    float digit_height = 40.0f;
    float spacing = 10.0f;

    float total_width = score_text.length() * (digit_width + spacing) - spacing;
    float start_x = center_x - total_width / 2.0f;

    SDL_SetRenderDrawColor(renderer_, 255, 255, 255, 255); // White

    for (size_t i = 0; i < score_text.length(); ++i)
    {
      char digit = score_text[i];
      float x = start_x + i * (digit_width + spacing);
      draw_digit(digit, x, top_y, digit_width, digit_height);
    }
  }

private:
  void draw_digit(char digit, float x, float y, float w, float h)
  {
    // Segment layout:
    //  --0--
    // |     |
    // 5     1
    // |     |
    //  --6--
    // |     |
    // 4     2
    // |     |
    //  --3--

    float thickness = w / 4.0f;
    float seg_len = w - thickness;
    float seg_height = (h - 3.0f * thickness) / 2.0f;

    // Define which segments are lit for each digit
    bool segments[7] = {false};

    switch (digit)
    {
    case '0':
      segments[0] = segments[1] = segments[2] = segments[3] = segments[4] = segments[5] = true;
      break;
    case '1':
      segments[1] = segments[2] = true;
      break;
    case '2':
      segments[0] = segments[1] = segments[3] = segments[4] = segments[6] = true;
      break;
    case '3':
      segments[0] = segments[1] = segments[2] = segments[3] = segments[6] = true;
      break;
    case '4':
      segments[1] = segments[2] = segments[5] = segments[6] = true;
      break;
    case '5':
      segments[0] = segments[2] = segments[3] = segments[5] = segments[6] = true;
      break;
    case '6':
      segments[0] = segments[2] = segments[3] = segments[4] = segments[5] = segments[6] = true;
      break;
    case '7':
      segments[0] = segments[1] = segments[2] = true;
      break;
    case '8':
      segments[0] = segments[1] = segments[2] = segments[3] = segments[4] = segments[5] =
          segments[6] = true;
      break;
    case '9':
      segments[0] = segments[1] = segments[2] = segments[3] = segments[5] = segments[6] = true;
      break;
    default:
      // Unknown digit - display nothing
      break;
    }

    // Draw horizontal segments
    if (segments[0])
    { // Top
      SDL_FRect rect = {x + thickness / 2.0f, y, seg_len, thickness};
      SDL_RenderFillRect(renderer_, &rect);
    }
    if (segments[3])
    { // Bottom
      SDL_FRect rect = {x + thickness / 2.0f, y + h - thickness, seg_len, thickness};
      SDL_RenderFillRect(renderer_, &rect);
    }
    if (segments[6])
    { // Middle
      SDL_FRect rect = {x + thickness / 2.0f, y + h / 2.0f - thickness / 2.0f, seg_len, thickness};
      SDL_RenderFillRect(renderer_, &rect);
    }

    // Draw vertical segments
    if (segments[5])
    { // Top left
      SDL_FRect rect = {x, y + thickness / 2.0f, thickness, seg_height};
      SDL_RenderFillRect(renderer_, &rect);
    }
    if (segments[1])
    { // Top right
      SDL_FRect rect = {x + w - thickness, y + thickness / 2.0f, thickness, seg_height};
      SDL_RenderFillRect(renderer_, &rect);
    }
    if (segments[4])
    { // Bottom left
      SDL_FRect rect = {x, y + h / 2.0f, thickness, seg_height};
      SDL_RenderFillRect(renderer_, &rect);
    }
    if (segments[2])
    { // Bottom right
      SDL_FRect rect = {x + w - thickness, y + h / 2.0f, thickness, seg_height};
      SDL_RenderFillRect(renderer_, &rect);
    }
  }

  SDL_Renderer *renderer_{nullptr};
};

} // namespace breakout::systems
