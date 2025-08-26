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
  // Optional app draw callback (content area). If null, default placeholder is
  // drawn.
  void (*draw_content)(Graphics &gfx, const Rect &content_rect,
                       void *user_data);
  void *user_data;
};

uint32_t get_titlebar_height();
bool point_in_titlebar(const Window &w, uint32_t x, uint32_t y);
void draw(Graphics &gfx, const Window &w);
void draw_frame_only(Graphics &gfx, const Window &w);

// Returns index of button hit in the left titlebar cluster:
// 0: close, 1: minimize, 2: maximize, 3: pin (always_on_top)
// Returns UINT32_MAX if no button was hit.
uint32_t hit_test_button(const Window &w, uint32_t x, uint32_t y);

// Resize hit-test bitmask
enum ResizeHit : uint32_t {
  ResizeNone = 0,
  ResizeLeft = 1u << 0,
  ResizeRight = 1u << 1,
  ResizeTop = 1u << 2,
  ResizeBottom = 1u << 3,
};

// Returns bitmask of edges/corners hovered for resize (combination of Resize*
// flags)
uint32_t hit_test_resize(const Window &w, uint32_t x, uint32_t y);

} // namespace window
} // namespace ui