#include "../include/ui.hpp"
#include "../include/taskbar.hpp"
#include "../include/window.hpp"
#include "font.hpp"
#include "graphics.hpp"
#include <cstdint>

namespace ui {

static inline uint32_t clamp_u32(uint32_t value, uint32_t lo, uint32_t hi) {
  return value < lo ? lo : (value > hi ? hi : value);
}

// Colors in 0xRRGGBB
static constexpr uint32_t kDesktopBg = 0x1E1E1E;
static constexpr uint32_t kTaskbarBg = 0x2B2B2B;
static constexpr uint32_t kTaskbarBorder = 0x3A3A3A;
static constexpr uint32_t kWindowBg = 0x262626;
static constexpr uint32_t kWindowBorder = 0x4A4A4A;
static constexpr uint32_t kTitlebarBg = 0x333333;
static constexpr uint32_t kTitleText = 0xFFFFFF;

static void draw_taskbar(Graphics &gfx, uint32_t screen_w, uint32_t screen_h) {
  const uint32_t taskbar_h = clamp_u32(screen_h / 18, 32, 64);
  const uint32_t y = screen_h - taskbar_h;
  gfx.fill_rect(0, y, screen_w, taskbar_h, kTaskbarBg);
  gfx.draw_rect(0, y, screen_w, taskbar_h, kTaskbarBorder);

  // Simple start label
  const char *label = "Start";
  uint32_t padding = 8;
  gfx.fill_rect(padding, y + 6, 80, taskbar_h - 12, 0x3B3B3B);
  gfx.draw_rect(padding, y + 6, 80, taskbar_h - 12, 0x5A5A5A);
  gfx.draw_string(label, padding + 10,
                  y + (taskbar_h / 2) - (default_font.char_height / 2),
                  kTitleText, default_font);
}

void draw_desktop(Graphics &gfx) {
  const uint32_t screen_w = gfx.get_width();
  const uint32_t screen_h = gfx.get_height();

  // Background
  gfx.clear_screen(kDesktopBg);

  // Taskbar
  draw_taskbar(gfx, screen_w, screen_h);

  // Center window
  const uint32_t taskbar_h = clamp_u32(screen_h / 18, 32, 64);
  const uint32_t usable_h = screen_h - taskbar_h - 20; // top/bottom margins
  const uint32_t usable_w = screen_w - 40;             // side margins
  uint32_t win_w = usable_w * 3 / 5;
  uint32_t win_h = usable_h * 3 / 5;
  if (win_w < 320)
    win_w = 320;
  if (win_h < 200)
    win_h = 200;
  const uint32_t win_x = (screen_w - win_w) / 2;
  const uint32_t win_y = (screen_h - taskbar_h - win_h) / 2;

  // Frame
  gfx.fill_rect(win_x, win_y, win_w, win_h, kWindowBg);
  gfx.draw_rect(win_x, win_y, win_w, win_h, kWindowBorder);

  // Titlebar
  const uint32_t title_h = 28;
  gfx.fill_rect(win_x + 1, win_y + 1, win_w - 2, title_h, kTitlebarBg);

  // Title text
  gfx.draw_string("Welcome to hOS", win_x + 12, win_y + 6, kTitleText,
                  default_font);

  // Content stub
  gfx.draw_string("This is a placeholder window.", win_x + 12,
                  win_y + title_h + 12, 0xCCCCCC, default_font);
}

static void draw_window_rect(Graphics &gfx, const Rect &r) {
  // Frame
  gfx.fill_rect(r.x, r.y, r.w, r.h, kWindowBg);
  gfx.draw_rect(r.x, r.y, r.w, r.h, kWindowBorder);

  // Titlebar
  const uint32_t title_h = 28;
  gfx.fill_rect(r.x + 1, r.y + 1, r.w - 2, title_h, kTitlebarBg);

  // Title text
  gfx.draw_string("Welcome to hOS", r.x + 12, r.y + 6, kTitleText,
                  default_font);

  // Content stub
  gfx.draw_string("This is a placeholder window.", r.x + 12, r.y + title_h + 12,
                  0xCCCCCC, default_font);
}

void draw_desktop(Graphics &gfx, const Rect &window_rect) {
  const uint32_t screen_w = gfx.get_width();
  const uint32_t screen_h = gfx.get_height();

  // Background
  gfx.clear_screen(kDesktopBg);

  // Taskbar
  draw_taskbar(gfx, screen_w, screen_h);

  // Window at provided rect
  draw_window_rect(gfx, window_rect);
}

void draw_desktop_layer(Graphics &gfx, RenderLayer layer,
                        const window::Window *windows, uint32_t count) {
  const uint32_t screen_w = gfx.get_width();
  const uint32_t screen_h = gfx.get_height();

  if (layer == RenderLayer::Background) {
    gfx.clear_screen(kDesktopBg);
    return;
  }

  if (layer == RenderLayer::Taskbar) {
    taskbar::draw(gfx, screen_w, screen_h, windows, count);
    return;
  }

  // Determine focused window
  int32_t focused_idx = -1;
  for (uint32_t i = 0; i < count; ++i) {
    if (windows[i].focused) {
      focused_idx = static_cast<int32_t>(i);
      break;
    }
  }

  if (layer == RenderLayer::WindowsBack) {
    for (uint32_t i = 0; i < count; ++i) {
      const window::Window &w = windows[i];
      if (w.minimized)
        continue;
      if (w.always_on_top)
        continue;
      if (static_cast<int32_t>(i) == focused_idx)
        continue;
      window::draw(gfx, w);
    }
    return;
  }

  if (layer == RenderLayer::WindowsFocused) {
    if (focused_idx >= 0) {
      const window::Window &fw = windows[focused_idx];
      if (!fw.minimized && !fw.always_on_top)
        window::draw(gfx, fw);
    }
    return;
  }

  if (layer == RenderLayer::WindowsTop) {
    for (uint32_t i = 0; i < count; ++i) {
      const window::Window &w = windows[i];
      if (w.minimized)
        continue;
      if (!w.always_on_top)
        continue;
      if (static_cast<int32_t>(i) == focused_idx)
        continue;
      window::draw(gfx, w);
    }
    return;
  }

  if (layer == RenderLayer::WindowsTopFocused) {
    if (focused_idx >= 0) {
      const window::Window &fw = windows[focused_idx];
      if (!fw.minimized && fw.always_on_top)
        window::draw(gfx, fw);
    }
    return;
  }

  // Overlay layer could draw start menu etc. Currently drawn by callers.
  if (layer == RenderLayer::Cursor) {
    // Cursor is drawn outside of this function by callers currently.
    // Placeholder to keep layer order consistent.
    return;
  }
}

void draw_desktop(Graphics &gfx, const window::Window *windows,
                  uint32_t count) {
  const uint32_t screen_w = gfx.get_width();
  const uint32_t screen_h = gfx.get_height();

  // Background
  draw_desktop_layer(gfx, RenderLayer::Background, windows, count);

  // Determine fullscreen presence to possibly short-circuit like before
  int32_t focused_fs_idx = -1;
  int32_t last_fs_idx = -1;
  for (uint32_t i = 0; i < count; ++i) {
    const window::Window &w = windows[i];
    if (!w.minimized && w.fullscreen) {
      last_fs_idx = static_cast<int32_t>(i);
      if (w.focused)
        focused_fs_idx = static_cast<int32_t>(i);
    }
  }

  if (focused_fs_idx >= 0 || last_fs_idx >= 0) {
    int32_t idx = (focused_fs_idx >= 0) ? focused_fs_idx : last_fs_idx;
    window::draw(gfx, windows[idx]);
    return;
  }

  // Layered draw order with no fullscreen window present
  (void)screen_w;
  (void)screen_h;
  draw_desktop_layer(gfx, RenderLayer::Taskbar, windows, count);
  draw_desktop_layer(gfx, RenderLayer::WindowsBack, windows, count);
  draw_desktop_layer(gfx, RenderLayer::WindowsFocused, windows, count);
  draw_desktop_layer(gfx, RenderLayer::WindowsTop, windows, count);
  draw_desktop_layer(gfx, RenderLayer::WindowsTopFocused, windows, count);
}

// Region rendering removed

uint32_t get_taskbar_height(uint32_t screen_h) {
  return clamp_u32(screen_h / 18, 32, 64);
}

uint32_t get_titlebar_height() { return 28; }

bool point_in_titlebar(const Rect &window_rect, uint32_t x, uint32_t y) {
  const uint32_t title_h = get_titlebar_height();
  return x >= window_rect.x + 1 && x < window_rect.x + window_rect.w - 1 &&
         y >= window_rect.y + 1 && y < window_rect.y + 1 + title_h;
}

} // namespace ui