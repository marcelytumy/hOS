#pragma once
#include <cstdint>

class Graphics;

namespace ui {

class Cursor {
public:
  Cursor();
  void set_position(uint32_t x, uint32_t y, uint32_t screen_w,
                    uint32_t screen_h);
  void move_by(int32_t dx, int32_t dy, uint32_t screen_w, uint32_t screen_h);
  void draw(Graphics &gfx);
  void erase(Graphics &gfx);
  // Invalidate saved underlay when the background was redrawn
  void invalidate();

  inline uint32_t x() const { return pos_x; }
  inline uint32_t y() const { return pos_y; }

private:
  uint32_t pos_x;
  uint32_t pos_y;
  // Saved underlay pixel to restore when erasing (simple 1-pixel pointer)
  uint32_t saved_pixel;
  bool has_saved;
};

} // namespace ui
