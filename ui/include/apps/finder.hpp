#pragma once

#include "fs/ext4.hpp"
#include "ui.hpp"
#include "window.hpp"
#include <cstdint>

namespace ui {
namespace apps {
namespace finder {

struct FinderState {
  fs::Ext4 *fs;
  const char *cwd; // currently only "/"
};

// Populate a Finder window configured to list the root directory of the given
// fs. Returns true on success.
bool create_window(uint32_t screen_w, uint32_t screen_h, fs::Ext4 &filesystem,
                   ui::window::Window &out_window);

} // namespace finder
} // namespace apps
} // namespace ui
