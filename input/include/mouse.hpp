#pragma once
#include <cstdint>

namespace input {

// Simple PS/2 mouse packet decoder (polled access via ports 0x60/0x64)
class Ps2Mouse {
public:
  struct State {
    int32_t x;
    int32_t y;
    bool left;
    bool right;
    bool middle;
  };

  Ps2Mouse();
  bool initialize();
  bool poll_packet(int8_t &dx, int8_t &dy, int8_t &dz, bool &left, bool &right,
                   bool &middle);

private:
  bool wheel_supported_ = false;
  bool write_command(uint8_t value);
  bool write_device(uint8_t value);
  bool read_data(uint8_t &value);
  bool wait_read();
  bool wait_write();
};

} // namespace input
