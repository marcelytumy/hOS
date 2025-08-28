#pragma once

#include "ui.hpp"
#include "window.hpp"
#include <cstdint>

namespace ui::apps::about {

ui::window::Window create_window(uint32_t screen_w, uint32_t screen_h,
                                 const ui::window::Window &anchor = {});

} // namespace ui::apps::about
