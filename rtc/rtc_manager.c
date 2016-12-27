/*
 *  Copyright (C) 2016, Xin Shuan <shuan.xin@ingenic.com>
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
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <utils/log.h>
#include <utils/assert.h>
#include <rtc/rtc_manager.h>


/*
 * Macros
 */
#define LOG_TAG             "rtc"
#define RTC_DEV              "/dev/rtc0"


static int32_t rtc_read(struct rtc_time *time) {
    int fd;
    assert_die_if(time == NULL, "rtc read time , time is null\n");

    fd = open(RTC_DEV, O_RDONLY);
    if(fd < 0) {
        LOGE("Failed to read rtc time : open errno %s\n", strerror(errno));
        return -1;
    }

    memset(time, 0, sizeof(*time));

    if (ioctl(fd, RTC_RD_TIME, time) < 0) {
        close(fd);
        LOGE("Failed to read rtc time : ioctl errno %s\n", strerror(errno));
        return -1;
    }

    close(fd);
    return 0;
}

static int32_t rtc_write(const struct rtc_time *time) {
    int fd;
    assert_die_if(time == NULL, "rtc write time , time is null\n");

    fd = open(RTC_DEV, O_WRONLY);
    if(fd < 0) {
        LOGE("Failed to write rtc time : open errno %s\n", strerror(errno));
        return -1;
    }

    if (ioctl(fd, RTC_SET_TIME, time) < 0) {
        close(fd);
        LOGE("Failed to write rtc time : ioctl errno %s\n", strerror(errno));
        return -1;
    }

    close(fd);
    return 0;
}

struct rtc_manager rtc_manager = {
    .read   = rtc_read,
    .write  = rtc_write,
};

struct rtc_manager *get_rtc_manager(void) {
    return &rtc_manager;
}
