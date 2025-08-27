#include "../include/startmenu.hpp"
#include "font.hpp"
#include "graphics.hpp"

namespace ui::startmenu {

static constexpr uint32_t kBg = 0x2A2A2A;
static constexpr uint32_t kBorder = 0x555555;
static constexpr uint32_t kItemBg = 0x333333;
static constexpr uint32_t kItemHover = 0x3F3F3F;
static constexpr uint32_t kText = 0xFFFFFF;

void init(State &st, uint32_t screen_w, uint32_t screen_h, const Item *items,
          uint32_t item_count) {
  st.open = false;
  const uint32_t menu_w = 220;
  const uint32_t menu_h = 200;
  const uint32_t tb_h =
      (screen_h / 18 < 32 ? 32 : (screen_h / 18 > 64 ? 64 : screen_h / 18));
  st.rect = ui::Rect{8u, screen_h - tb_h - menu_h - 8u, menu_w, menu_h};
  st.items = items;
  st.item_count = item_count;
  st.hover_index = -1;
}

void draw(Graphics &gfx, const State &st) {
  if (!st.open)
    return;
  const ui::Rect &r = st.rect;
  gfx.fill_rect(r.x, r.y, r.w, r.h, kBg);
  gfx.draw_rect(r.x, r.y, r.w, r.h, kBorder);

  const uint32_t row_h = default_font.char_height + 8u;
  uint32_t y = r.y + 8u;
  for (uint32_t i = 0; i < st.item_count; ++i) {
    if (y + row_h > r.y + r.h)
      break;
    uint32_t bg =
        (static_cast<int32_t>(i) == st.hover_index) ? kItemHover : kItemBg;
    gfx.fill_rect(r.x + 6, y, r.w - 12, row_h, bg);
    gfx.draw_string(st.items[i].label, r.x + 12, y + 4, kText, default_font);
    y += row_h + 6u;
  }
}

uint32_t hit_test_click(const State &st, uint32_t x, uint32_t y) {
  if (!st.open)
    return UINT32_MAX;
  const ui::Rect &r = st.rect;
  if (!(x >= r.x && x < r.x + r.w && y >= r.y && y < r.y + r.h)) {
    return 0xFFFFFFFEu; // outside: request close
  }
  const uint32_t row_h = default_font.char_height + 8u;
  uint32_t ycur = r.y + 8u;
  for (uint32_t i = 0; i < st.item_count; ++i) {
    if (ycur + row_h > r.y + r.h)
      break;
    ui::Rect ir{r.x + 6, ycur, r.w - 12, row_h};
    if (x >= ir.x && x < ir.x + ir.w && y >= ir.y && y < ir.y + ir.h) {
      return st.items[i].id;
    }
    ycur += row_h + 6u;
  }
  return UINT32_MAX;
}

void update_hover(State &st, uint32_t x, uint32_t y) {
  if (!st.open) {
    st.hover_index = -1;
    return;
  }
  const ui::Rect &r = st.rect;
  st.hover_index = -1;
  if (!(x >= r.x && x < r.x + r.w && y >= r.y && y < r.y + r.h))
    return;
  const uint32_t row_h = default_font.char_height + 8u;
  uint32_t ycur = r.y + 8u;
  for (uint32_t i = 0; i < st.item_count; ++i) {
    if (ycur + row_h > r.y + r.h)
      break;
    ui::Rect ir{r.x + 6, ycur, r.w - 12, row_h};
    if (x >= ir.x && x < ir.x + ir.w && y >= ir.y && y < ir.y + ir.h) {
      st.hover_index = static_cast<int32_t>(i);
      return;
    }
    ycur += row_h + 6u;
  }
}

} // namespace ui::startmenu
