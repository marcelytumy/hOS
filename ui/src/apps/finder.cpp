#include "apps/finder.hpp"
#include "font.hpp"
#include "graphics.hpp"

namespace ui::apps::finder {

static void draw(Graphics &gfx, const ui::Rect &r, void *ud) {
  FinderState *st = static_cast<FinderState *>(ud);
  if (!st || !st->fs)
    return;
  fs::Dirent ents[64];
  uint32_t cnt = 0;
  if (!st->fs->list_dir_by_path("/", ents, 64, cnt))
    return;
  const uint32_t row_h = 20;
  const uint32_t icon_w = 10;
  uint32_t y = r.y;
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
    y += row_h;
  }
}

bool create_window(uint32_t screen_w, uint32_t screen_h, fs::Ext4 &filesystem,
                   ui::window::Window &out_window) {
  static FinderState s_state{}; // simple static for now
  s_state.fs = &filesystem;
  s_state.cwd = "/";

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
  return true;
}

} // namespace ui::apps::finder
