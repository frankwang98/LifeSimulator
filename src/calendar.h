#pragma once

#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>

namespace life {
struct Date {
  int year = 2026;
  int month = 10;
  int day = 1;
};
inline bool leapYear(int year) {
  return year % 4 == 0 && (year % 100 != 0 || year % 400 == 0);
}
inline int monthDays(int year, int month) {
  constexpr int lengths[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  return lengths[month - 1] + (month == 2 && leapYear(year) ? 1 : 0);
}
inline Date parseDate(const std::string& text) {
  Date date;
  char first, second;
  std::istringstream input(text);
  if (text.size() != 10 || text[4] != '-' || text[7] != '-' ||
      !(input >> date.year >> first >> date.month >> second >> date.day) || first != '-' ||
      second != '-' || date.year < 1900 || date.year > 2200 || date.month < 1 || date.month > 12 ||
      date.day < 1 || date.day > monthDays(date.year, date.month)) {
    throw std::invalid_argument("start_date must be a valid YYYY-MM-DD (1900..2200)");
  }
  return date;
}
inline std::string dateString(Date date) {
  std::ostringstream output;
  output << std::setfill('0') << std::setw(4) << date.year << '-' << std::setw(2) << date.month
         << '-' << std::setw(2) << date.day;
  return output.str();
}
inline Date nextDate(Date date) {
  if (++date.day > monthDays(date.year, date.month)) {
    date.day = 1;
    if (++date.month > 12) {
      date.month = 1;
      ++date.year;
    }
  }
  return date;
}
// Gregorian weekday: 0=Sunday, 6=Saturday. No host timezone dependency.
inline int weekday(Date date) {
  constexpr int offsets[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
  int year = date.year - (date.month < 3 ? 1 : 0);
  return (year + year / 4 - year / 100 + year / 400 + offsets[date.month - 1] + date.day) % 7;
}
}  // namespace life
