#include "graphics.hpp"
#include "font.hpp"
#include "../include/ui.hpp"
#include "../include/window.hpp"
#include "../include/taskbar.hpp"

namespace ui { namespace taskbar {

static constexpr uint32_t kTaskbarBg = 0x2B2B2B;
static constexpr uint32_t kTaskbarBorder = 0x3A3A3A;
static constexpr uint32_t kButtonBg = 0x3B3B3B;
static constexpr uint32_t kButtonBorder = 0x5A5A5A;
static constexpr uint32_t kButtonText = 0xFFFFFF;

uint32_t height(uint32_t screen_h) {
	return (screen_h / 18 < 32 ? 32 : (screen_h / 18 > 64 ? 64 : screen_h / 18));
}

void draw(Graphics &gfx, uint32_t screen_w, uint32_t screen_h, const window::Window* windows, uint32_t count) {
	const uint32_t h = height(screen_h);
	const uint32_t y = screen_h - h;
	gfx.fill_rect(0, y, screen_w, h, kTaskbarBg);
	gfx.draw_rect(0, y, screen_w, h, kTaskbarBorder);

	// Start area
	uint32_t x = 8;
	const uint32_t start_w = 80;
	gfx.fill_rect(x, y + 6, start_w, h - 12, kButtonBg);
	gfx.draw_rect(x, y + 6, start_w, h - 12, kButtonBorder);
	gfx.draw_string("Start", x + 10, y + (h / 2) - (default_font.char_height / 2), kButtonText, default_font);
	x += start_w + 8;

	// Window buttons
	for (uint32_t i = 0; i < count; ++i) {
		const char* title = windows[i].title ? windows[i].title : "Window";
		uint32_t btn_w = 140;
		if (x + btn_w + 8 > screen_w) break;
		// Dim if minimized
		uint32_t bg = windows[i].minimized ? 0x2E2E2E : kButtonBg;
		uint32_t border = windows[i].minimized ? 0x4A4A4A : kButtonBorder;
		uint32_t text = windows[i].minimized ? 0xAAAAAA : kButtonText;
		gfx.fill_rect(x, y + 6, btn_w, h - 12, bg);
		gfx.draw_rect(x, y + 6, btn_w, h - 12, border);
		gfx.draw_string(title, x + 10, y + (h / 2) - (default_font.char_height / 2), text, default_font);
		x += btn_w + 8;
	}
}

uint32_t hit_test(uint32_t px, uint32_t py, uint32_t screen_w, uint32_t screen_h, const window::Window* /*windows*/, uint32_t count) {
	const uint32_t h = height(screen_h);
	const uint32_t y = screen_h - h;
	if (py < y) return UINT32_MAX;
	uint32_t x = 8; // start button
	const uint32_t start_w = 80;
	if (px >= x && px < x + start_w && py >= y + 6 && py < y + 6 + (h - 12)) return UINT32_MAX;
	x += start_w + 8;

	for (uint32_t i = 0; i < count; ++i) {
		uint32_t btn_w = 140;
		if (x + btn_w + 8 > screen_w) break;
		if (px >= x && px < x + btn_w && py >= y + 6 && py < y + 6 + (h - 12)) return i;
		x += btn_w + 8;
	}
	return UINT32_MAX;
}

} }


