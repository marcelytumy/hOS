#pragma once

#include "ui.hpp"
#include "window.hpp"
#include <cstdint>

namespace ui {
namespace apps {
namespace welcome {

// Create the main Welcome window centered on screen.
bool create_window(uint32_t screen_w, uint32_t screen_h,
                   ui::window::Window &out_window);

} // namespace welcome
} // namespace apps
} // namespace ui
