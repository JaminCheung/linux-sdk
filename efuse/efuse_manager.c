/*
 *  Copyright (C) 2016, Wang Qiuwei <qiuwei.wang@ingenic.com, panddio@163.com>
 *
 *  Ingenic QRcode SDK Project
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under  the terms of the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the License, or (at your
 *  option) any later version.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/types.h>

#include <utils/log.h>
#include <utils/assert.h>
#include <efuse/efuse_manager.h>

/*
 * Macros
 */
#define DEFAULT_EFUSE_DEV   "/dev/jz-efuse-v13"
#define LOG_TAG  "efuse"


static int32_t efuse_read(enum efuse_segment seg_id, uint32_t *buf, uint32_t length) {
    int fd;
    struct efuse_wr_info info;

    fd = open(DEFAULT_EFUSE_DEV, O_RDONLY);
    assert_die_if(fd < 0, "Failed to open %s: %s\n", \
            DEFAULT_EFUSE_DEV, strerror(errno));

    if (seg_id < 0 || seg_id > SEGMENT_END) {
        LOGE("The segment ID: %d is invalid!\n", seg_id);
        return -1;
    }

    info.seg_id      = seg_id;
    info.data_length = length;
    info.buf         = buf;

    if (ioctl(fd, CMD_READ, &info) < 0) {
        LOGE("Failed to read segment ID: %d, %s\n", seg_id, strerror(errno));
        return -1;
    }

    close(fd);
    return 0;
}

static int32_t efuse_write(enum efuse_segment seg_id, uint32_t *buf, uint32_t length) {
    int fd;
    struct efuse_wr_info info;

    fd = open(DEFAULT_EFUSE_DEV, O_WRONLY);
    assert_die_if(fd < 0, "Failed to open %s: %s\n", \
            DEFAULT_EFUSE_DEV, strerror(errno));

    if (seg_id < 0 || seg_id > SEGMENT_END) {
        LOGE("The segment ID: %d is invalid!\n", seg_id);
        return -1;
    }

    info.seg_id      = seg_id;
    info.data_length = length;
    info.buf         = buf;

    if (ioctl(fd, CMD_WRITE, &info) < 0) {
        LOGE("Failed to write segment ID: %d, %s\n", seg_id, strerror(errno));
        return -1;
    }

    close(fd);
    return 0;

}

struct efuse_manager efuse_manager = {
    .read  = efuse_read,
    .write = efuse_write,
};

struct efuse_manager *get_efuse_manager(void) {
    return &efuse_manager;
}
