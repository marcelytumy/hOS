#pragma once

#include "fs/ext4.hpp"
#include "ui.hpp"
#include "window.hpp"
#include <cstdint>

namespace ui::apps::textviewer {

struct TextViewerState {
  fs::Ext4 *fs;
  const char *file_path;
  char file_path_buf[256];
  char content[8192]; // Static buffer for file content
  uint32_t content_size;
  uint32_t scroll_y;
  uint32_t max_scroll_y;
  bool content_loaded;
  const char *load_error; // Error message if loading failed
  bool dragging_thumb;
  int32_t drag_offset_y; // distance from thumb top where drag started
  bool scrollbar_visible;
  uint32_t scrollbar_x;
  uint32_t scrollbar_y;
  uint32_t scrollbar_h;
  uint32_t thumb_y;
  uint32_t thumb_h;
  uint32_t content_w;
  uint32_t content_h;
  uint32_t visible_lines_cache;
  uint32_t line_count_cache;
};

// Create a text viewer window for the specified file
ui::window::Window create_window(uint32_t screen_w, uint32_t screen_h,
                                 fs::Ext4 &filesystem, const char *file_path);

} // namespace ui::apps::textviewer
