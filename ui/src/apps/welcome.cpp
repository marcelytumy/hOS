#include "apps/welcome.hpp"
#include "font.hpp"
#include "graphics.hpp"

namespace ui::apps::welcome {

static void draw(Graphics &gfx, const ui::Rect &r, void * /*ud*/) {
  gfx.draw_string("This is a placeholder window.", r.x, r.y, 0xCCCCCC,
                  default_font);
}

ui::window::Window create_window(uint32_t screen_w, uint32_t screen_h) {
  // Calculate window size based on screen size
  uint32_t win_w = (screen_w > 40 ? (screen_w - 40) * 3 / 5 : screen_w);
  uint32_t win_h = (screen_h > 20 ? (screen_h - 20) * 3 / 5 : screen_h);

  if (win_w < 320)
    win_w = 320;
  if (win_h < 200)
    win_h = 200;

  return ui::window_manager::create_centered_window(
      screen_w, screen_h, "Welcome to hOS", win_w, win_h, &draw);
}

} // namespace ui::apps::welcome
