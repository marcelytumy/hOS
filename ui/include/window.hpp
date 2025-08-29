#pragma once
#include "ui.hpp"

class Graphics;

namespace ui {
namespace window {

struct MouseEvent {
  enum class Type : uint32_t { Down = 0, Up = 1, Move = 2, Wheel = 3 };
  Type type;
  // Position relative to the window content rect (not including frame/padding)
  uint32_t x;
  uint32_t y;
  bool left;
  bool right;
  bool middle;
  int8_t wheel_y; // positive for wheel up, negative for wheel down
};

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
  // Optional mouse event handler for content area. Coordinates are relative to
  // content_rect origin (passed above to draw_content).
  void (*on_mouse)(const MouseEvent &ev, void *user_data);
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

// Compute the content rect for a window as drawn by draw(), taking into
// account fullscreen/maximized states and taskbar height. screen_w/h are the
// framebuffer dimensions used for layout.
Rect get_content_rect(const Window &w, uint32_t screen_w, uint32_t screen_h);

} // namespace window
} // namespace ui