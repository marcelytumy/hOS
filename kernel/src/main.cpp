#include "../../input/include/mouse.hpp"
#include "../../ui/include/cursor.hpp"
#include "../../ui/include/taskbar.hpp"
#include "../../ui/include/ui.hpp"
#include "../../ui/include/window.hpp"
#include "font.hpp"
#include "graphics.hpp"
#include <cstddef>
#include <cstdint>
#include <limine.h>

// Set the base revision to 3, this is recommended as this is the latest
// base revision described by the Limine boot protocol specification.
// See specification for further info.

namespace {

__attribute__((used,
               section(".limine_requests"))) volatile LIMINE_BASE_REVISION(3);

}

// The Limine requests can be placed anywhere, but it is important that
// the compiler does not optimise them away, so, usually, they should
// be made volatile or equivalent, _and_ they should be accessed at least
// once or marked as used with the "used" attribute as done here.

namespace {

__attribute__((used,
               section(".limine_requests"))) volatile limine_framebuffer_request
    framebuffer_request = {
        .id = LIMINE_FRAMEBUFFER_REQUEST, .revision = 0, .response = nullptr};

}

// Finally, define the start and end markers for the Limine requests.
// These can also be moved anywhere, to any .cpp file, as seen fit.

namespace {

__attribute__((
    used,
    section(".limine_requests_start"))) volatile LIMINE_REQUESTS_START_MARKER;

__attribute__((
    used, section(".limine_requests_end"))) volatile LIMINE_REQUESTS_END_MARKER;

} // namespace

// GCC and Clang reserve the right to generate calls to the following
// 4 functions even if they are not directly called.
// Implement them as the C specification mandates.
// DO NOT remove or rename these functions, or stuff will eventually break!
// They CAN be moved to a different .cpp file.

extern "C" {

void *memcpy(void *__restrict dest, const void *__restrict src, std::size_t n) {
  std::uint8_t *__restrict pdest = static_cast<std::uint8_t *__restrict>(dest);
  const std::uint8_t *__restrict psrc =
      static_cast<const std::uint8_t *__restrict>(src);

  for (std::size_t i = 0; i < n; i++) {
    pdest[i] = psrc[i];
  }

  return dest;
}

void *memset(void *s, int c, std::size_t n) {
  std::uint8_t *p = static_cast<std::uint8_t *>(s);

  for (std::size_t i = 0; i < n; i++) {
    p[i] = static_cast<uint8_t>(c);
  }

  return s;
}

void *memmove(void *dest, const void *src, std::size_t n) {
  std::uint8_t *pdest = static_cast<std::uint8_t *>(dest);
  const std::uint8_t *psrc = static_cast<const std::uint8_t *>(src);

  if (src > dest) {
    for (std::size_t i = 0; i < n; i++) {
      pdest[i] = psrc[i];
    }
  } else if (src < dest) {
    for (std::size_t i = n; i > 0; i--) {
      pdest[i - 1] = psrc[i - 1];
    }
  }

  return dest;
}

int memcmp(const void *s1, const void *s2, std::size_t n) {
  const std::uint8_t *p1 = static_cast<const std::uint8_t *>(s1);
  const std::uint8_t *p2 = static_cast<const std::uint8_t *>(s2);

  for (std::size_t i = 0; i < n; i++) {
    if (p1[i] != p2[i]) {
      return p1[i] < p2[i] ? -1 : 1;
    }
  }

  return 0;
}
}

// Halt and catch fire function.
namespace {

void hcf() {
  for (;;) {
#if defined(__x86_64__)
    asm("hlt");
#elif defined(__aarch64__) || defined(__riscv)
    asm("wfi");
#elif defined(__loongarch64)
    asm("idle 0");
#endif
  }
}

} // namespace

// The following stubs are required by the Itanium C++ ABI (the one we use,
// regardless of the "Itanium" nomenclature).
// Like the memory functions above, these stubs can be moved to a different .cpp
// file, but should not be removed, unless you know what you are doing.
extern "C" {
int __cxa_atexit(void (*)(void *), void *, void *) { return 0; }
void __cxa_pure_virtual() { hcf(); }
void *__dso_handle;
}

// Extern declarations for global constructors array.
extern void (*__init_array[])();
extern void (*__init_array_end[])();

// The following will be our kernel's entry point.
// If renaming kmain() to something else, make sure to change the
// linker script accordingly.
extern "C" void kmain() {
  // Ensure the bootloader actually understands our base revision (see spec).
  if (LIMINE_BASE_REVISION_SUPPORTED == false) {
    hcf();
  }

  // Call global constructors.
  for (std::size_t i = 0; &__init_array[i] != __init_array_end; i++) {
    __init_array[i]();
  }

  // Ensure we got a framebuffer.
  if (framebuffer_request.response == nullptr ||
      framebuffer_request.response->framebuffer_count < 1) {
    hcf();
  }

  // Fetch the first framebuffer.
  limine_framebuffer *framebuffer =
      framebuffer_request.response->framebuffers[0];

  // Initialize graphics system
  Graphics graphics(framebuffer);

  // Clear screen to white
  graphics.clear_screen(0x000000);

  // Draw centered, scaled text below the logo
  graphics.draw_string_centered_scaled("hOS 0.1", framebuffer->height / 2,
                                       0xFFFFFF, default_font, 4);

  // Draw loading bar outline
  int bar_x = framebuffer->width / 2 - 100;
  int bar_y = framebuffer->height / 2 + 50;
  int bar_width = 200;
  int bar_height = 20;

  graphics.draw_rect(bar_x, bar_y, bar_width, bar_height, 0xFFFFFF);

  // Animate loading bar fill
  for (int fill = 0; fill <= bar_width; fill++) {
    // Fill the bar up to 'fill' width
    graphics.fill_rect(bar_x, bar_y, fill, bar_height, 0xFFFFFF);

    // Simple delay loop (not precise, but enough for a visible animation)
    for (volatile int d = 0; d < 1000000; ++d) {
    }
  }

  // Enable double-buffering backbuffer
  static uint32_t backbuffer_storage[1920 * 1080];
  graphics.enable_backbuffer(backbuffer_storage, 1920u * 1080u);

  // Compute initial centered window rect
  const uint32_t screen_w = graphics.get_width();
  const uint32_t screen_h = graphics.get_height();
  const uint32_t taskbar_h = ui::taskbar::height(screen_h);
  const uint32_t usable_h = screen_h - taskbar_h - 20;
  const uint32_t usable_w = screen_w - 40;
  uint32_t win_w = (usable_w * 3) / 5;
  uint32_t win_h = (usable_h * 3) / 5;
  if (win_w < 320)
    win_w = 320;
  if (win_h < 200)
    win_h = 200;
  // Create initial windows
  ui::window::Window windows[2];
  windows[0].rect = ui::Rect{(screen_w - win_w) / 2,
                             (screen_h - taskbar_h - win_h) / 2, win_w, win_h};
  windows[0].title = "Welcome to hOS";
  windows[0].minimized = false;
  windows[0].maximized = false;
  windows[0].fullscreen = false;
  windows[0].resizable = true;
  windows[0].movable = true;
  windows[0].draggable = true;
  windows[0].closeable = true;
  windows[0].focused = false;
  windows[0].always_on_top = false;
  // Second window (smaller)
  uint32_t win2_w = win_w / 2;
  if (win2_w < 280)
    win2_w = 280;
  uint32_t win2_h = win_h / 2;
  if (win2_h < 160)
    win2_h = 160;
  windows[1].rect =
      ui::Rect{windows[0].rect.x + 40, windows[0].rect.y + 40, win2_w, win2_h};
  windows[1].title = "About";
  windows[1].minimized = false;
  windows[1].maximized = false;
  windows[1].fullscreen = false;
  windows[1].resizable = true;
  windows[1].movable = true;
  windows[1].draggable = true;
  windows[1].closeable = true;
  windows[1].focused = true; // topmost initially
  windows[1].always_on_top = false;
  uint32_t window_count = 2;

  // Saved rects for restore after maximize
  ui::Rect saved_rects[16];
  bool saved_rect_valid[16];
  for (uint32_t i = 0; i < 16; ++i)
    saved_rect_valid[i] = false;

  // Draw desktop with windows (to backbuffer), then present
  ui::draw_desktop(graphics, windows, window_count);
  graphics.present();

  // Initialize mouse and cursor
  input::Ps2Mouse mouse;
  mouse.initialize();
  ui::Cursor cursor;
  cursor.set_position(screen_w / 2, screen_h / 2, screen_w, screen_h);
  cursor.draw(graphics);
  graphics.present();

  bool dragging = false;
  bool resizing = false;
  uint32_t resize_mask = 0;
  uint32_t drag_off_x = 0;
  uint32_t drag_off_y = 0;
  int dragging_index = -1;
  bool prev_left = false;
  bool perf_border_only =
      false; // performance-over-visuals flag (future setting)

  // Simple event loop: poll mouse, move cursor, support window dragging
  for (;;) {
    int8_t dx = 0, dy = 0;
    bool left = false, right = false, middle = false;
    if (!mouse.poll_packet(dx, dy, left, right, middle)) {
      continue;
    }

    // Track dirty regions for partial redraw/present
    bool ui_changed = false;
    ui::Rect ui_dirty{0, 0, 0, 0};
    auto add_dirty = [&](const ui::Rect &r) {
      if (r.w == 0 || r.h == 0)
        return;
      if (ui_dirty.w == 0 || ui_dirty.h == 0) {
        ui_dirty = r;
      } else {
        uint32_t x0 = ui_dirty.x < r.x ? ui_dirty.x : r.x;
        uint32_t y0 = ui_dirty.y < r.y ? ui_dirty.y : r.y;
        uint32_t x1a = ui_dirty.x + ui_dirty.w;
        uint32_t y1a = ui_dirty.y + ui_dirty.h;
        uint32_t x1b = r.x + r.w;
        uint32_t y1b = r.y + r.h;
        uint32_t x1 = x1a > x1b ? x1a : x1b;
        uint32_t y1 = y1a > y1b ? y1a : y1b;
        ui_dirty.x = x0;
        ui_dirty.y = y0;
        ui_dirty.w = x1 - x0;
        ui_dirty.h = y1 - y0;
      }
      ui_changed = true;
    };

    bool cursor_erased = false;
    ui::Rect cursor_dirty{0, 0, 0, 0};
    const bool cursor_moved = (dx != 0) || (dy != 0);
    uint32_t prev_cx = cursor.x();
    uint32_t prev_cy = cursor.y();
    if (cursor_moved) {
      cursor.erase(graphics);
      cursor_erased = true;
      cursor.move_by(dx, dy, screen_w, screen_h);
      // Dirty rect covering old and new cursor pixels
      uint32_t x0 = prev_cx < cursor.x() ? prev_cx : cursor.x();
      uint32_t y0 = prev_cy < cursor.y() ? prev_cy : cursor.y();
      uint32_t x1 = (prev_cx > cursor.x() ? prev_cx : cursor.x()) + 1;
      uint32_t y1 = (prev_cy > cursor.y() ? prev_cy : cursor.y()) + 1;
      cursor_dirty.x = x0;
      cursor_dirty.y = y0;
      cursor_dirty.w = x1 - x0;
      cursor_dirty.h = y1 - y0;
    }

    // Handle drag begin/end and taskbar clicks
    if (left && !prev_left) {
      // Check taskbar click first
      uint32_t tb_hit = ui::taskbar::hit_test(cursor.x(), cursor.y(), screen_w,
                                              screen_h, windows, window_count);
      if (tb_hit != UINT32_MAX && tb_hit < window_count) {
        // Toggle minimize; if restoring, bring to front by moving to end
        bool was_min = windows[tb_hit].minimized;
        windows[tb_hit].minimized = !windows[tb_hit].minimized;
        ui::Rect affected = windows[tb_hit].rect;
        if (was_min) {
          // bring to front
          ui::window::Window tmp = windows[tb_hit];
          for (uint32_t i = tb_hit; i + 1 < window_count; ++i)
            windows[i] = windows[i + 1];
          windows[window_count - 1] = tmp;
          // focus restored window
          for (uint32_t j = 0; j < window_count; ++j)
            windows[j].focused = (j == window_count - 1);
        }
        // Dirty: the window area and the taskbar band
        add_dirty(ui::Rect{affected.x, affected.y, affected.w, affected.h});
        add_dirty(ui::Rect{0, screen_h - taskbar_h, screen_w, taskbar_h});
      } else {
        // Click on windows from topmost to bottom: handle buttons first, then
        // drag
        bool handled = false;
        for (int i = static_cast<int>(window_count) - 1; i >= 0; --i) {
          if (windows[i].minimized)
            continue;
          // First try resize edges
          uint32_t hm =
              ui::window::hit_test_resize(windows[i], cursor.x(), cursor.y());
          if (hm != 0) {
            resizing = true;
            dragging_index = i;
            resize_mask = hm;
            drag_off_x = cursor.x() - windows[i].rect.x;
            drag_off_y = cursor.y() - windows[i].rect.y;
            // Bring to front
            if (static_cast<uint32_t>(i) != window_count - 1) {
              ui::window::Window tmp = windows[i];
              for (uint32_t k = i; k + 1 < window_count; ++k)
                windows[k] = windows[k + 1];
              windows[window_count - 1] = tmp;
              dragging_index = window_count - 1;
            }
            for (uint32_t j = 0; j < window_count; ++j)
              windows[j].focused = (j == static_cast<uint32_t>(dragging_index));
            handled = true;
            break;
          }
          uint32_t btn =
              ui::window::hit_test_button(windows[i], cursor.x(), cursor.y());
          if (btn != UINT32_MAX) {
            // Focus this window
            for (uint32_t j = 0; j < window_count; ++j)
              windows[j].focused = (j == static_cast<uint32_t>(i));
            // Handle button action
            if (btn == 0 && windows[i].closeable) {
              // Close window: remove from array
              ui::Rect oldr = windows[i].rect;
              for (uint32_t k = i; k + 1 < window_count; ++k) {
                windows[k] = windows[k + 1];
                saved_rects[k] = saved_rects[k + 1];
                saved_rect_valid[k] = saved_rect_valid[k + 1];
              }
              window_count--;
              add_dirty(ui::Rect{oldr.x, oldr.y, oldr.w, oldr.h});
              add_dirty(ui::Rect{0, screen_h - taskbar_h, screen_w, taskbar_h});
            } else if (btn == 1) {
              // Minimize
              bool was_min_local = windows[i].minimized;
              windows[i].minimized = !windows[i].minimized;
              add_dirty(ui::Rect{windows[i].rect.x, windows[i].rect.y,
                                 windows[i].rect.w, windows[i].rect.h});
              add_dirty(ui::Rect{0, screen_h - taskbar_h, screen_w, taskbar_h});
              if (!was_min_local) {
                // If minimized the focused window, clear focus
                if (windows[i].focused)
                  windows[i].focused = false;
              }
            } else if (btn == 2) {
              // Maximize/restore
              ui::Rect oldr = windows[i].rect;
              if (!windows[i].maximized) {
                saved_rects[i] = windows[i].rect;
                saved_rect_valid[i] = true;
                windows[i].maximized = true;
                windows[i].fullscreen = false;
                windows[i].rect.x = 0;
                windows[i].rect.y = 0;
                windows[i].rect.w = screen_w;
                windows[i].rect.h = screen_h - taskbar_h;
              } else {
                windows[i].maximized = false;
                if (saved_rect_valid[i]) {
                  windows[i].rect = saved_rects[i];
                  saved_rect_valid[i] = false;
                }
              }
              // bring to front
              if (static_cast<uint32_t>(i) != window_count - 1) {
                ui::window::Window tmp = windows[i];
                ui::Rect sr = saved_rects[i];
                bool srv = saved_rect_valid[i];
                for (uint32_t k = i; k + 1 < window_count; ++k) {
                  windows[k] = windows[k + 1];
                  saved_rects[k] = saved_rects[k + 1];
                  saved_rect_valid[k] = saved_rect_valid[k + 1];
                }
                windows[window_count - 1] = tmp;
                saved_rects[window_count - 1] = sr;
                saved_rect_valid[window_count - 1] = srv;
              }
              for (uint32_t j = 0; j < window_count; ++j)
                windows[j].focused = (j == window_count - 1);
              // Dirty union
              uint32_t rx0 = oldr.x < windows[window_count - 1].rect.x
                                 ? oldr.x
                                 : windows[window_count - 1].rect.x;
              uint32_t ry0 = oldr.y < windows[window_count - 1].rect.y
                                 ? oldr.y
                                 : windows[window_count - 1].rect.y;
              uint32_t rx1 =
                  (oldr.x + oldr.w) > (windows[window_count - 1].rect.x +
                                       windows[window_count - 1].rect.w)
                      ? (oldr.x + oldr.w)
                      : (windows[window_count - 1].rect.x +
                         windows[window_count - 1].rect.w);
              uint32_t ry1 =
                  (oldr.y + oldr.h) > (windows[window_count - 1].rect.y +
                                       windows[window_count - 1].rect.h)
                      ? (oldr.y + oldr.h)
                      : (windows[window_count - 1].rect.y +
                         windows[window_count - 1].rect.h);
              add_dirty(ui::Rect{rx0, ry0, rx1 - rx0, ry1 - ry0});
              add_dirty(ui::Rect{0, screen_h - taskbar_h, screen_w, taskbar_h});
            } else if (btn == 3) {
              // Toggle always_on_top (pin)
              windows[i].always_on_top = !windows[i].always_on_top;
              // bring to front for interaction consistency
              if (static_cast<uint32_t>(i) != window_count - 1) {
                ui::window::Window tmp = windows[i];
                ui::Rect sr = saved_rects[i];
                bool srv = saved_rect_valid[i];
                for (uint32_t k = i; k + 1 < window_count; ++k) {
                  windows[k] = windows[k + 1];
                  saved_rects[k] = saved_rects[k + 1];
                  saved_rect_valid[k] = saved_rect_valid[k + 1];
                }
                windows[window_count - 1] = tmp;
                saved_rects[window_count - 1] = sr;
                saved_rect_valid[window_count - 1] = srv;
              }
              for (uint32_t j = 0; j < window_count; ++j)
                windows[j].focused = (j == window_count - 1);
              add_dirty(ui::Rect{windows[window_count - 1].rect.x,
                                 windows[window_count - 1].rect.y,
                                 windows[window_count - 1].rect.w,
                                 windows[window_count - 1].rect.h});
            }
            handled = true;
            break;
          }
        }
        if (!handled) {
          // Check for titlebar drag
          for (int i = static_cast<int>(window_count) - 1; i >= 0; --i) {
            if (windows[i].minimized)
              continue;
            if (ui::window::point_in_titlebar(windows[i], cursor.x(),
                                              cursor.y())) {
              dragging = true;
              dragging_index = i;
              drag_off_x = cursor.x() - windows[i].rect.x;
              drag_off_y = cursor.y() - windows[i].rect.y;
              // bring to front if not already
              if (static_cast<uint32_t>(i) != window_count - 1) {
                ui::window::Window tmp = windows[i];
                ui::Rect sr = saved_rects[i];
                bool srv = saved_rect_valid[i];
                for (uint32_t k = i; k + 1 < window_count; ++k) {
                  windows[k] = windows[k + 1];
                  saved_rects[k] = saved_rects[k + 1];
                  saved_rect_valid[k] = saved_rect_valid[k + 1];
                }
                windows[window_count - 1] = tmp;
                saved_rects[window_count - 1] = sr;
                saved_rect_valid[window_count - 1] = srv;
                dragging_index = window_count - 1;
                add_dirty(ui::Rect{windows[dragging_index].rect.x,
                                   windows[dragging_index].rect.y,
                                   windows[dragging_index].rect.w,
                                   windows[dragging_index].rect.h});
              }
              for (uint32_t j = 0; j < window_count; ++j)
                windows[j].focused =
                    (j == static_cast<uint32_t>(dragging_index));
              break;
            }
          }
        }
      }
    } else if (!left && prev_left) {
      bool was_dragging = dragging;
      bool was_resizing = resizing;
      dragging = false;
      if (resizing) {
        // Finalize resize
        resizing = false;
        resize_mask = 0;
      }
      dragging_index = -1;
      // If we were in perf-border-only mode and just finished an interaction,
      // trigger a full redraw so the window contents are rendered.
      if (perf_border_only && (was_dragging || was_resizing)) {
        ui_changed = true;
        ui_dirty = ui::Rect{0, 0, screen_w, screen_h};
      }
    }

    // Update window position if dragging
    if (dragging && dragging_index >= 0) {
      ui::window::Window &w = windows[dragging_index];
      uint32_t old_x = w.rect.x;
      uint32_t old_y = w.rect.y;
      int64_t new_x =
          static_cast<int64_t>(cursor.x()) - static_cast<int64_t>(drag_off_x);
      int64_t new_y =
          static_cast<int64_t>(cursor.y()) - static_cast<int64_t>(drag_off_y);
      if (new_x < 0)
        new_x = 0;
      if (new_y < 0)
        new_y = 0;
      if (new_x > static_cast<int64_t>(screen_w - w.rect.w))
        new_x = screen_w - w.rect.w;
      uint32_t bottom_limit = screen_h - taskbar_h - w.rect.h;
      if (new_y > static_cast<int64_t>(bottom_limit))
        new_y = bottom_limit;
      uint32_t nx = static_cast<uint32_t>(new_x);
      uint32_t ny = static_cast<uint32_t>(new_y);
      if (nx != old_x || ny != old_y) {
        w.rect.x = nx;
        w.rect.y = ny;
        // Dirty region is union of old and new window rects
        uint32_t rx0 = old_x < nx ? old_x : nx;
        uint32_t ry0 = old_y < ny ? old_y : ny;
        uint32_t rx1 = (old_x + w.rect.w) > (nx + w.rect.w) ? (old_x + w.rect.w)
                                                            : (nx + w.rect.w);
        uint32_t ry1 = (old_y + w.rect.h) > (ny + w.rect.h) ? (old_y + w.rect.h)
                                                            : (ny + w.rect.h);
        add_dirty(ui::Rect{rx0, ry0, rx1 - rx0, ry1 - ry0});
      }
    }

    // Update window size if resizing
    if (resizing && dragging_index >= 0) {
      ui::window::Window &w = windows[dragging_index];
      uint32_t old_x = w.rect.x;
      uint32_t old_y = w.rect.y;
      uint32_t old_w = w.rect.w;
      uint32_t old_h = w.rect.h;

      int64_t nx = w.rect.x;
      int64_t ny = w.rect.y;
      int64_t nw = w.rect.w;
      int64_t nh = w.rect.h;

      const int64_t min_w = 160;
      const int64_t min_h = 120;

      if (resize_mask & ui::window::ResizeLeft) {
        int64_t new_left = static_cast<int64_t>(cursor.x());
        if (new_left < 0)
          new_left = 0;
        if (new_left > static_cast<int64_t>(w.rect.x + w.rect.w) - min_w)
          new_left = static_cast<int64_t>(w.rect.x + w.rect.w) - min_w;
        nw = (w.rect.x + w.rect.w) - new_left;
        nx = new_left;
      }
      if (resize_mask & ui::window::ResizeRight) {
        int64_t new_right = static_cast<int64_t>(cursor.x());
        if (new_right > static_cast<int64_t>(screen_w))
          new_right = screen_w;
        nw = new_right - static_cast<int64_t>(w.rect.x);
        if (nw < min_w)
          nw = min_w;
      }
      if (resize_mask & ui::window::ResizeTop) {
        int64_t new_top = static_cast<int64_t>(cursor.y());
        if (new_top < 0)
          new_top = 0;
        if (new_top > static_cast<int64_t>(w.rect.y + w.rect.h) - min_h)
          new_top = static_cast<int64_t>(w.rect.y + w.rect.h) - min_h;
        nh = (w.rect.y + w.rect.h) - new_top;
        ny = new_top;
      }
      if (resize_mask & ui::window::ResizeBottom) {
        int64_t new_bottom = static_cast<int64_t>(cursor.y());
        uint32_t bottom_limit = screen_h - taskbar_h;
        if (new_bottom > static_cast<int64_t>(bottom_limit))
          new_bottom = bottom_limit;
        nh = new_bottom - static_cast<int64_t>(w.rect.y);
        if (nh < min_h)
          nh = min_h;
      }

      if (nx != static_cast<int64_t>(old_x) ||
          ny != static_cast<int64_t>(old_y) ||
          nw != static_cast<int64_t>(old_w) ||
          nh != static_cast<int64_t>(old_h)) {
        w.rect.x = static_cast<uint32_t>(nx);
        w.rect.y = static_cast<uint32_t>(ny);
        w.rect.w = static_cast<uint32_t>(nw);
        w.rect.h = static_cast<uint32_t>(nh);
        // Dirty region is union of old and new window rects
        uint32_t rx0 = old_x < w.rect.x ? old_x : w.rect.x;
        uint32_t ry0 = old_y < w.rect.y ? old_y : w.rect.y;
        uint32_t rx1 = (old_x + old_w) > (w.rect.x + w.rect.w)
                           ? (old_x + old_w)
                           : (w.rect.x + w.rect.w);
        uint32_t ry1 = (old_y + old_h) > (w.rect.y + w.rect.h)
                           ? (old_y + old_h)
                           : (w.rect.y + w.rect.h);
        add_dirty(ui::Rect{rx0, ry0, rx1 - rx0, ry1 - ry0});
      }
    }

    // If UI changed, ensure cursor underlay is accurate before redraw
    if (ui_changed && !cursor_erased) {
      cursor.erase(graphics);
      cursor_erased = true;
    }

    // Redraw only the UI dirty region if needed
    if (ui_changed) {
      if ((dragging || resizing) && perf_border_only) {
        ui::draw_desktop_region_frames_only(graphics, windows, window_count,
                                            ui_dirty);
      } else {
        ui::draw_desktop_region(graphics, windows, window_count, ui_dirty);
      }
    }

    // Redraw cursor if it moved or UI changed underneath
    if (cursor_erased) {
      cursor.draw(graphics);
    }

    // Present only the union of dirty regions
    if (ui_changed || cursor_moved) {
      // Union ui_dirty and cursor_dirty
      ui::Rect present_rect = ui_dirty;
      if (cursor_moved) {
        if (present_rect.w == 0 || present_rect.h == 0) {
          present_rect = cursor_dirty;
        } else {
          uint32_t x0 =
              present_rect.x < cursor_dirty.x ? present_rect.x : cursor_dirty.x;
          uint32_t y0 =
              present_rect.y < cursor_dirty.y ? present_rect.y : cursor_dirty.y;
          uint32_t x1a = present_rect.x + present_rect.w;
          uint32_t y1a = present_rect.y + present_rect.h;
          uint32_t x1b = cursor_dirty.x + cursor_dirty.w;
          uint32_t y1b = cursor_dirty.y + cursor_dirty.h;
          uint32_t x1 = x1a > x1b ? x1a : x1b;
          uint32_t y1 = y1a > y1b ? y1a : y1b;
          present_rect.x = x0;
          present_rect.y = y0;
          present_rect.w = x1 - x0;
          present_rect.h = y1 - y0;
        }
      }
      if (present_rect.w != 0 && present_rect.h != 0) {
        graphics.present_rect(present_rect.x, present_rect.y, present_rect.w,
                              present_rect.h);
      }
    }

    prev_left = left;
  }
}
