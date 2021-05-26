#ifndef __KERN_DRIVER_SDCARD_H__
#define __KERN_DRIVER_SDCARD_H__

void sdcard_init(void);

void sdcard_read_sector(uint8 *buf, int sectorno);

void sdcard_write_sector(uint8 *buf, int sectorno);

void test_sdcard(void);

#endif 
