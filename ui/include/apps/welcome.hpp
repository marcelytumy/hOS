#pragma once

#include "ui.hpp"
#include "window.hpp"
#include "window_manager.hpp"
#include <cstdint>

namespace ui {
namespace apps {
namespace welcome {

// Create the main Welcome window centered on screen.
ui::window::Window create_window(uint32_t screen_w, uint32_t screen_h);

} // namespace welcome
} // namespace apps
} // namespace ui
