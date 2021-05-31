#include "fs/devicefs.h"
#include "console/console.h"
#include "driver/disk.h"
#include "driver/sdcard.h"

extern struct console console;

void DeviceFS::init() {}

int DeviceFS::open(const char * filepath, uint64 flags, struct file * fp){
    fp->type = fp->FD_DEVICE;
    fp->directory = false;
    return 0;
}

size_t DeviceFS::read(const char *path, bool user, char *buf, int offset, int n) {
    if (strncmp(path, "/dev/tty", strlen(path)) == 0) {
        return consoleread(reinterpret_cast<uint64>(buf), user, n);
    }
#ifdef K210
    else if (strncmp(path, "/dev/vda2", strlen(path)) == 0) {
        sdcard_read_sector(reinterpret_cast<uint8 *>(buf), offset / 512);
    } else if (strncmp(path, "/dev/hda1", strlen("/dev/hda1")) == 0) {
        // LOG_DEBUG("read hda1");
        sdcard_read_sector(reinterpret_cast<uint8 *>(buf), offset / 512);
    } else {
    panic("not support path");
    }
#endif
    return 0;
}

size_t DeviceFS::write(const char *path, bool user, const char *buf, int offset, int n) {
    if (strncmp(path, "/dev/tty", strlen(path)) == 0) {
        return consolewrite(reinterpret_cast<uint64>(buf), user, n);
    }
#ifdef K210
    else if (strncmp(path, "/dev/vda2", strlen("/dev/vda2")) == 0) {
        sdcard_write_sector((uint8 *)(buf), offset / 512);
    } else if (strncmp(path, "/dev/hda1", strlen("/dev/hda1")) == 0) {
        sdcard_write_sector((uint8 *)(buf), offset / 512);
    } else {
        panic("not support path");
    }
#endif
    return 0;
}

size_t DeviceFS::truncate(const char *filepath, size_t size) {return 0;}

size_t DeviceFS::clear(const char *filepath, size_t count, size_t offset, size_t &written) { return 0; }

int DeviceFS::get_file(const char *filepath, struct file *fp) { return 0; }

size_t DeviceFS::touch(const char *filepath) { return 0; }

size_t DeviceFS::rm(const char *filepath) { return 0; }

int DeviceFS::ls(const char *filepath, char *contents, int len,bool user) { return 0; }
