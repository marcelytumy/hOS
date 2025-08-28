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
};

// Create a text viewer window for the specified file
ui::window::Window create_window(uint32_t screen_w, uint32_t screen_h,
                                 fs::Ext4 &filesystem, const char *file_path);

} // namespace ui::apps::textviewer
