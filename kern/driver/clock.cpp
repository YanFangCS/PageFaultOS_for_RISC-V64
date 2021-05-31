#include "libs/types.h"
#include "libs/printf.h"
#include "proc/proc.h"
#include "driver/rtc.h"
#include "driver/clock.h"

bool leapYear[100];

int setDateTime(int year, int mon, int day, int hour, int min, int sec) {
    return rtc_timer_set(year, mon, day, hour, min, sec);
}

int getDateTime(int *year, int *mon, int *day, int *hour, int *min, int *sec) {
  return rtc_timer_get(year, mon, day, hour, min, sec);
}

uint64 getTimestamp() {
  struct tm *tm = rtc_timer_get_tm();
  int off = tm->tm_year - 1970;
  int leaps = leapYear[off - 1];
  uint64 days = leaps * 366 + (off - 1 - leaps) * 365 + tm->tm_yday;
  long ts = days * 86400 + tm->tm_hour * 3600 + tm->tm_sec;
  return ts;
}

void initLeapYear() {
  int year = 0;
  year = 1970;
  leapYear[0] = 0;  // 1970不是闰年
  for (int i = 1; i < 100; i++) {
    year = i + 1970;
    if (((year % 4 == 0) && (year % 100 != 0)) || (year % 400 == 0)) {
      leapYear[i] += leapYear[i] + 1;
    } else {
      leapYear[i] += leapYear[i];
    }
  }
}

void initClock() {
  rtc_init();
  initLeapYear();
  rtc_timer_set(2021, 5, 30, 0, 0, 0);
}
