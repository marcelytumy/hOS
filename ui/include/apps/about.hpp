#pragma once

#include "ui.hpp"
#include "window.hpp"
#include <cstdint>

namespace ui {
namespace apps {
namespace about {

bool create_window(uint32_t screen_w, uint32_t screen_h,
                   const ui::window::Window &anchor,
                   ui::window::Window &out_window);

} // namespace about
} // namespace apps
} // namespace ui
