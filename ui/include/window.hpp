#pragma once
#include <cstdint>
#include "ui.hpp"

class Graphics;

namespace ui {
namespace window {

struct Window {
	Rect rect;
	const char* title;
	bool minimized;
};

uint32_t get_titlebar_height();
bool point_in_titlebar(const Window &w, uint32_t x, uint32_t y);
void draw(Graphics &gfx, const Window &w);

}
}


