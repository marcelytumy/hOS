#pragma once
#include "ui.hpp"
#include <cstdint>

class Graphics;

namespace ui {

namespace window {
struct Window;
}

namespace taskbar {

// Special hit-test code for Start button
static constexpr uint32_t kHitStart = 0xFFFFFFFEu; // UINT32_MAX - 1 sentinel

struct ButtonInfo {
  Rect rect;
  uint32_t window_index;
};

uint32_t height(uint32_t screen_h);
void draw(Graphics &gfx, uint32_t screen_w, uint32_t screen_h,
          const window::Window *windows, uint32_t count);

// Returns index of clicked window if a button is clicked, otherwise UINT32_MAX
uint32_t hit_test(uint32_t x, uint32_t y, uint32_t screen_w, uint32_t screen_h,
                  const window::Window *windows, uint32_t count);

} // namespace taskbar
} // namespace ui
