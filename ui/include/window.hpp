#pragma once
#include "ui.hpp"

class Graphics;

namespace ui {
namespace window {

struct Window {
  Rect rect;
  const char *title;
  bool minimized;
  bool maximized;
  bool fullscreen;
  bool resizable;
  bool movable;
  bool draggable;
  bool closeable;
  bool focused;
  bool always_on_top;
};

uint32_t get_titlebar_height();
bool point_in_titlebar(const Window &w, uint32_t x, uint32_t y);
void draw(Graphics &gfx, const Window &w);

// Returns index of button hit in the left titlebar cluster:
// 0: close, 1: minimize, 2: maximize, 3: pin (always_on_top)
// Returns UINT32_MAX if no button was hit.
uint32_t hit_test_button(const Window &w, uint32_t x, uint32_t y);

} // namespace window
} // namespace ui