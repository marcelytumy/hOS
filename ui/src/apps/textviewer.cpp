#include "apps/textviewer.hpp"
#include "font.hpp"
#include "graphics.hpp"
#include "window_manager.hpp"

namespace ui::apps::textviewer {

static constexpr uint32_t kTextColor = 0xFFFFFF;
static constexpr uint32_t kBackgroundColor = 0x1E1E1E;
static constexpr uint32_t kScrollbarColor = 0x4A4A4A;
static constexpr uint32_t kScrollbarThumbColor = 0x7A7A7A;
static constexpr uint32_t kLineHeight = 16;
static constexpr uint32_t kScrollbarWidth = 12;
static constexpr uint32_t kPadding = 8;

static void draw(Graphics &gfx, const ui::Rect &r, void *ud) {
  auto *st = static_cast<TextViewerState *>(ud);
  if (!st) {
    gfx.draw_string("Error: No state", r.x, r.y, 0xFF0000, default_font);
    return;
  }

  // Fill background
  gfx.fill_rect(r.x, r.y, r.w, r.h, kBackgroundColor);

  if (!st->content_loaded) {
    if (st->load_error) {
      gfx.draw_string(st->load_error, r.x + kPadding, r.y + kPadding, 0xFF0000,
                      default_font);
    } else {
      gfx.draw_string("Loading...", r.x + kPadding, r.y + kPadding, kTextColor,
                      default_font);
    }
    return;
  }

  if (st->content_size == 0) {
    gfx.draw_string("Empty file", r.x + kPadding, r.y + kPadding, kTextColor,
                    default_font);
    return;
  }

  // Calculate text area (excluding scrollbar)
  uint32_t text_area_h = r.h - kPadding * 2;
  uint32_t text_x = r.x + kPadding;
  uint32_t text_y = r.y + kPadding;

  // Count lines and calculate max scroll
  uint32_t line_count = 0;
  uint32_t visible_lines = text_area_h / kLineHeight;

  for (uint32_t i = 0; i < st->content_size; ++i) {
    if (st->content[i] == '\n') {
      line_count++;
    }
  }
  if (st->content_size > 0 && st->content[st->content_size - 1] != '\n') {
    line_count++; // Count last line if it doesn't end with newline
  }

  st->max_scroll_y =
      (line_count > visible_lines) ? (line_count - visible_lines) : 0;
  if (st->scroll_y > st->max_scroll_y) {
    st->scroll_y = st->max_scroll_y;
  }

  // Draw text content
  uint32_t current_line = 0;
  uint32_t line_start = 0;
  uint32_t y_offset = 0;

  for (uint32_t i = 0; i < st->content_size && y_offset < text_area_h; ++i) {
    if (st->content[i] == '\n' || i == st->content_size - 1) {
      // Draw this line if it's in the visible range
      if (current_line >= st->scroll_y &&
          current_line < st->scroll_y + visible_lines) {
        uint32_t line_len = i - line_start;
        if (i == st->content_size - 1 && st->content[i] != '\n') {
          line_len++; // Include the last character
        }

        // Truncate line if it's too long
        if (line_len > 0) {
          char line_buf[256];
          uint32_t copy_len = (line_len < sizeof(line_buf) - 1)
                                  ? line_len
                                  : sizeof(line_buf) - 1;
          for (uint32_t j = 0; j < copy_len; ++j) {
            line_buf[j] = st->content[line_start + j];
          }
          line_buf[copy_len] = '\0';

          gfx.draw_string(line_buf, text_x, text_y + y_offset, kTextColor,
                          default_font);
        }
        y_offset += kLineHeight;
      }

      line_start = i + 1;
      current_line++;
    }
  }

  // Draw scrollbar if needed
  if (st->max_scroll_y > 0) {
    uint32_t scrollbar_x = r.x + r.w - kScrollbarWidth - kPadding;
    uint32_t scrollbar_y = r.y + kPadding;
    uint32_t scrollbar_h = r.h - kPadding * 2;

    // Scrollbar background
    gfx.fill_rect(scrollbar_x, scrollbar_y, kScrollbarWidth, scrollbar_h,
                  kScrollbarColor);

    // Scrollbar thumb
    uint32_t thumb_h = (visible_lines * scrollbar_h) / line_count;
    if (thumb_h < 20)
      thumb_h = 20; // Minimum thumb size

    uint32_t thumb_y = scrollbar_y + (st->scroll_y * (scrollbar_h - thumb_h)) /
                                         st->max_scroll_y;
    gfx.fill_rect(scrollbar_x, thumb_y, kScrollbarWidth, thumb_h,
                  kScrollbarThumbColor);
  }
}

static void on_mouse(const ui::window::MouseEvent &ev, void *ud) {
  auto *st = static_cast<TextViewerState *>(ud);
  if (!st)
    return;

  // Handle scrollbar clicks
  if (ev.type == ui::window::MouseEvent::Type::Down && ev.left) {
    // Check if click is in scrollbar area
    if (ev.x >= 400 - kScrollbarWidth - kPadding) { // Approximate window width
      // Calculate scroll position based on click
      uint32_t scrollbar_h = 300 - kPadding * 2; // Approximate window height
      uint32_t click_y = ev.y - kPadding;

      if (st->max_scroll_y > 0) {
        uint32_t new_scroll = (click_y * st->max_scroll_y) / scrollbar_h;
        if (new_scroll > st->max_scroll_y) {
          new_scroll = st->max_scroll_y;
        }
        st->scroll_y = new_scroll;
      }
    }
  }
}

static bool load_file_content(TextViewerState *st) {
  if (!st || !st->fs || !st->file_path) {
    st->load_error = "Invalid state";
    return false;
  }

  // Store the file path for debugging
  static char debug_path[256];
  uint32_t i = 0;
  for (; st->file_path[i] && i < sizeof(debug_path) - 1; ++i) {
    debug_path[i] = st->file_path[i];
  }
  debug_path[i] = '\0';

  // Try to read the file
  uint64_t bytes_read = 0;

  if (st->fs->read_file_by_path(st->file_path, st->content,
                                sizeof(st->content) - 1, bytes_read)) {
    st->content_size = static_cast<uint32_t>(bytes_read);

    // Ensure null termination
    st->content[st->content_size] = '\0';

    st->content_loaded = true;
    st->scroll_y = 0;
    st->load_error = nullptr;
    return true;
  }

  // Show the path that failed
  st->load_error = debug_path;
  return false;
}

ui::window::Window create_window(uint32_t screen_w, uint32_t screen_h,
                                 fs::Ext4 &filesystem, const char *file_path) {
  // Use a static array to store multiple states (for multiple windows)
  static TextViewerState s_states[8];
  static uint32_t s_next_state = 0;

  TextViewerState &s_state = s_states[s_next_state];
  s_next_state = (s_next_state + 1) % 8; // Cycle through states

  s_state.fs = &filesystem;
  s_state.file_path = file_path;
  s_state.content_size = 0;
  s_state.scroll_y = 0;
  s_state.max_scroll_y = 0;
  s_state.content_loaded = false;
  s_state.load_error = nullptr;

  // Copy file path
  if (file_path) {
    uint32_t i = 0;
    for (; file_path[i] && i < sizeof(s_state.file_path_buf) - 1; ++i) {
      s_state.file_path_buf[i] = file_path[i];
    }
    s_state.file_path_buf[i] = '\0';
    s_state.file_path = s_state.file_path_buf;
  }

  // Try to load the file content
  load_file_content(&s_state);

  // Create window title from file path
  const char *title = "Text Viewer";
  if (file_path) {
    // Extract filename from path
    const char *filename = file_path;
    for (const char *p = file_path; *p; ++p) {
      if (*p == '/') {
        filename = p + 1;
      }
    }
    if (*filename) {
      title = filename;
    }
  }

  ui::window_manager::WindowOptions options;
  options.title = title;
  options.width = 600;
  options.height = 400;
  options.x = 100;
  options.y = 100;
  options.user_data = &s_state;
  options.draw_content = &draw;
  options.on_mouse = &on_mouse;

  return ui::window_manager::create_window(screen_w, screen_h, options);
}

} // namespace ui::apps::textviewer