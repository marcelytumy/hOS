#include "graphics.hpp"
#include "font.hpp"
#include "../include/window.hpp"

namespace ui { namespace window {

static constexpr uint32_t kWindowBg = 0x262626;
static constexpr uint32_t kWindowBorder = 0x4A4A4A;
static constexpr uint32_t kTitlebarBg = 0x333333;
static constexpr uint32_t kTitleText = 0xFFFFFF;

uint32_t get_titlebar_height() { return 28; }

bool point_in_titlebar(const Window &w, uint32_t x, uint32_t y) {
	const uint32_t th = get_titlebar_height();
	return x >= w.rect.x + 1 && x < w.rect.x + w.rect.w - 1 &&
	       y >= w.rect.y + 1 && y < w.rect.y + 1 + th;
}

void draw(Graphics &gfx, const Window &w) {
	if (w.minimized) return;
	// Frame
	gfx.fill_rect(w.rect.x, w.rect.y, w.rect.w, w.rect.h, kWindowBg);
	gfx.draw_rect(w.rect.x, w.rect.y, w.rect.w, w.rect.h, kWindowBorder);

	// Titlebar
	const uint32_t th = get_titlebar_height();
	gfx.fill_rect(w.rect.x + 1, w.rect.y + 1, w.rect.w - 2, th, kTitlebarBg);

	// Title
	if (w.title) {
		gfx.draw_string(w.title, w.rect.x + 12, w.rect.y + 6, kTitleText, default_font);
	}

	// Placeholder content
	gfx.draw_string("This is a placeholder window.", w.rect.x + 12, w.rect.y + th + 12, 0xCCCCCC, default_font);
}

} }


