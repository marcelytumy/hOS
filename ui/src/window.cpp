#include "graphics.hpp"
#include "font.hpp"
#include "../include/window.hpp"
#include "../include/taskbar.hpp"

namespace ui { namespace window {

static constexpr uint32_t kWindowBg = 0x262626;
static constexpr uint32_t kWindowBorder = 0x4A4A4A;
static constexpr uint32_t kWindowBorderFocused = 0x7A7A7A;
static constexpr uint32_t kTitlebarBg = 0x333333;
static constexpr uint32_t kTitlebarBgFocused = 0x3D3D3D;
static constexpr uint32_t kTitleText = 0xFFFFFF;
static constexpr uint32_t kTitleTextUnfocused = 0xDDDDDD;
// Left-side titlebar button colors
static constexpr uint32_t kBtnCloseBg = 0xD84C4C;   // red
static constexpr uint32_t kBtnCloseBorder = 0x9E2F2F;
static constexpr uint32_t kBtnMinBg = 0xE5C04B;     // yellow
static constexpr uint32_t kBtnMinBorder = 0xA3861F;
static constexpr uint32_t kBtnMaxBg = 0x58C26E;     // green
static constexpr uint32_t kBtnMaxBorder = 0x2E7D49;
static constexpr uint32_t kBtnPinBg = 0x9B59B6;     // purple
static constexpr uint32_t kBtnPinBorder = 0x6F3A86;

uint32_t get_titlebar_height() { return 28; }

bool point_in_titlebar(const Window &w, uint32_t x, uint32_t y) {
	if (w.minimized) return false;
	if (!w.movable || !w.draggable) return false;

	const uint32_t th = get_titlebar_height();

	uint32_t rx = w.rect.x;
	uint32_t ry = w.rect.y;
	uint32_t rw = w.rect.w;

	// Exclude the left button cluster (close, minimize, maximize, pin)
	const uint32_t btn_margin = 6;
	const uint32_t btn_size = th - 12; // square
	const uint32_t btn_spacing = 6;
	uint32_t num_buttons = 4u - (w.closeable ? 0u : 1u);
	const uint32_t cluster_w = num_buttons ? (btn_size * num_buttons) + (btn_spacing * (num_buttons - 1)) : 0u;
	uint32_t left_exclude_end = rx + 1 + btn_margin + cluster_w;

	return x >= left_exclude_end && x < rx + rw - 1 &&
	       y >= ry + 1 && y < ry + 1 + th;
}

uint32_t hit_test_button(const Window &w, uint32_t x, uint32_t y) {
	if (w.minimized) return UINT32_MAX;
	const uint32_t th = get_titlebar_height();
	uint32_t rx = w.rect.x;
	uint32_t ry = w.rect.y;
	uint32_t rw = w.rect.w;
	(void)rw;
	const uint32_t btn_margin = 6;
	const uint32_t btn_size = th - 12;
	const uint32_t btn_spacing = 6;
	uint32_t bx = rx + 1 + btn_margin;
	const uint32_t by = ry + 1 + 6;
	uint32_t idx = 0;
	// Close (optional)
	if (w.closeable) {
		if (x >= bx && x < bx + btn_size && y >= by && y < by + btn_size) return idx;
		bx += btn_size + btn_spacing;
	} else {
		idx = 1; // skip close index so minimize is 1
	}
	// Minimize
	if (x >= bx && x < bx + btn_size && y >= by && y < by + btn_size) return 1;
	bx += btn_size + btn_spacing;
	// Maximize
	if (x >= bx && x < bx + btn_size && y >= by && y < by + btn_size) return 2;
	bx += btn_size + btn_spacing;
	// Pin
	if (x >= bx && x < bx + btn_size && y >= by && y < by + btn_size) return 3;
	return UINT32_MAX;
}

void draw(Graphics &gfx, const Window &w) {
	if (w.minimized) return;

	// Effective rect based on fullscreen/maximized
	uint32_t rx = w.rect.x;
	uint32_t ry = w.rect.y;
	uint32_t rw = w.rect.w;
	uint32_t rh = w.rect.h;

	const uint32_t screen_w = gfx.get_width();
	const uint32_t screen_h = gfx.get_height();

	if (w.fullscreen) {
		rx = 0; ry = 0; rw = screen_w; rh = screen_h;
	} else if (w.maximized) {
		rx = 0; ry = 0; rw = screen_w; rh = screen_h;
		const uint32_t tb_h = ui::taskbar::height(screen_h);
		if (rh > tb_h) rh -= tb_h; // keep taskbar visible
	}

	// Colors depending on focus
	const bool is_focused = w.focused;
	const uint32_t border_col = is_focused ? kWindowBorderFocused : kWindowBorder;
	const uint32_t titlebar_col = is_focused ? kTitlebarBgFocused : kTitlebarBg;
	const uint32_t titletext_col = is_focused ? kTitleText : kTitleTextUnfocused;

	// Frame
	gfx.fill_rect(rx, ry, rw, rh, kWindowBg);
	gfx.draw_rect(rx, ry, rw, rh, border_col);

	// Titlebar
	const uint32_t th = get_titlebar_height();
	gfx.fill_rect(rx + 1, ry + 1, rw - 2, th, titlebar_col);

	// Left-side buttons: close, minimize, maximize, pin
	{
		const uint32_t btn_margin = 6;
		const uint32_t btn_size = th - 12; // square buttons
		const uint32_t btn_spacing = 6;
		uint32_t bx = rx + 1 + btn_margin;
		const uint32_t by = ry + 1 + 6;

		// Close (optional)
		if (w.closeable) {
			gfx.fill_rect(bx, by, btn_size, btn_size, kBtnCloseBg);
			gfx.draw_rect(bx, by, btn_size, btn_size, kBtnCloseBorder);
			bx += btn_size + btn_spacing;
		}
		// Minimize
		gfx.fill_rect(bx, by, btn_size, btn_size, kBtnMinBg);
		gfx.draw_rect(bx, by, btn_size, btn_size, kBtnMinBorder);
		bx += btn_size + btn_spacing;
		// Maximize
		gfx.fill_rect(bx, by, btn_size, btn_size, kBtnMaxBg);
		gfx.draw_rect(bx, by, btn_size, btn_size, kBtnMaxBorder);
		bx += btn_size + btn_spacing;
		// Pin (always-on-top)
		gfx.fill_rect(bx, by, btn_size, btn_size, kBtnPinBg);
		gfx.draw_rect(bx, by, btn_size, btn_size, kBtnPinBorder);
	}

	// Title
	if (w.title) {
		const uint32_t btn_margin = 6;
		const uint32_t btn_size = th - 12;
		const uint32_t btn_spacing = 6;
		uint32_t num_buttons = 4u - (w.closeable ? 0u : 1u);
		const uint32_t cluster_w = num_buttons ? (btn_size * num_buttons) + (btn_spacing * (num_buttons - 1)) : 0u;
		uint32_t text_x = rx + 1 + btn_margin + cluster_w + 8; // some gap after buttons
		gfx.draw_string(w.title, text_x, ry + 6, titletext_col, default_font);
	}

	// Resizable handle (skip when fullscreen/maximized)
	if (w.resizable && !w.fullscreen && !w.maximized) {
		uint32_t handle_col = is_focused ? 0x858585 : 0x6A6A6A;
		for (uint32_t i = 0; i < 3; ++i) {
			uint32_t ox = 4 * i;
			uint32_t oy = 4 * i;
			if (rw >= 2 + ox + 2 && rh >= 2 + oy + 2) {
				gfx.fill_rect(rx + rw - 2 - (2 + ox), ry + rh - 2 - (2 + oy), 2, 2, handle_col);
			}
		}
	}

	// Placeholder content
	gfx.draw_string("This is a placeholder window.", rx + 12, ry + th + 12, 0xCCCCCC, default_font);
}

} }


