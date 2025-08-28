#include "window_manager.hpp"

namespace ui::window_manager {

uint32_t center_x(uint32_t screen_w, uint32_t window_w) {
  return screen_w > window_w ? (screen_w - window_w) / 2 : 0;
}

uint32_t center_y(uint32_t screen_h, uint32_t window_h) {
  return screen_h > window_h ? (screen_h - window_h) / 2 : 0;
}

bool ensure_window_fits(uint32_t &x, uint32_t &y, uint32_t w, uint32_t h,
                        uint32_t screen_w, uint32_t screen_h) {
  bool adjusted = false;

  // Ensure window doesn't go off the right edge
  if (x + w > screen_w) {
    x = screen_w > w ? (screen_w - w) : 0;
    adjusted = true;
  }

  // Ensure window doesn't go off the bottom edge
  if (y + h > screen_h) {
    y = screen_h > h ? (screen_h - h) : 0;
    adjusted = true;
  }

  return adjusted;
}

window::Window create_window(uint32_t screen_w, uint32_t screen_h,
                             const WindowOptions &options) {
  uint32_t x = options.x;
  uint32_t y = options.y;

  // Auto-center if x or y is 0
  if (x == 0) {
    x = center_x(screen_w, options.width);
  }
  if (y == 0) {
    y = center_y(screen_h, options.height);
  }

  // Ensure window fits on screen
  ensure_window_fits(x, y, options.width, options.height, screen_w, screen_h);

  window::Window window = {};
  window.rect = Rect{x, y, options.width, options.height};
  window.title = options.title;
  window.minimized = options.minimized;
  window.maximized = options.maximized;
  window.fullscreen = options.fullscreen;
  window.resizable = options.resizable;
  window.movable = options.movable;
  window.draggable = options.draggable;
  window.closeable = options.closeable;
  window.focused = options.focused;
  window.always_on_top = options.always_on_top;
  window.user_data = options.user_data;
  window.draw_content = options.draw_content;
  window.on_mouse = options.on_mouse;

  return window;
}

window::Window create_window_relative(uint32_t screen_w, uint32_t screen_h,
                                      const window::Window &anchor,
                                      const WindowOptions &options) {
  uint32_t x = anchor.rect.x + 40; // Default offset from anchor
  uint32_t y = anchor.rect.y + 40;

  // Use provided x/y if specified, otherwise use relative positioning
  if (options.x != 0) {
    x = anchor.rect.x + options.x;
  }
  if (options.y != 0) {
    y = anchor.rect.y + options.y;
  }

  // Ensure window fits on screen
  ensure_window_fits(x, y, options.width, options.height, screen_w, screen_h);

  window::Window window = {};
  window.rect = Rect{x, y, options.width, options.height};
  window.title = options.title;
  window.minimized = options.minimized;
  window.maximized = options.maximized;
  window.fullscreen = options.fullscreen;
  window.resizable = options.resizable;
  window.movable = options.movable;
  window.draggable = options.draggable;
  window.closeable = options.closeable;
  window.focused = options.focused;
  window.always_on_top = options.always_on_top;
  window.user_data = options.user_data;
  window.draw_content = options.draw_content;
  window.on_mouse = options.on_mouse;

  return window;
}

window::Window create_centered_window(
    uint32_t screen_w, uint32_t screen_h, const char *title, uint32_t width,
    uint32_t height,
    void (*draw_func)(Graphics &gfx, const Rect &content_rect, void *user_data),
    void *user_data) {
  WindowOptions options;
  options.title = title;
  options.width = width;
  options.height = height;
  options.x = 0; // Auto-center
  options.y = 0; // Auto-center
  options.draw_content = draw_func;
  options.user_data = user_data;

  return create_window(screen_w, screen_h, options);
}

window::Window create_dialog_window(
    uint32_t screen_w, uint32_t screen_h, const char *title, uint32_t width,
    uint32_t height,
    void (*draw_func)(Graphics &gfx, const Rect &content_rect, void *user_data),
    void *user_data) {
  WindowOptions options;
  options.title = title;
  options.width = width;
  options.height = height;
  options.x = 0; // Auto-center
  options.y = 0; // Auto-center
  options.draw_content = draw_func;
  options.user_data = user_data;
  options.always_on_top = true; // Dialogs typically stay on top
  options.focused = true;       // Dialogs typically get focus

  return create_window(screen_w, screen_h, options);
}

} // namespace ui::window_manager
