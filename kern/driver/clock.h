#ifndef __KERN_DRIVER_CLOCK_H__
#define __KERN_DRIVER_CLOCK_H__

#include "libs/types.h"

void clockInit();
int setDateTime(int year, int mon, int day,
                int hour, int min, int sec);
        
int getDateTime(int *year, int *mon, int *day,
                int *hour, int *min, int *sec);

uint64 getTimeStamp();

#endif