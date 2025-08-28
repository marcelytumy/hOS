#pragma once

#include "ui.hpp"
#include "window.hpp"
#include "window_manager.hpp"
#include <cstdint>

namespace ui {
namespace apps {
namespace about {

ui::window::Window create_window(uint32_t screen_w, uint32_t screen_h,
                                 const ui::window::Window &anchor = {});

} // namespace about
} // namespace apps
} // namespace ui
