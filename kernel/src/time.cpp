#include "../..//ui/include/time.hpp"
#include <cstdint>

namespace platform {

#if defined(__x86_64__)

static inline void outb(uint16_t port, uint8_t val) {
  asm volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
  uint8_t ret;
  asm volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
  return ret;
}

static inline uint8_t cmos_read(uint8_t reg) {
  // Select register (with NMI disabled by setting bit 7)
  outb(0x70, (uint8_t)(reg | 0x80));
  return inb(0x71);
}

static inline bool is_update_in_progress() {
  // Register A, bit 7 == UIP
  return (cmos_read(0x0A) & 0x80) != 0;
}

static inline uint8_t bcd_to_bin(uint8_t bcd) {
  return (uint8_t)((bcd & 0x0F) + ((bcd >> 4) * 10));
}

bool get_current_datetime(DateTime &out) {
  // Wait until any update in progress completes
  for (int i = 0; i < 1000000 && is_update_in_progress(); ++i) {
    // spin
  }

  uint8_t sec = cmos_read(0x00);
  uint8_t min = cmos_read(0x02);
  uint8_t hour = cmos_read(0x04);
  uint8_t day = cmos_read(0x07);
  uint8_t mon = cmos_read(0x08);
  uint8_t yr = cmos_read(0x09);
  uint8_t century = 0; // optional
  // Try to read century register (commonly 0x32 if provided)
  century = cmos_read(0x32);

  uint8_t regB = cmos_read(0x0B);

  bool is_binary = (regB & 0x04) != 0; // 1 = binary, 0 = BCD
  bool is_24h = (regB & 0x02) != 0;    // 1 = 24-hour, 0 = 12-hour

  if (!is_binary) {
    sec = bcd_to_bin(sec);
    min = bcd_to_bin(min);
    hour = bcd_to_bin(hour);
    day = bcd_to_bin(day);
    mon = bcd_to_bin(mon);
    yr = bcd_to_bin(yr);
    if (century != 0x00 && century != 0xFF) {
      century = bcd_to_bin(century);
    }
  }

  // Convert 12-hour to 24-hour if needed
  if (!is_24h) {
    bool is_pm = (hour & 0x80) != 0; // Some RTCs set bit 7 for PM
    hour = (uint8_t)(hour & 0x7F);
    if (is_pm && hour != 12)
      hour = (uint8_t)(hour + 12);
    if (!is_pm && hour == 12)
      hour = 0;
  }

  int full_year = 2000 + (int)yr;
  if (century != 0 && century != 0xFF) {
    full_year = (int)century * 100 + (int)yr;
  } else if (yr >= 70) {
    full_year = 1900 + (int)yr;
  }

  out.year = full_year;
  out.month = (int)mon;
  out.day = (int)day;
  out.hour = (int)hour;
  out.minute = (int)min;
  out.second = (int)sec;
  out.valid = true;
  return true;
}

#else

bool get_current_datetime(DateTime &out) {
  (void)out;
  return false;
}

#endif

} // namespace platform
