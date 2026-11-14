#pragma once

#include <lgfx/v1/LGFXBase.hpp>
#include <lgfx/v1/LGFX_Sprite.hpp>

// Filters the transfer of a palette_4bit sprite as area512_crt_push does an
// RGB565 sprite drawn in the same colors.
void area512_quaternary_crt_push(
  lgfx::LGFX_Device *display,
  lgfx::LGFX_Sprite *sprite,
  int x,
  int y,
  bool core_shifts_green_and_narrows_blue,
  bool transparent,
  uint32_t transparent_color_index
);
