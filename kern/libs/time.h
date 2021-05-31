#ifndef __KERN_LIBS_TIME_H__
#define __KERN_LIBS_TIME_H__

#include "libs/types.h"

struct tm {
    int tm_sec;
    int tm_min;
    int tm_hour;
    int tm_mday;
    int tm_mon;
    int tm_year;
    int tm_wday;
    int tm_yday;
    int tm_isdst;
};

struct tms {
    long tms_utime;
    long tms_stime;
    long tms_cutime;
    long tms_cstime;
};

typedef struct {
    uint64 sec;
    uint64 usec;
} TimeVal;

#endif