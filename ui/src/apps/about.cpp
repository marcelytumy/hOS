#include "apps/about.hpp"
#include "font.hpp"
#include "graphics.hpp"
#include "window_manager.hpp"

namespace ui::apps::about {

static void draw(Graphics &gfx, const ui::Rect &r, void * /*ud*/) {
  gfx.draw_string("About hOS", r.x, r.y, 0xFFFFFF, default_font);
  gfx.draw_string("Simple demo OS UI", r.x, r.y + 18, 0xCCCCCC, default_font);
}

ui::window::Window create_window(uint32_t screen_w, uint32_t screen_h,
                                 const ui::window::Window &anchor) {
  ui::window_manager::WindowOptions options;
  options.title = "About";
  options.width = 280;
  options.height = 160;
  options.draw_content = &draw;

  if (anchor.rect.w > 0 && anchor.rect.h > 0) {
    // Position relative to anchor window
    return ui::window_manager::create_window_relative(screen_w, screen_h,
                                                      anchor, options);
  } else {
    // Create centered window
    return ui::window_manager::create_centered_window(screen_w, screen_h,
                                                      "About", 280, 160, &draw);
  }
}

} // namespace ui::apps::about
