#include "apps/finder.hpp"
#include "../../include/window.hpp"
#include "font.hpp"
#include "graphics.hpp"

namespace ui::apps::finder {

static void draw(Graphics &gfx, const ui::Rect &r, void *ud) {
  FinderState *st = static_cast<FinderState *>(ud);
  if (!st || !st->fs)
    return;
  fs::Dirent ents[64];
  uint32_t cnt = 0;
  const char *path = st->cwd ? st->cwd : "/";
  if (!st->fs->list_dir_by_path(path, ents, 64, cnt))
    return;
  const uint32_t row_h = 20;
  const uint32_t icon_w = 10;
  uint32_t y = r.y;
  // Header with current path and a simple back button on the left
  gfx.fill_rect(r.x, y, 18, 18, 0x444444);
  gfx.draw_string("<", r.x + 4, y, 0xFFFFFF, default_font);
  gfx.draw_string(st->cwd ? st->cwd : "/", r.x + 24, y, 0xAAAAFF, default_font);
  y += 24;
  for (uint32_t i = 0; i < cnt; ++i) {
    if (y + row_h > r.y + r.h)
      break;
    uint32_t name_col =
        (ents[i].type == fs::NodeType::Directory) ? 0x80FF80 : 0xFFFFFF;
    uint32_t icon_col =
        (ents[i].type == fs::NodeType::Directory) ? 0x2E8B57 : 0x4682B4;
    gfx.fill_rect(r.x, y + 4, icon_w, icon_w, icon_col);
    gfx.draw_string(ents[i].name, r.x + icon_w + 8, y + 2, name_col,
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
  fs::Dirent ents[64];
  uint32_t cnt = 0;
  if (!st->fs->list_dir_by_path(st->cwd ? st->cwd : "/", ents, 64, cnt))
    return;
  if (st->selected_index < 0 ||
      static_cast<uint32_t>(st->selected_index) >= cnt)
    return;
  const fs::Dirent &e = ents[st->selected_index];
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
      st->selected_index = static_cast<int32_t>(row);
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
      st->hover_index = static_cast<int32_t>(row);
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
  }
}

bool create_window(uint32_t screen_w, uint32_t screen_h, fs::Ext4 &filesystem,
                   ui::window::Window &out_window) {
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

  out_window = {};
  out_window.rect = ui::Rect{40, 40, screen_w / 2, screen_h / 2};
  out_window.title = "Finder";
  out_window.minimized = false;
  out_window.maximized = false;
  out_window.fullscreen = false;
  out_window.resizable = true;
  out_window.movable = true;
  out_window.draggable = true;
  out_window.closeable = true;
  out_window.focused = true;
  out_window.always_on_top = false;
  out_window.user_data = &s_state;
  out_window.draw_content = &draw;
  out_window.on_mouse = &on_mouse;
  return true;
}

} // namespace ui::apps::finder
