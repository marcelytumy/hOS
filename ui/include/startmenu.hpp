#pragma once
#include "ui.hpp"
#include <cstdint>

class Graphics;

namespace ui {

namespace window {
struct Window;
}

namespace startmenu {

struct Item {
  const char *label;
  uint32_t id; // opaque id for kernel to map actions
};

struct State {
  bool open;
  ui::Rect rect; // menu panel rect
  // simple static list for now
  const Item *items;
  uint32_t item_count;
  int32_t hover_index;
};

// Initialize default start menu state (position near bottom-left)
void init(State &st, uint32_t screen_w, uint32_t screen_h, const Item *items,
          uint32_t item_count);

// Draw menu if open
void draw(Graphics &gfx, const State &st);

// Returns clicked item id if a menu item is clicked, UINT32_MAX otherwise.
// Returns 0xFFFFFFFE if click outside should close when open.
uint32_t hit_test_click(const State &st, uint32_t x, uint32_t y);

// Update hover index based on mouse move
void update_hover(State &st, uint32_t x, uint32_t y);

} // namespace startmenu
} // namespace ui
