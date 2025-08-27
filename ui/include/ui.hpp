#pragma once
#include <cstdint>

class Graphics;

namespace ui {

struct Rect {
  uint32_t x;
  uint32_t y;
  uint32_t w;
  uint32_t h;
};

// High-level render layers to control draw order
enum class RenderLayer : uint32_t {
  Background = 0,        // desktop background
  Taskbar = 1,           // taskbar and its widgets
  WindowsBack = 2,       // non-top, non-focused windows
  WindowsFocused = 3,    // focused window in normal layer
  WindowsTop = 4,        // always-on-top windows (non-focused)
  WindowsTopFocused = 5, // focused always-on-top
  Overlay = 6,           // overlays (start menu, menus, tooltips)
  Cursor = 7             // mouse cursor (topmost)
};

// Draw the desktop UI (taskbar + window) with default centered window.
void draw_desktop(Graphics &gfx);

// Draw the desktop UI (taskbar + window) at provided window rect.
void draw_desktop(Graphics &gfx, const Rect &window_rect);

// Query UI metrics and helpers
uint32_t get_taskbar_height(uint32_t screen_h);
uint32_t get_titlebar_height();
bool point_in_titlebar(const Rect &window_rect, uint32_t x, uint32_t y);

namespace window {
struct Window;
}

// Draw desktop with multiple windows; draws taskbar and all non-minimized
// windows in order
void draw_desktop(Graphics &gfx, const window::Window *windows, uint32_t count);

// Layered renderer: draw a specific layer only. Callers can iterate layers
// from Background to Overlay to produce the full scene, or render selectively.
void draw_desktop_layer(Graphics &gfx, RenderLayer layer,
                        const window::Window *windows, uint32_t count);

// Region rendering removed due to border artifacts.

} // namespace ui
