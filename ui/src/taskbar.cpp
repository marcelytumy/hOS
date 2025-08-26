#include "../include/taskbar.hpp"
#include "../include/time.hpp"
#include "../include/ui.hpp"
#include "../include/window.hpp"
#include "font.hpp"
#include "graphics.hpp"

namespace ui {
namespace taskbar {

static constexpr uint32_t kTaskbarBg = 0x2B2B2B;
static constexpr uint32_t kTaskbarBorder = 0x3A3A3A;
static constexpr uint32_t kButtonBg = 0x3B3B3B;
static constexpr uint32_t kButtonBorder = 0x5A5A5A;
static constexpr uint32_t kButtonText = 0xFFFFFF;
static constexpr uint32_t kButtonBgFocused = 0x4A4A4A;
static constexpr uint32_t kButtonBorderFocused = 0x7A7A7A;
static constexpr uint32_t kButtonTextFocused = 0xFFFFFF;

uint32_t height(uint32_t screen_h) {
  return (screen_h / 18 < 32 ? 32 : (screen_h / 18 > 64 ? 64 : screen_h / 18));
}

void draw(Graphics &gfx, uint32_t screen_w, uint32_t screen_h,
          const window::Window *windows, uint32_t count) {
  const uint32_t h = height(screen_h);
  const uint32_t y = screen_h - h;
  gfx.fill_rect(0, y, screen_w, h, kTaskbarBg);
  gfx.draw_rect(0, y, screen_w, h, kTaskbarBorder);

  // Start area
  uint32_t x = 8;
  const uint32_t start_w = 80;
  gfx.fill_rect(x, y + 6, start_w, h - 12, kButtonBg);
  gfx.draw_rect(x, y + 6, start_w, h - 12, kButtonBorder);
  gfx.draw_string("Start", x + 10, y + (h / 2) - (default_font.char_height / 2),
                  kButtonText, default_font);
  x += start_w + 8;

  // Reserve a right-side area for date/time (placeholder content for now)
  const uint32_t char_w = default_font.char_width;
  const uint32_t char_h = default_font.char_height;
  uint32_t clock_w = 10u * char_w + 20u; // enough for YYYY-MM-DD plus padding
  if (clock_w < 100u)
    clock_w = 100u;
  uint32_t right_limit =
      (screen_w > (clock_w + 8u)) ? (screen_w - (clock_w + 8u)) : 0u;

  // Window buttons
  for (uint32_t i = 0; i < count; ++i) {
    const char *title = windows[i].title ? windows[i].title : "Window";
    uint32_t btn_w = 140;
    if (x + btn_w + 8 > right_limit)
      break;
    // Dim if minimized; highlight if focused
    bool is_focused = windows[i].focused;
    uint32_t bg = windows[i].minimized
                      ? 0x2E2E2E
                      : (is_focused ? kButtonBgFocused : kButtonBg);
    uint32_t border = windows[i].minimized
                          ? 0x4A4A4A
                          : (is_focused ? kButtonBorderFocused : kButtonBorder);
    uint32_t text = windows[i].minimized
                        ? 0xAAAAAA
                        : (is_focused ? kButtonTextFocused : kButtonText);
    gfx.fill_rect(x, y + 6, btn_w, h - 12, bg);
    gfx.draw_rect(x, y + 6, btn_w, h - 12, border);
    gfx.draw_string(title, x + 10, y + (h / 2) - (default_font.char_height / 2),
                    text, default_font);
    x += btn_w + 8;
  }

  // Draw the right-aligned date/time box
  if (screen_w > clock_w + 8u) {
    const uint32_t clock_x = screen_w - clock_w - 8u;
    const uint32_t inner_y = y + 6u;
    const uint32_t inner_h = h - 12u;
    gfx.fill_rect(clock_x, inner_y, clock_w, inner_h, kButtonBg);
    gfx.draw_rect(clock_x, inner_y, clock_w, inner_h, kButtonBorder);

    // Query platform time
    platform::DateTime dt{0, 0, 0, 0, 0, 0, false};
    bool ok = platform::get_current_datetime(dt);
    char time_buf[6] = {0};
    char date_buf[11] = {0};
    if (ok && dt.valid) {
      // HH:MM
      time_buf[0] = char('0' + ((dt.hour / 10) % 10));
      time_buf[1] = char('0' + (dt.hour % 10));
      time_buf[2] = ':';
      time_buf[3] = char('0' + ((dt.minute / 10) % 10));
      time_buf[4] = char('0' + (dt.minute % 10));
      time_buf[5] = '\0';
      // YYYY-MM-DD
      int yv = dt.year;
      date_buf[0] = char('0' + ((yv / 1000) % 10));
      date_buf[1] = char('0' + ((yv / 100) % 10));
      date_buf[2] = char('0' + ((yv / 10) % 10));
      date_buf[3] = char('0' + (yv % 10));
      date_buf[4] = '-';
      date_buf[5] = char('0' + ((dt.month / 10) % 10));
      date_buf[6] = char('0' + (dt.month % 10));
      date_buf[7] = '-';
      date_buf[8] = char('0' + ((dt.day / 10) % 10));
      date_buf[9] = char('0' + (dt.day % 10));
      date_buf[10] = '\0';
    } else {
      // Fallback placeholders
      time_buf[0] = '0';
      time_buf[1] = '0';
      time_buf[2] = ':';
      time_buf[3] = '0';
      time_buf[4] = '0';
      time_buf[5] = '\0';
      date_buf[0] = '1';
      date_buf[1] = '9';
      date_buf[2] = '7';
      date_buf[3] = '0';
      date_buf[4] = '-';
      date_buf[5] = '0';
      date_buf[6] = '1';
      date_buf[7] = '-';
      date_buf[8] = '0';
      date_buf[9] = '1';
      date_buf[10] = '\0';
    }

    const uint32_t content_h = (2u * char_h) + 2u;
    uint32_t text_top =
        inner_y + (inner_h > content_h ? (inner_h - content_h) / 2u : 0u);
    uint32_t time_w_px = 5u * char_w;
    uint32_t date_w_px = 10u * char_w;
    uint32_t time_x =
        clock_x + (clock_w > time_w_px ? (clock_w - time_w_px) / 2u : 0u);
    uint32_t date_x =
        clock_x + (clock_w > date_w_px ? (clock_w - date_w_px) / 2u : 0u);

    gfx.draw_string(time_buf, time_x, text_top, kButtonText, default_font);
    gfx.draw_string(date_buf, date_x, text_top + char_h + 2u, kButtonText,
                    default_font);
  }
}

uint32_t hit_test(uint32_t px, uint32_t py, uint32_t screen_w,
                  uint32_t screen_h, const window::Window * /*windows*/,
                  uint32_t count) {
  const uint32_t h = height(screen_h);
  const uint32_t y = screen_h - h;
  if (py < y)
    return UINT32_MAX;
  uint32_t x = 8; // start button
  const uint32_t start_w = 80;
  if (px >= x && px < x + start_w && py >= y + 6 && py < y + 6 + (h - 12))
    return UINT32_MAX;
  x += start_w + 8;

  // Exclude right-side date/time area from hit testing
  const uint32_t char_w = default_font.char_width;
  uint32_t clock_w = 10u * char_w + 20u;
  if (clock_w < 100u)
    clock_w = 100u;
  uint32_t right_limit =
      (screen_w > (clock_w + 8u)) ? (screen_w - (clock_w + 8u)) : 0u;
  if (screen_w > (clock_w + 8u)) {
    const uint32_t clock_x = screen_w - clock_w - 8u;
    if (px >= clock_x && px < clock_x + clock_w && py >= y + 6 &&
        py < y + 6 + (h - 12))
      return UINT32_MAX;
  }

  for (uint32_t i = 0; i < count; ++i) {
    uint32_t btn_w = 140;
    if (x + btn_w + 8 > right_limit)
      break;
    if (px >= x && px < x + btn_w && py >= y + 6 && py < y + 6 + (h - 12))
      return i;
    x += btn_w + 8;
  }
  return UINT32_MAX;
}

} // namespace taskbar
} // namespace ui
