#include "graphics.hpp"
#include "../include/cursor.hpp"

namespace ui {

Cursor::Cursor() : pos_x(20), pos_y(20), saved_pixel(0), has_saved(false) {}

void Cursor::set_position(uint32_t x, uint32_t y, uint32_t screen_w, uint32_t screen_h) {
	if (x >= screen_w) x = screen_w - 1;
	if (y >= screen_h) y = screen_h - 1;
	pos_x = x;
	pos_y = y;
}

void Cursor::move_by(int32_t dx, int32_t dy, uint32_t screen_w, uint32_t screen_h) {
	int64_t nx = static_cast<int64_t>(pos_x) + dx;
	int64_t ny = static_cast<int64_t>(pos_y) + dy;
	if (nx < 0) nx = 0; if (ny < 0) ny = 0;
	if (nx >= static_cast<int64_t>(screen_w)) nx = screen_w - 1;
	if (ny >= static_cast<int64_t>(screen_h)) ny = screen_h - 1;
	pos_x = static_cast<uint32_t>(nx);
	pos_y = static_cast<uint32_t>(ny);
}

void Cursor::draw(Graphics &gfx) {
	// Save underlay pixel and draw a simple white pixel cursor
	// With double-buffering and full-frame redraws, save/restore is optional,
	// but keep it to prevent artifacts if partial redraws occur.
	if (!has_saved) {
		saved_pixel = gfx.get_pixel(pos_x, pos_y);
		has_saved = true;
	}
	gfx.set_pixel(pos_x, pos_y, 0xFFFFFF);
}

void Cursor::erase(Graphics &gfx) {
	if (has_saved) {
		gfx.set_pixel(pos_x, pos_y, saved_pixel);
		has_saved = false;
	}
}

}


