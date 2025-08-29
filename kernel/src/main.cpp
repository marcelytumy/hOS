#include "../../input/include/mouse.hpp"
#include "../../ui/include/cursor.hpp"
#include "../../ui/include/startmenu.hpp"
#include "../../ui/include/taskbar.hpp"
#include "../../ui/include/ui.hpp"
#include "../../ui/include/window.hpp"
#include "../../ui/include/window_manager.hpp"
#include "apps/about.hpp"
#include "apps/finder.hpp"
#include "apps/start_ids.hpp"
#include "apps/textviewer.hpp"
#include "apps/welcome.hpp"
#include "font.hpp"
#include "fs/blockdev.hpp"
#include "fs/ext4.hpp"
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

__attribute__((used,
               section(".limine_requests"))) volatile limine_module_request
    module_request = {.id = LIMINE_MODULE_REQUEST,
                      .revision = 0,
                      .response = nullptr,
                      .internal_module_count = 0,
                      .internal_modules = nullptr};

} // namespace

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

void *memcpy(void *dest, const void *src, std::size_t n) {
  auto *d = static_cast<uint8_t *>(dest);
  const auto *s = static_cast<const uint8_t *>(src);

  for (std::size_t i = 0; i < n; ++i) {
    d[i] = s[i];
  }

  return dest;
}

void *memset(void *buffer, int value, std::size_t count) {
  auto *p = static_cast<std::uint8_t *>(buffer);
  auto byte_value = static_cast<std::uint8_t>(value);

  for (std::size_t i = 0; i < count; ++i) {
    p[i] = byte_value;
  }

  return buffer;
}

void *memmove(void *dest, const void *src, std::size_t n) {
  auto *pdest = static_cast<std::uint8_t *>(dest);
  const auto *psrc = static_cast<const std::uint8_t *>(src);

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
  const auto *p1 = static_cast<const std::uint8_t *>(s1);
  const auto *p2 = static_cast<const std::uint8_t *>(s2);

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
  int bar_x = static_cast<int>(framebuffer->width) / 2 - 100;
  int bar_y = static_cast<int>(framebuffer->height) / 2 + 50;
  int bar_width = 200;
  int bar_height = 20;

  graphics.draw_rect(bar_x, bar_y, bar_width, bar_height, 0xFFFFFF);
  // Progress drawer: fills inner area based on percent [0..100]
  auto set_progress = [&](int percent) {
    if (percent < 0)
      percent = 0;
    if (percent > 100)
      percent = 100;
    int inner_w = bar_width - 2;
    int inner_h = bar_height - 2;
    int fill_w = (inner_w * percent) / 100;
    // Draw border (kept), then fill inside from left
    graphics.draw_rect(bar_x, bar_y, bar_width, bar_height, 0xFFFFFF);
    if (fill_w > 0) {
      graphics.fill_rect(bar_x + 1, bar_y + 1, fill_w, inner_h, 0xFFFFFF);
    }
  };
  // Begin with 5%
  set_progress(5);

  // Try to locate a rootfs module from Limine modules (real milestone)
  const limine_module_response *mods = module_request.response;
  const limine_file *rootfs = nullptr;
  if (mods && mods->module_count > 0) {
    for (uint64_t i = 0; i < mods->module_count; ++i) {
      const limine_file *f = mods->modules[i];
      if (!f || !f->path)
        continue;
#if LIMINE_API_REVISION >= 3
      if (f->string) {
        const char *s = f->string;
        bool match =
            (s[0] == 'r' && s[1] == 'o' && s[2] == 'o' && s[3] == 't' &&
             s[4] == 'f' && s[5] == 's' && s[6] == '\0');
        if (match) {
          rootfs = f;
          break;
        }
      }
#endif
      if (rootfs == nullptr)
        rootfs = f;
    }
  }
  // Found modules/rootfs info ~40%
  set_progress(rootfs ? 40 : 20);

  // Enable double-buffering backbuffer
  static uint32_t backbuffer_storage[1920 * 1080];
  graphics.enable_backbuffer(backbuffer_storage, 1920u * 1080u);
  // Backbuffer enabled ~50%
  set_progress(50);
  graphics.present();

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
  ui::window::Window windows[16];
  windows[0] = ui::apps::welcome::create_window(screen_w, screen_h);
  windows[1] = ui::apps::about::create_window(screen_w, screen_h, windows[0]);
  uint32_t window_count = 2;

  // Saved rects for restore after maximize
  ui::Rect saved_rects[16];
  bool saved_rect_valid[16];
  for (uint32_t i = 0; i < 16; ++i)
    saved_rect_valid[i] = false;

  // Draw desktop with windows (to backbuffer), then present
  ui::draw_desktop(graphics, windows, window_count);
  // Draw overlay if any (none at boot)
  graphics.present();
  // Initial desktop drawn ~60%
  set_progress(60);
  graphics.present();

  if (rootfs && rootfs->address && rootfs->size > 4096) {
    // Static lifetime so callbacks can read later
    static fs::MemoryBlockDevice s_memdev(nullptr, 0);
    static bool memdev_init = false;
    if (!memdev_init) {
      // Reconstruct via simple assignment
      s_memdev = fs::MemoryBlockDevice(rootfs->address, rootfs->size);
      memdev_init = true;
    }
    static fs::Ext4 s_ext4(s_memdev);
    if (s_ext4.mount()) {
      // Filesystem mounted ~80%
      set_progress(80);
      graphics.present();
      if (window_count < 16) {
        windows[window_count++] =
            ui::apps::finder::create_window(screen_w, screen_h, s_ext4);
        for (uint32_t j = 0; j < window_count; ++j)
          windows[j].focused = (j == window_count - 1);
        ui::draw_desktop(graphics, windows, window_count);
        graphics.present();
      }
    }
  }

  // Initialize mouse and cursor
  input::Ps2Mouse mouse;
  mouse.initialize();
  ui::Cursor cursor;
  cursor.set_position(screen_w / 2, screen_h / 2, screen_w, screen_h);
  cursor.draw(graphics);
  graphics.present();
  // Input ready, UI responsive ~100%
  set_progress(100);

  // Start menu state and items
  static const ui::startmenu::Item kStartItems[] = {
      {"Welcome", apps::Start_Welcome},
      {"About", apps::Start_About},
      {"Finder", apps::Start_Finder},
      {"Text Viewer", apps::Start_TextViewer},
  };
  ui::startmenu::State start_state{};
  ui::startmenu::init(start_state, screen_w, screen_h, kStartItems,
                      sizeof(kStartItems) / sizeof(kStartItems[0]));

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
    int8_t dx = 0, dy = 0, dz = 0;
    bool left = false, right = false, middle = false;
    if (!mouse.poll_packet(dx, dy, dz, left, right, middle)) {
      continue;
    }

    // Track if UI changed this frame; drop region rendering entirely
    bool ui_changed = false;
    auto add_dirty = [&](const ui::Rect &) { ui_changed = true; };

    const bool cursor_moved = (dx != 0) || (dy != 0);
    if (cursor_moved) {
      // Restore background under old cursor before any updates
      cursor.erase(graphics);
      cursor.move_by(dx, dy, screen_w, screen_h);
    }

    // Handle drag begin/end, start menu, and taskbar clicks
    if (left && !prev_left) {
      // Check taskbar click first (includes Start)
      uint32_t tb_hit = ui::taskbar::hit_test(cursor.x(), cursor.y(), screen_w,
                                              screen_h, windows, window_count);
      if (tb_hit == ui::taskbar::kHitStart) {
        // Toggle Start menu
        start_state.open = !start_state.open;
        ui_changed = true;
      } else if (tb_hit != UINT32_MAX && tb_hit < window_count) {
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
        // If Start menu is open, check for menu item clicks or outside close
        if (start_state.open) {
          uint32_t sm = ui::startmenu::hit_test_click(start_state, cursor.x(),
                                                      cursor.y());
          if (sm == 0xFFFFFFFEu) {
            start_state.open = false;
            ui_changed = true;
          } else if (sm != UINT32_MAX) {
            // Launch app by id
            if (sm == apps::Start_Welcome && window_count < 16) {
              windows[window_count++] =
                  ui::apps::welcome::create_window(screen_w, screen_h);
            } else if (sm == apps::Start_About && window_count < 16) {
              uint32_t current_count = window_count;
              windows[window_count++] = ui::apps::about::create_window(
                  screen_w, screen_h, windows[current_count - 1]);
            } else if (sm == apps::Start_Finder && window_count < 16 &&
                       rootfs && rootfs->address && rootfs->size > 4096) {
              // Reuse mounted fs if available
              static fs::MemoryBlockDevice s_memdev2(nullptr, 0);
              static fs::Ext4 s_ext4_2(s_memdev2);
              static bool init2 = false;
              if (!init2) {
                s_memdev2 =
                    fs::MemoryBlockDevice(rootfs->address, rootfs->size);
                init2 = s_ext4_2.mount();
              }
              if (init2) {
                windows[window_count++] = ui::apps::finder::create_window(
                    screen_w, screen_h, s_ext4_2);
              }
            } else if (sm == apps::Start_TextViewer && window_count < 16 &&
                       rootfs && rootfs->address && rootfs->size > 4096) {
              // Reuse mounted fs if available
              static fs::MemoryBlockDevice s_memdev3(nullptr, 0);
              static fs::Ext4 s_ext4_3(s_memdev3);
              static bool init3 = false;
              if (!init3) {
                s_memdev3 =
                    fs::MemoryBlockDevice(rootfs->address, rootfs->size);
                init3 = s_ext4_3.mount();
              }
              if (init3) {
                windows[window_count++] = ui::apps::textviewer::create_window(
                    screen_w, screen_h, s_ext4_3, nullptr);
              }
            }
            // Focus the newly created window
            for (uint32_t j = 0; j < window_count; ++j)
              windows[j].focused = (j == window_count - 1);
            start_state.open = false;
            ui_changed = true;
          }
        }
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
            if (static_cast<unsigned int>(i) != window_count - 1) {
              ui::window::Window tmp = windows[i];
              for (auto k = static_cast<unsigned int>(i); k + 1 < window_count;
                   ++k)
                windows[k] = windows[k + 1];
              windows[window_count - 1] = tmp;
              dragging_index = static_cast<int>(window_count - 1);
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
                dragging_index = static_cast<int>(window_count - 1);
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

    // Check for file opening requests from finder windows
    for (uint32_t i = 0; i < window_count; ++i) {
      ui::window::Window &w = windows[i];
      if (w.user_data) {
        // Check if this is a finder window with a file to open
        // We need to cast and check the state
        auto *finder_state =
            static_cast<ui::apps::finder::FinderState *>(w.user_data);
        if (finder_state && finder_state->should_open_file &&
            finder_state->file_to_open[0] != '\0' && window_count < 16) {

          // Clear the file opening request immediately to prevent multiple
          // windows
          char file_path_to_open[256];
          uint32_t path_len = 0;
          for (; finder_state->file_to_open[path_len] &&
                 path_len < sizeof(file_path_to_open) - 1;
               ++path_len) {
            file_path_to_open[path_len] = finder_state->file_to_open[path_len];
          }
          file_path_to_open[path_len] = '\0';

          finder_state->file_to_open[0] = '\0';
          finder_state->should_open_file = false;

          // Create text viewer for the file
          if (rootfs && rootfs->address && rootfs->size > 4096) {
            static fs::MemoryBlockDevice s_memdev4(nullptr, 0);
            static fs::Ext4 s_ext4_4(s_memdev4);
            static bool init4 = false;
            if (!init4) {
              s_memdev4 = fs::MemoryBlockDevice(rootfs->address, rootfs->size);
              init4 = s_ext4_4.mount();
            }
            if (init4) {
              windows[window_count++] = ui::apps::textviewer::create_window(
                  screen_w, screen_h, s_ext4_4, file_path_to_open);

              // Focus the new text viewer
              for (uint32_t j = 0; j < window_count; ++j) {
                windows[j].focused = (j == window_count - 1);
              }

              ui_changed = true;
            }
          }
        }
      }
    }

    // Dispatch mouse events to window content (topmost first)
    auto dispatch_to_content = [&](ui::window::MouseEvent::Type etype) {
      // Find topmost window whose content rect contains the cursor
      for (int i = static_cast<int>(window_count) - 1; i >= 0; --i) {
        const ui::window::Window &w = windows[i];
        if (w.minimized || w.on_mouse == nullptr)
          continue;
        ui::Rect content = ui::window::get_content_rect(w, screen_w, screen_h);
        uint32_t cx = cursor.x();
        uint32_t cy = cursor.y();
        if (cx >= content.x && cx < content.x + content.w && cy >= content.y &&
            cy < content.y + content.h) {
          ui::window::MouseEvent ev{};
          ev.type = etype;
          ev.x = cx - content.x;
          ev.y = cy - content.y;
          ev.left = left;
          ev.right = right;
          ev.middle = middle;
          ev.wheel_y = 0;
          w.on_mouse(ev, w.user_data);
          // Assume content may have changed (e.g., hover highlight)
          ui_changed = true;
          break;
        }
      }
    };

    // Generate events based on current state
    if (cursor_moved) {
      dispatch_to_content(ui::window::MouseEvent::Type::Move);
    }
    if (left && !prev_left) {
      dispatch_to_content(ui::window::MouseEvent::Type::Down);
    }
    if (!left && prev_left) {
      dispatch_to_content(ui::window::MouseEvent::Type::Up);
    }
    if (dz != 0) {
      // Deliver a wheel event to the window under cursor
      for (int i = static_cast<int>(window_count) - 1; i >= 0; --i) {
        const ui::window::Window &w = windows[i];
        if (w.minimized || w.on_mouse == nullptr)
          continue;
        ui::Rect content = ui::window::get_content_rect(w, screen_w, screen_h);
        uint32_t cx = cursor.x();
        uint32_t cy = cursor.y();
        if (cx >= content.x && cx < content.x + content.w && cy >= content.y &&
            cy < content.y + content.h) {
          ui::window::MouseEvent ev{};
          ev.type = ui::window::MouseEvent::Type::Wheel;
          ev.x = cx - content.x;
          ev.y = cy - content.y;
          ev.left = left;
          ev.right = right;
          ev.middle = middle;
          // Treat positive dz as wheel up (scroll up)
          ev.wheel_y = dz;
          w.on_mouse(ev, w.user_data);
          ui_changed = true;
          break;
        }
      }
    }

    // Present: prefer minimal redraw on cursor-only movement
    if (ui_changed) {
      // Full scene redraw invalidates cursor underlay cache
      ui::draw_desktop(graphics, windows, window_count);
      if (start_state.open) {
        ui::startmenu::draw(graphics, start_state);
      }
      cursor.invalidate();
      cursor.draw(graphics);
      graphics.present();
    } else if (cursor_moved) {
      if (start_state.open) {
        ui::startmenu::update_hover(start_state, cursor.x(), cursor.y());
        ui::startmenu::draw(graphics, start_state);
      }
      cursor.draw(graphics);
      graphics.present();
    }

    prev_left = left;
  }
}
