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

// Draw the desktop UI (taskbar + window) with default centered window.
void draw_desktop(Graphics &gfx);

// Draw the desktop UI (taskbar + window) at provided window rect.
void draw_desktop(Graphics &gfx, const Rect &window_rect);

// Query UI metrics and helpers
uint32_t get_taskbar_height(uint32_t screen_h);
uint32_t get_titlebar_height();
bool point_in_titlebar(const Rect &window_rect, uint32_t x, uint32_t y);

}


