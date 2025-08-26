#include "apps/about.hpp"
#include "font.hpp"
#include "graphics.hpp"

namespace ui {
namespace apps {
namespace about {

static void draw(Graphics &gfx, const ui::Rect &r, void * /*ud*/) {
  gfx.draw_string("About hOS", r.x, r.y, 0xFFFFFF, default_font);
  gfx.draw_string("Simple demo OS UI", r.x, r.y + 18, 0xCCCCCC, default_font);
}

bool create_window(uint32_t screen_w, uint32_t screen_h,
                   const ui::window::Window &anchor,
                   ui::window::Window &out_window) {
  uint32_t win_w = anchor.rect.w / 2;
  if (win_w < 280)
    win_w = 280;
  uint32_t win_h = anchor.rect.h / 2;
  if (win_h < 160)
    win_h = 160;
  uint32_t x = anchor.rect.x + 40;
  uint32_t y = anchor.rect.y + 40;
  if (x + win_w > screen_w)
    x = screen_w > win_w ? (screen_w - win_w) : 0;
  if (y + win_h > screen_h)
    y = screen_h > win_h ? (screen_h - win_h) : 0;

  out_window = {};
  out_window.rect = ui::Rect{x, y, win_w, win_h};
  out_window.title = "About";
  out_window.minimized = false;
  out_window.maximized = false;
  out_window.fullscreen = false;
  out_window.resizable = true;
  out_window.movable = true;
  out_window.draggable = true;
  out_window.closeable = true;
  out_window.focused = true;
  out_window.always_on_top = false;
  out_window.user_data = nullptr;
  out_window.draw_content = &draw;
  return true;
}

} // namespace about
} // namespace apps
} // namespace ui
