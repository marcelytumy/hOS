#pragma once
#include <cstdint>
#include <limine.h>
#include "font.hpp"

class Graphics {
private:
    limine_framebuffer* framebuffer;
    uint32_t* fb_ptr;
    uint32_t width;
    uint32_t height;
    uint32_t pitch;
    
public:
    Graphics(limine_framebuffer* fb);
    
    // Basic pixel operations
    void set_pixel(uint32_t x, uint32_t y, uint32_t color);
    uint32_t get_pixel(uint32_t x, uint32_t y);
    
    // Text rendering
    void draw_char(char c, uint32_t x, uint32_t y, uint32_t color, const Font& font);
    void draw_string(const char* str, uint32_t x, uint32_t y, uint32_t color, const Font& font);
    void draw_string_centered(const char* str, uint32_t y, uint32_t color, const Font& font);
    void draw_char_scaled(char c, uint32_t x, uint32_t y, uint32_t color, const Font& font, uint32_t scale);
    void draw_string_scaled(const char* str, uint32_t x, uint32_t y, uint32_t color, const Font& font, uint32_t scale);
    void draw_string_centered_scaled(const char* str, uint32_t y, uint32_t color, const Font& font, uint32_t scale);
    
    // Bitmap rendering
    void draw_bitmap(const uint8_t* bitmap, uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t color);
    void draw_bitmap_rgba(const uint32_t* bitmap, uint32_t x, uint32_t y, uint32_t width, uint32_t height);
    
    // BMP file support
    struct BMPHeader {
        uint16_t signature;      // 'BM'
        uint32_t file_size;      // Size of the file
        uint16_t reserved1;      // Reserved
        uint16_t reserved2;      // Reserved
        uint32_t data_offset;    // Offset to image data
        uint32_t header_size;    // Size of DIB header
        int32_t width;          // Image width
        int32_t height;         // Image height
        uint16_t planes;        // Number of color planes
        uint16_t bits_per_pixel; // Bits per pixel
        uint32_t compression;   // Compression method
        uint32_t image_size;    // Size of image data
        int32_t x_pixels_per_meter; // Horizontal resolution
        int32_t y_pixels_per_meter; // Vertical resolution
        uint32_t colors_used;   // Number of colors in palette
        uint32_t important_colors; // Important colors
    };
    
    bool load_bmp(const uint8_t* bmp_data, uint32_t data_size, uint32_t*& image_data, uint32_t& width, uint32_t& height);
    void draw_bmp(const uint8_t* bmp_data, uint32_t data_size, uint32_t x, uint32_t y);
    void draw_bmp_centered(const uint8_t* bmp_data, uint32_t data_size, uint32_t y);
    
    // Utility functions
    void clear_screen(uint32_t color);
    void draw_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color);
    void fill_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color);
};
