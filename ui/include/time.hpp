#pragma once

namespace platform {

struct DateTime {
  int year;
  int month;
  int day;
  int hour;
  int minute;
  int second;
  bool valid;
};

// Returns true and fills out with the current date/time if available.
// On unsupported platforms, returns false and leaves out.valid = false.
bool get_current_datetime(DateTime &out);

} // namespace platform
