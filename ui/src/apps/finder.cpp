#include "apps/finder.hpp"
#include "../../include/window.hpp"
#include "font.hpp"
#include "graphics.hpp"
#include "window_manager.hpp"

namespace ui::apps::finder {

static inline bool is_dot_or_dotdot(const char *name) {
  if (!name)
    return false;
  // "."
  if (name[0] == '.' && name[1] == '\0')
    return true;
  // ".."
  if (name[0] == '.' && name[1] == '.' && name[2] == '\0')
    return true;
  return false;
}

static void draw(Graphics &gfx, const ui::Rect &r, void *ud) {
  FinderState *st = static_cast<FinderState *>(ud);
  if (!st || !st->fs)
    return;
  fs::Dirent ents[256];
  uint32_t cnt = 0;
  const char *path = st->cwd ? st->cwd : "/";
  if (!st->fs->list_dir_by_path(path, ents, 256, cnt))
    return;
  // Build filtered list excluding "." and ".."
  fs::Dirent vis[256];
  uint32_t vcnt = 0;
  for (uint32_t i = 0; i < cnt && vcnt < 256; ++i) {
    if (is_dot_or_dotdot(ents[i].name))
      continue;
    vis[vcnt++] = ents[i];
  }
  const uint32_t row_h = 20;
  const uint32_t icon_w = 10;
  uint32_t y = r.y;
  // Header with current path and a simple back button on the left
  gfx.fill_rect(r.x, y, 18, 18, 0x444444);
  gfx.draw_string("<", r.x + 4, y, 0xFFFFFF, default_font);
  gfx.draw_string(st->cwd ? st->cwd : "/", r.x + 24, y, 0xAAAAFF, default_font);
  y += 24;
  for (uint32_t i = 0; i < vcnt; ++i) {
    // apply scroll offset
    if (i < st->scroll_offset)
      continue;
    if (y + row_h > r.y + r.h)
      break;
    uint32_t name_col =
        (vis[i].type == fs::NodeType::Directory) ? 0x80FF80 : 0xFFFFFF;
    uint32_t icon_col =
        (vis[i].type == fs::NodeType::Directory) ? 0x2E8B57 : 0x4682B4;
    gfx.fill_rect(r.x, y + 4, icon_w, icon_w, icon_col);
    gfx.draw_string(vis[i].name, r.x + icon_w + 8, y + 2, name_col,
                    default_font);
    // Highlight selected
    if (static_cast<int32_t>(i) == st->selected_index) {
      gfx.draw_rect(r.x, y, r.w, row_h, 0x888888);
    }
    // Hover highlight
    if (static_cast<int32_t>(i) == st->hover_index) {
      gfx.draw_rect(r.x + 1, y + 1, r.w - 2, row_h - 2, 0x555555);
    }
    y += row_h;
  }

  // cache how many rows fit for scroll handling
  st->last_view_rows = (r.h - 24) / row_h;

  // Drag ghost
  if (st->dragging && st->drag_index >= 0) {
    uint32_t gx = r.x + st->last_mouse_x + 6;
    uint32_t gy = r.y + st->last_mouse_y + 6;
    gfx.draw_rect(gx, gy, 80, 16, 0xAAAAAA);
  }
}

static void open_selected(FinderState *st) {
  if (!st || !st->fs)
    return;
  fs::Dirent ents[256];
  uint32_t cnt = 0;
  if (!st->fs->list_dir_by_path(st->cwd ? st->cwd : "/", ents, 256, cnt))
    return;
  // Build filtered list excluding "." and ".."
  fs::Dirent vis[256];
  uint32_t vcnt = 0;
  for (uint32_t i = 0; i < cnt && vcnt < 256; ++i) {
    if (is_dot_or_dotdot(ents[i].name))
      continue;
    vis[vcnt++] = ents[i];
  }
  if (st->selected_index < 0 ||
      static_cast<uint32_t>(st->selected_index) >= vcnt)
    return;
  const fs::Dirent &e = vis[st->selected_index];
  if (e.type == fs::NodeType::Directory) {
    // Append name to cwd
    char new_path[256];
    uint32_t n = 0;
    const char *cwd = st->cwd ? st->cwd : "/";
    if (cwd[0] != '/' || cwd[1] != '\0') {
      // non-root
      for (; cwd[n] && n < sizeof(new_path) - 1; ++n)
        new_path[n] = cwd[n];
      if (n < sizeof(new_path) - 1 && new_path[n - 1] != '/')
        new_path[n++] = '/';
    } else {
      new_path[n++] = '/';
    }
    // copy entry name
    for (uint32_t i = 0; e.name[i] && n < sizeof(new_path) - 1; ++i)
      new_path[n++] = e.name[i];
    new_path[n] = '\0';
    // save history
    if (st->history_len < 16) {
      uint32_t idx = st->history_len++;
      // copy old cwd
      uint32_t j = 0;
      const char *oc = st->cwd ? st->cwd : "/";
      for (; oc[j] && j < sizeof(st->history[0]) - 1; ++j)
        st->history[idx][j] = oc[j];
      st->history[idx][j] = '\0';
    }
    // update cwd
    uint32_t m = 0;
    for (; new_path[m] && m < sizeof(st->cwd_buf) - 1; ++m)
      st->cwd_buf[m] = new_path[m];
    st->cwd_buf[m] = '\0';
    st->cwd = st->cwd_buf;
    st->selected_index = -1;
  } else if (e.type == fs::NodeType::File) {
    // Handle file opening - check file extension
    bool is_text_file = false;
    const char *ext = nullptr;

    // Find file extension
    for (uint32_t i = 0; e.name[i]; ++i) {
      if (e.name[i] == '.') {
        ext = &e.name[i + 1];
      }
    }

    if (ext) {
      // Check for text file extensions
      if ((ext[0] == 't' && ext[1] == 'x' && ext[2] == 't' && ext[3] == '\0') ||
          (ext[0] == 'h' && ext[1] == 'o' && ext[2] == 's' && ext[3] == '-' &&
           ext[4] == 'r' && ext[5] == 'e' && ext[6] == 'g' && ext[7] == 'i' &&
           ext[8] == 's' && ext[9] == 't' && ext[10] == 'r' && ext[11] == 'y' &&
           ext[12] == '\0')) {
        is_text_file = true;
      }
    } else {
      // If no extension, treat as text file (for files created with nano)
      is_text_file = true;
    }

    if (is_text_file) {
      // Build full file path
      char file_path[256];
      uint32_t n = 0;
      const char *cwd = st->cwd ? st->cwd : "/";
      if (cwd[0] != '/' || cwd[1] != '\0') {
        // non-root
        for (; cwd[n] && n < sizeof(file_path) - 1; ++n)
          file_path[n] = cwd[n];
        if (n < sizeof(file_path) - 1 && file_path[n - 1] != '/')
          file_path[n++] = '/';
      } else {
        file_path[n++] = '/';
      }
      // copy entry name
      for (uint32_t i = 0; e.name[i] && n < sizeof(file_path) - 1; ++i)
        file_path[n++] = e.name[i];
      file_path[n] = '\0';

      // Store the file path for the text viewer to use
      // This will be handled by the main loop when it detects a text file open
      // request
      uint32_t path_len = 0;
      for (; file_path[path_len] && path_len < sizeof(st->file_to_open) - 1;
           ++path_len) {
        st->file_to_open[path_len] = file_path[path_len];
      }
      st->file_to_open[path_len] = '\0';
      st->should_open_file = true;
    }
  }
}

static void go_back(FinderState *st) {
  if (!st)
    return;
  if (st->history_len == 0)
    return;
  // pop
  uint32_t idx = st->history_len - 1;
  // copy back into cwd_buf
  uint32_t i = 0;
  for (; st->history[idx][i] && i < sizeof(st->cwd_buf) - 1; ++i)
    st->cwd_buf[i] = st->history[idx][i];
  st->cwd_buf[i] = '\0';
  st->cwd = st->cwd_buf;
  st->history_len--;
  st->selected_index = -1;
}

static void on_mouse(const ui::window::MouseEvent &ev, void *ud) {
  FinderState *st = static_cast<FinderState *>(ud);
  if (!st || !st->fs)
    return;
  st->last_mouse_x = ev.x;
  st->last_mouse_y = ev.y;
  // Layout must match draw(): header 24px, then rows of 20px
  if (ev.type == ui::window::MouseEvent::Type::Down && ev.left) {
    // Back button hit test: right-top corner 18x18 square
    // Assume content rect width is unknown here; approximate by x > width-22 is
    // not available. Instead, use fixed area near left for simplicity: '<'
    // button at x in [0,18], y in [0,18]
    if (ev.x <= 18 && ev.y <= 18) {
      go_back(st);
      return;
    }
    // Select row
    if (ev.y >= 24) {
      uint32_t row = (ev.y - 24) / 20;
      st->selected_index = static_cast<int32_t>(row + st->scroll_offset);
      st->drag_index = st->selected_index;
      st->dragging = false;
      st->press_x = ev.x;
      st->press_y = ev.y;
    }
  } else if (ev.type == ui::window::MouseEvent::Type::Up) {
    // If it was a simple click (no movement), open
    uint32_t dx =
        (ev.x > st->press_x) ? (ev.x - st->press_x) : (st->press_x - ev.x);
    uint32_t dy =
        (ev.y > st->press_y) ? (ev.y - st->press_y) : (st->press_y - ev.y);
    bool was_dragging = st->dragging;
    st->dragging = false;
    if (!was_dragging && dx < 3 && dy < 3) {
      open_selected(st);
    }
  } else if (ev.type == ui::window::MouseEvent::Type::Move) {
    // Could draw a drag ghost in draw() based on st->dragging and last mouse
    if (ev.y >= 24) {
      uint32_t row = (ev.y - 24) / 20;
      st->hover_index = static_cast<int32_t>(row + st->scroll_offset);
    } else {
      st->hover_index = -1;
    }
    if (st->selected_index >= 0 && ev.left && !st->dragging) {
      uint32_t dx =
          (ev.x > st->press_x) ? (ev.x - st->press_x) : (st->press_x - ev.x);
      uint32_t dy =
          (ev.y > st->press_y) ? (ev.y - st->press_y) : (st->press_y - ev.y);
      if (dx >= 3 || dy >= 3)
        st->dragging = true;
    }
  } else if (ev.type == ui::window::MouseEvent::Type::Wheel) {
    // Positive wheel_y scrolls up
    int32_t so = static_cast<int32_t>(st->scroll_offset);
    so -= (ev.wheel_y > 0 ? 1 : -1);
    if (so < 0)
      so = 0;
    // compute max
    fs::Dirent ents[256];
    uint32_t cnt = 0;
    const char *path = st->cwd ? st->cwd : "/";
    if (st->fs->list_dir_by_path(path, ents, 256, cnt)) {
      uint32_t vcnt = 0;
      for (uint32_t i = 0; i < cnt && vcnt < 256; ++i)
        if (!is_dot_or_dotdot(ents[i].name))
          vcnt++;
      uint32_t rows = st->last_view_rows ? st->last_view_rows : 10u;
      uint32_t max_off = (vcnt > rows) ? (vcnt - rows) : 0;
      if (static_cast<uint32_t>(so) > max_off)
        so = static_cast<int32_t>(max_off);
    }
    st->scroll_offset = static_cast<uint32_t>(so);
  }
}

ui::window::Window create_window(uint32_t screen_w, uint32_t screen_h,
                                 fs::Ext4 &filesystem) {
  static FinderState s_state{}; // simple static for now
  s_state.fs = &filesystem;
  s_state.cwd_buf[0] = '/';
  s_state.cwd_buf[1] = '\0';
  s_state.cwd = s_state.cwd_buf;
  s_state.history_len = 0;
  s_state.selected_index = -1;
  s_state.hover_index = -1;
  s_state.dragging = false;
  s_state.drag_index = -1;
  s_state.last_mouse_x = 0;
  s_state.last_mouse_y = 0;
  s_state.press_x = 0;
  s_state.press_y = 0;
  s_state.file_to_open[0] = '\0';
  s_state.should_open_file = false;
  s_state.scroll_offset = 0;
  s_state.last_view_rows = 0;

  ui::window_manager::WindowOptions options;
  options.title = "Finder";
  options.width = screen_w / 2;
  options.height = screen_h / 2;
  options.x = 40; // Fixed position
  options.y = 40;
  options.user_data = &s_state;
  options.draw_content = &draw;
  options.on_mouse = &on_mouse;

  return ui::window_manager::create_window(screen_w, screen_h, options);
}

} // namespace ui::apps::finder
