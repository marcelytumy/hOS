#include "apps/welcome.hpp"
#include "font.hpp"
#include "graphics.hpp"

namespace ui::apps::welcome {

static void draw(Graphics &gfx, const ui::Rect &r, void * /*ud*/) {
  gfx.draw_string("This is a placeholder window.", r.x, r.y, 0xCCCCCC,
                  default_font);
}

bool create_window(uint32_t screen_w, uint32_t screen_h,
                   ui::window::Window &out_window) {
  // Default centered Welcome window size
  const uint32_t taskbar_h =
      0; // caller decides exact taskbar; we center simply
  uint32_t win_w = (screen_w > 40 ? (screen_w - 40) * 3 / 5 : screen_w);
  uint32_t win_h =
      (screen_h > taskbar_h + 20 ? (screen_h - taskbar_h - 20) * 3 / 5
                                 : screen_h);
  if (win_w < 320)
    win_w = 320;
  if (win_h < 200)
    win_h = 200;
  const uint32_t win_x = (screen_w > win_w ? (screen_w - win_w) / 2 : 0);
  const uint32_t win_y =
      (screen_h > taskbar_h + win_h ? (screen_h - taskbar_h - win_h) / 2 : 0);

  out_window = {};
  out_window.rect = ui::Rect{win_x, win_y, win_w, win_h};
  out_window.title = "Welcome to hOS";
  out_window.minimized = false;
  out_window.maximized = false;
  out_window.fullscreen = false;
  out_window.resizable = true;
  out_window.movable = true;
  out_window.draggable = true;
  out_window.closeable = true;
  out_window.focused = false;
  out_window.always_on_top = false;
  out_window.user_data = nullptr;
  out_window.draw_content = &draw;
  return true;
}

} // namespace ui::apps::welcome
