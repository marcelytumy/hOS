#include <cstdint>
#if defined(__x86_64__)
static inline void outb(uint16_t port, uint8_t val) {
  asm volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}
static inline uint8_t inb(uint16_t port) {
  uint8_t ret;
  asm volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
  return ret;
}
#endif

#include "../include/mouse.hpp"

namespace input {

// Controller ports
static constexpr uint16_t PS2_DATA = 0x60;
static constexpr uint16_t PS2_STATUS = 0x64;
static constexpr uint16_t PS2_CMD = 0x64;

// Status bits
static constexpr uint8_t PS2_STATUS_OUTPUT = 1 << 0; // Data available to read
static constexpr uint8_t PS2_STATUS_INPUT = 1 << 1;  // Input buffer full

Ps2Mouse::Ps2Mouse() {}

bool Ps2Mouse::wait_read() {
#if defined(__x86_64__)
  for (int i = 0; i < 100000; ++i) {
    if (inb(PS2_STATUS) & PS2_STATUS_OUTPUT)
      return true;
  }
#endif
  return false;
}

bool Ps2Mouse::wait_write() {
#if defined(__x86_64__)
  for (int i = 0; i < 100000; ++i) {
    if ((inb(PS2_STATUS) & PS2_STATUS_INPUT) == 0)
      return true;
  }
#endif
  return false;
}

bool Ps2Mouse::read_data(uint8_t &value) {
#if defined(__x86_64__)
  if (!wait_read())
    return false;
  value = inb(PS2_DATA);
  return true;
#else
  (void)value;
  return false;
#endif
}

bool Ps2Mouse::write_command(uint8_t value) {
#if defined(__x86_64__)
  if (!wait_write())
    return false;
  outb(PS2_CMD, value);
  return true;
#else
  (void)value;
  return false;
#endif
}

bool Ps2Mouse::write_device(uint8_t value) {
#if defined(__x86_64__)
  if (!wait_write())
    return false;
  outb(PS2_DATA, value);
  return true;
#else
  return false;
#endif
}

bool Ps2Mouse::initialize() {
#if defined(__x86_64__)
  // Enable auxiliary device
  if (!write_command(0xA8))
    return false;
  // Enable IRQ12 (read controller command byte, set bit1)
  if (!write_command(0x20))
    return false; // Read command byte
  uint8_t cmd = 0;
  if (!read_data(cmd))
    return false;
  cmd |= 0x02; // enable IRQ12
  if (!wait_write())
    return false;
  outb(PS2_CMD, 0x60); // write command byte
  if (!wait_write())
    return false;
  outb(PS2_DATA, cmd);

  // Try to enable IntelliMouse scroll wheel (set sample rate sequence)
  // Send to mouse: 0xF3 200, 0xF3 100, 0xF3 80 then read ID 0xF2
  uint8_t ack = 0;
  if (!write_command(0xD4))
    return false; // to mouse
  if (!write_device(0xF3))
    return false; // set sample rate
  if (!read_data(ack))
    return false;
  if (!write_command(0xD4))
    return false;
  if (!write_device(200))
    return false;
  if (!read_data(ack))
    return false;
  if (!write_command(0xD4))
    return false;
  if (!write_device(0xF3))
    return false;
  if (!read_data(ack))
    return false;
  if (!write_command(0xD4))
    return false;
  if (!write_device(100))
    return false;
  if (!read_data(ack))
    return false;
  if (!write_command(0xD4))
    return false;
  if (!write_device(0xF3))
    return false;
  if (!read_data(ack))
    return false;
  if (!write_command(0xD4))
    return false;
  if (!write_device(80))
    return false;
  if (!read_data(ack))
    return false;
  // Get device ID
  if (!write_command(0xD4))
    return false;
  if (!write_device(0xF2))
    return false; // Get device ID
  if (!read_data(ack))
    return false; // ACK
  uint8_t id = 0;
  if (!read_data(id))
    return false; // ID
  wheel_supported_ = (id == 3);

  // Enable data reporting
  if (!write_command(0xD4))
    return false;
  if (!write_device(0xF4))
    return false; // Enable data reporting
  if (!read_data(ack))
    return false; // ACK 0xFA
  return ack == 0xFA;
#else
  return false;
#endif
}

bool Ps2Mouse::poll_packet(int8_t &dx, int8_t &dy, int8_t &dz, bool &left,
                           bool &right, bool &middle) {
#if defined(__x86_64__)
  // Attempt to read a 3-byte or 4-byte packet
  if (!wait_read())
    return false;
  uint8_t b1 = inb(PS2_DATA);
  if ((b1 & 0x08) == 0)
    return false; // sync bit not set
  if (!wait_read())
    return false;
  uint8_t b2 = inb(PS2_DATA);
  if (!wait_read())
    return false;
  uint8_t b3 = inb(PS2_DATA);

  left = b1 & 0x01;
  right = b1 & 0x02;
  middle = b1 & 0x04;
  int8_t x = static_cast<int8_t>(b2);
  int8_t y = static_cast<int8_t>(b3);
  // Y is typically negative upward; convert to screen coords (down positive)
  dx = x;
  dy = -y;
  // Read 4th byte if wheel supported
  dz = 0;
  if (wheel_supported_) {
    if (!wait_read())
      return true; // if no 4th byte yet, return movement only
    uint8_t b4 = inb(PS2_DATA);
    dz = static_cast<int8_t>(b4);
  }
  return true;
#else
  (void)dx;
  (void)dy;
  (void)dz;
  (void)left;
  (void)right;
  (void)middle;
  return false;
#endif
}

} // namespace input
