#pragma once

#include "ui.hpp"
#include "window.hpp"
#include <cstdint>

namespace ui::window_manager {

// Window creation options
struct WindowOptions {
  const char *title = "Window";
  uint32_t width = 400;
  uint32_t height = 300;
  uint32_t x = 0; // 0 means auto-center
  uint32_t y = 0; // 0 means auto-center
  bool resizable = true;
  bool movable = true;
  bool draggable = true;
  bool closeable = true;
  bool focused = true;
  bool always_on_top = false;
  bool minimized = false;
  bool maximized = false;
  bool fullscreen = false;
  void *user_data = nullptr;
  void (*draw_content)(Graphics &gfx, const Rect &content_rect,
                       void *user_data) = nullptr;
  void (*on_mouse)(const window::MouseEvent &ev, void *user_data) = nullptr;
};

// Create a window with default positioning (centered)
window::Window create_window(uint32_t screen_w, uint32_t screen_h,
                             const WindowOptions &options = {});

// Create a window positioned relative to an anchor window
window::Window create_window_relative(uint32_t screen_w, uint32_t screen_h,
                                      const window::Window &anchor,
                                      const WindowOptions &options = {});

// Create a centered window with common defaults
window::Window create_centered_window(
    uint32_t screen_w, uint32_t screen_h, const char *title, uint32_t width,
    uint32_t height,
    void (*draw_func)(Graphics &gfx, const Rect &content_rect,
                      void *user_data) = nullptr,
    void *user_data = nullptr);

// Create a dialog-style window (smaller, centered)
window::Window
create_dialog_window(uint32_t screen_w, uint32_t screen_h, const char *title,
                     uint32_t width, uint32_t height,
                     void (*draw_func)(Graphics &gfx, const Rect &content_rect,
                                       void *user_data) = nullptr,
                     void *user_data = nullptr);

// Utility functions for common window positioning
uint32_t center_x(uint32_t screen_w, uint32_t window_w);
uint32_t center_y(uint32_t screen_h, uint32_t window_h);
bool ensure_window_fits(uint32_t &x, uint32_t &y, uint32_t w, uint32_t h,
                        uint32_t screen_w, uint32_t screen_h);

} // namespace ui::window_manager
