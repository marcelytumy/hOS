#include "graphics.hpp"
#include "font.hpp"

Graphics::Graphics(limine_framebuffer* fb) {
    framebuffer = fb;
    fb_ptr = static_cast<uint32_t*>(framebuffer->address);
    width = framebuffer->width;
    height = framebuffer->height;
    pitch = framebuffer->pitch / 4; // Assuming 32-bit pixels
}

void Graphics::set_pixel(uint32_t x, uint32_t y, uint32_t color) {
    if (x < width && y < height) {
        fb_ptr[y * pitch + x] = color;
    }
}

uint32_t Graphics::get_pixel(uint32_t x, uint32_t y) {
    if (x < width && y < height) {
        return fb_ptr[y * pitch + x];
    }
    return 0;
}

void Graphics::draw_char(char c, uint32_t x, uint32_t y, uint32_t color, const Font& font) {
    if (c < 32 || c > 126) return; // Only printable ASCII
    
    uint32_t char_index = c - 32;
    // Each character is 8 bytes (8 rows), so we need to access the correct character data
    const uint8_t* char_data = &font.data[char_index * 8];
    
    for (uint32_t py = 0; py < font.char_height; py++) {
        uint8_t row_data = char_data[py];
        for (uint32_t px = 0; px < font.char_width; px++) {
            // Check if the bit is set (left to right)
            if (row_data & (0x80 >> px)) {
                set_pixel(x + px, y + py, color);
            }
        }
    }
}

void Graphics::draw_string(const char* str, uint32_t x, uint32_t y, uint32_t color, const Font& font) {
    uint32_t current_x = x;
    for (const char* c = str; *c; c++) {
        if (*c == '\n') {
            y += font.char_height;
            current_x = x;
        } else {
            draw_char(*c, current_x, y, color, font);
            current_x += font.char_width;
        }
    }
}

void Graphics::draw_bitmap(const uint8_t* bitmap, uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t color) {
    for (uint32_t py = 0; py < height; py++) {
        for (uint32_t px = 0; px < width; px++) {
            uint32_t pixel_index = py * width + px;
            uint32_t byte_index = pixel_index / 8;
            uint32_t bit_index = pixel_index % 8;
            
            if (bitmap[byte_index] & (1 << (7 - bit_index))) {
                set_pixel(x + px, y + py, color);
            }
        }
    }
}

void Graphics::draw_bitmap_rgba(const uint32_t* bitmap, uint32_t x, uint32_t y, uint32_t width, uint32_t height) {
    for (uint32_t py = 0; py < height; py++) {
        for (uint32_t px = 0; px < width; px++) {
            uint32_t color = bitmap[py * width + px];
            if ((color & 0xFF000000) != 0) { // Check alpha channel
                set_pixel(x + px, y + py, color);
            }
        }
    }
}

void Graphics::clear_screen(uint32_t color) {
    for (uint32_t y = 0; y < height; y++) {
        for (uint32_t x = 0; x < width; x++) {
            set_pixel(x, y, color);
        }
    }
}

void Graphics::draw_string_centered(const char* str, uint32_t y, uint32_t color, const Font& font) {
    // Calculate string width
    uint32_t string_width = 0;
    for (const char* c = str; *c; c++) {
        if (*c == '\n') break;
        string_width += font.char_width;
    }
    
    // Calculate x position to center the string
    uint32_t x = (width - string_width) / 2;
    draw_string(str, x, y, color, font);
}

void Graphics::draw_char_scaled(char c, uint32_t x, uint32_t y, uint32_t color, const Font& font, uint32_t scale) {
    if (c < 32 || c > 126) return; // Only printable ASCII
    
    uint32_t char_index = c - 32;
    const uint8_t* char_data = &font.data[char_index * 8];
    
    for (uint32_t py = 0; py < font.char_height; py++) {
        uint8_t row_data = char_data[py];
        for (uint32_t px = 0; px < font.char_width; px++) {
            if (row_data & (0x80 >> px)) {
                // Draw scaled pixel
                for (uint32_t sy = 0; sy < scale; sy++) {
                    for (uint32_t sx = 0; sx < scale; sx++) {
                        set_pixel(x + px * scale + sx, y + py * scale + sy, color);
                    }
                }
            }
        }
    }
}

void Graphics::draw_string_scaled(const char* str, uint32_t x, uint32_t y, uint32_t color, const Font& font, uint32_t scale) {
    uint32_t current_x = x;
    for (const char* c = str; *c; c++) {
        if (*c == '\n') {
            y += font.char_height * scale;
            current_x = x;
        } else {
            draw_char_scaled(*c, current_x, y, color, font, scale);
            current_x += font.char_width * scale;
        }
    }
}

void Graphics::draw_string_centered_scaled(const char* str, uint32_t y, uint32_t color, const Font& font, uint32_t scale) {
    // Calculate string width
    uint32_t string_width = 0;
    for (const char* c = str; *c; c++) {
        if (*c == '\n') break;
        string_width += font.char_width * scale;
    }
    
    // Calculate x position to center the string
    uint32_t x = (width - string_width) / 2;
    draw_string_scaled(str, x, y, color, font, scale);
}

bool Graphics::load_bmp(const uint8_t* bmp_data, uint32_t data_size, uint32_t*& image_data, uint32_t& width, uint32_t& height) {
    if (data_size < sizeof(BMPHeader)) {
        return false;
    }
    
    const BMPHeader* header = reinterpret_cast<const BMPHeader*>(bmp_data);
    
    // Check BMP signature
    if (header->signature != 0x4D42) { // 'BM'
        return false;
    }
    
    // Check if it's a supported BMP format (8-bit, 24-bit, or 32-bit)
    if (header->bits_per_pixel != 8 && header->bits_per_pixel != 24 && header->bits_per_pixel != 32) {
        return false;
    }
    
    // Check if compression is supported (0 = no compression)
    if (header->compression != 0) {
        return false;
    }
    
    width = static_cast<uint32_t>(header->width);
    height = static_cast<uint32_t>(header->height);
    
    // Check if the image is too large for stack allocation
    if (width > 1024 || height > 1024 || width * height > 1024 * 1024) {
        return false;
    }
    
    // Use static buffer instead of dynamic allocation
    static uint32_t image_buffer[1024 * 1024]; // 1MB buffer
    image_data = image_buffer;
    
    // Calculate row size (BMP rows are padded to 4-byte boundaries)
    uint32_t row_size = (width * header->bits_per_pixel + 31) / 32 * 4;
    
    // Get pointer to color palette (for 8-bit BMPs)
    const uint8_t* palette_ptr = bmp_data + sizeof(BMPHeader);
    
    // Get pointer to image data
    const uint8_t* image_ptr = bmp_data + header->data_offset;
    
    // BMP is stored bottom-to-top, so we need to flip it
    for (uint32_t y = 0; y < height; y++) {
        uint32_t src_y = height - 1 - y; // Flip vertically
        const uint8_t* row_ptr = image_ptr + src_y * row_size;
        
        for (uint32_t x = 0; x < width; x++) {
            uint32_t pixel_index = y * width + x;
            
            if (header->bits_per_pixel == 8) {
                // 8-bit indexed color
                uint8_t palette_index = row_ptr[x];
                // Each palette entry is 4 bytes (BGR + reserved)
                const uint8_t* color_entry = palette_ptr + palette_index * 4;
                uint8_t b = color_entry[0];
                uint8_t g = color_entry[1];
                uint8_t r = color_entry[2];
                // Skip transparent pixels (assuming index 0 is transparent)
                if (palette_index != 0) {
                    image_data[pixel_index] = (r << 16) | (g << 8) | b;
                } else {
                    image_data[pixel_index] = 0; // Transparent
                }
            } else if (header->bits_per_pixel == 24) {
                // 24-bit BGR
                uint8_t b = row_ptr[x * 3];
                uint8_t g = row_ptr[x * 3 + 1];
                uint8_t r = row_ptr[x * 3 + 2];
                image_data[pixel_index] = (r << 16) | (g << 8) | b;
            } else {
                // 32-bit BGRA
                uint8_t b = row_ptr[x * 4];
                uint8_t g = row_ptr[x * 4 + 1];
                uint8_t r = row_ptr[x * 4 + 2];
                uint8_t a = row_ptr[x * 4 + 3];
                image_data[pixel_index] = (a << 24) | (r << 16) | (g << 8) | b;
            }
        }
    }
    
    return true;
}

void Graphics::draw_bmp(const uint8_t* bmp_data, uint32_t data_size, uint32_t x, uint32_t y) {
    uint32_t* image_data = nullptr;
    uint32_t width, height;
    
    if (load_bmp(bmp_data, data_size, image_data, width, height)) {
        draw_bitmap_rgba(image_data, x, y, width, height);
    }
}

void Graphics::draw_bmp_centered(const uint8_t* bmp_data, uint32_t data_size, uint32_t y) {
    uint32_t* image_data = nullptr;
    uint32_t width, height;
    
    if (load_bmp(bmp_data, data_size, image_data, width, height)) {
        uint32_t x = (this->width - width) / 2;
        draw_bitmap_rgba(image_data, x, y, width, height);
    }
}

void Graphics::draw_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color) {
    // Draw top line
    for (uint32_t i = 0; i < w; i++) {
        set_pixel(x + i, y, color);
    }
    // Draw bottom line
    for (uint32_t i = 0; i < w; i++) {
        set_pixel(x + i, y + h - 1, color);
    }
    // Draw left line
    for (uint32_t i = 0; i < h; i++) {
        set_pixel(x, y + i, color);
    }
    // Draw right line
    for (uint32_t i = 0; i < h; i++) {
        set_pixel(x + w - 1, y + i, color);
    }
}

void Graphics::fill_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color) {
    for (uint32_t py = 0; py < h; py++) {
        for (uint32_t px = 0; px < w; px++) {
            set_pixel(x + px, y + py, color);
        }
    }
}
