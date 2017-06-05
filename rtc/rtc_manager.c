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

#define LOG_TAG "rtc_manager"
#define RTC_DEV "/dev/rtc0"

#define JZ_RTC_HIBERNATE_STATUS _IOR('r', 0x01, int)    /* Hibernate wakeup status  */

static int fd;
static pthread_mutex_t init_lock = PTHREAD_MUTEX_INITIALIZER;

static int32_t init(void) {
    int error = 0;

    pthread_mutex_lock(&init_lock);
    if (fd > 0) {
        LOGE("rtc manager already init.\n");
        goto error;
    }

    fd = open(RTC_DEV, O_RDWR);
    if(fd < 0) {
        LOGE("Failed to open %s: %s\n", RTC_DEV, strerror(errno));
        goto error;
    }

    error = ioctl(fd, RTC_AIE_OFF, 0);
    if (error < 0) {
        LOGE("Failed to disable rtc aie: %s\n", strerror(errno));
        goto error;
    }

    pthread_mutex_unlock(&init_lock);

    return 0;

error:
    if (fd > 0)
        close(fd);
    pthread_mutex_unlock(&init_lock);

    return -1;
}

static int32_t deinit(void) {
    pthread_mutex_lock(&init_lock);
    if (fd <= 0) {
        LOGE("rtc manager already deinit.\n");
        goto error;
    }

    if (fd > 0)
        close(fd);

    pthread_mutex_unlock(&init_lock);

    return 0;

error:
    pthread_mutex_unlock(&init_lock);
    return -1;
}

static int32_t get_rtc(struct rtc_time *time) {
    assert_die_if(time == NULL, "rtc read time , time is null\n");

    memset(time, 0, sizeof(*time));

    if (ioctl(fd, RTC_RD_TIME, time) < 0) {
        LOGE("Failed to read rtc time : ioctl errno %s\n", strerror(errno));
        return -1;
    }

    return 0;
}

static int32_t set_rtc(const struct rtc_time *time) {
    assert_die_if(time == NULL, "rtc write time , time is null\n");

    if (ioctl(fd, RTC_SET_TIME, time) < 0) {
        LOGE("Failed to write rtc time : ioctl errno %s\n", strerror(errno));
        return -1;
    }

    return 0;
}

int32_t get_hibernate_wakeup_status(void) {
    int error = 0;
    int status = 0;

    error = ioctl(fd, JZ_RTC_HIBERNATE_STATUS, &status);
    if (error < 0) {
        LOGE("Failed to get hibernate wakeup status: %s\n", strerror(errno));
        return -1;
    }

    return status;
}

int32_t set_bootup_alarm(struct rtc_time* time) {
    assert_die_if(alarm == NULL, "alarm is null\n");

    struct rtc_wkalrm alarm;
    int error = 0;

    alarm.enabled = 0;
    alarm.pending = 0;
    alarm.time = *time;

    error = ioctl(fd, RTC_WKALM_SET, &alarm);
    if (error < 0) {
        if (errno == ENOTTY)
            LOGE("rtc driver not support alarm IRQ\n");
        LOGE("Failed to set rtc alarm: %s\n", strerror(errno));
        return -1;
    }

#ifdef LOCAL_DEBUG
    struct rtc_wkalrm alarm_time;
    struct rtc_time* tm = &alarm_time.time;

    error = ioctl(fd, RTC_WKALM_RD, &alarm_time);
    if (error < 0) {
        LOGE("Failed to read rtc alarm time\n");
        return -1;
    }

    LOGD("Bootup alarm time set to: %04d-%02d-%02d %02d:%02d:%02d.\n",
            tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min,
            tm->tm_sec);
#endif

    error = ioctl(fd, RTC_AIE_ON, 0);
    if (error < 0) {
        LOGE("Failed to enable alarm IRQ\n");
        return -1;
    }

    return 0;
}

struct rtc_manager rtc_manager = {
    .init = init,
    .deinit = deinit,
    .set_rtc = set_rtc,
    .get_rtc = get_rtc,
    .get_hibernate_wakeup_status = get_hibernate_wakeup_status,
    .set_bootup_alarm = set_bootup_alarm,
};

struct rtc_manager *get_rtc_manager(void) {
    return &rtc_manager;
}
