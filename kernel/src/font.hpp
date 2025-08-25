#pragma once
#include <cstdint>

struct Font {
    const uint8_t* data;
    uint32_t width;
    uint32_t height;
    uint32_t char_width;
    uint32_t char_height;
    uint32_t chars_per_row;
};

// 8x8 font
extern const uint8_t font_8x8[];
extern const Font default_font;
