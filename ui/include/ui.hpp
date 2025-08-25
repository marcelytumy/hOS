#pragma once
#include <cstdint>

class Graphics;

namespace ui {

struct Rect {
	uint32_t x;
	uint32_t y;
	uint32_t w;
	uint32_t h;
};

// Draw the static desktop UI (taskbar + sample window). Non-interactive.
void draw_desktop(Graphics &gfx);

}


