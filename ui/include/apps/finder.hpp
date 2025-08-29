#pragma once

#include "fs/ext4.hpp"
#include "ui.hpp"
#include "window.hpp"
#include <cstdint>

namespace ui::apps::finder {

struct FinderState {
  fs::Ext4 *fs;
  const char *cwd; // points into cwd_buf
  char cwd_buf[256];
  char history[16][256];
  uint32_t history_len;
  int32_t selected_index;
  int32_t hover_index;
  bool dragging;
  int32_t drag_index;
  uint32_t last_mouse_x;
  uint32_t last_mouse_y;
  uint32_t press_x;
  uint32_t press_y;
  // File opening support
  char file_to_open[256];
  bool should_open_file;
  // Scrolling
  uint32_t scroll_offset;
  uint32_t last_view_rows;
};

// Populate a Finder window configured to list the root directory of the given
// fs. Returns the created window.
ui::window::Window create_window(uint32_t screen_w, uint32_t screen_h,
                                 fs::Ext4 &filesystem);

} // namespace ui::apps::finder
