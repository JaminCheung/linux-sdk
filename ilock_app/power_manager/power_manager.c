/*
 *  Copyright (C) 2016, Wang Qiuwei <qiuwei.wang@ingenic.com, panddio@163.com>
 *
 *  Ingenic Linux SDK Project
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
#include <sys/reboot.h>

#include <utils/log.h>
#include <utils/assert.h>
#include <app_power_manager.h>

/*
 * Macros
 */
#define ENTRY_SLEEP_DEV                 "/sys/power/state"
#define LOG_TAG                         "PowerManager"


static int32_t pm_power_off(void) {
    int ret;

    sync();

    ret = reboot(RB_POWER_OFF);
    if (ret < 0) {
        LOGE("Failed to power off: %s\n", strerror(errno));
        return -1;
    }
    return 0;
}

static int32_t pm_reboot(void) {
    int ret;

    sync();

    ret = reboot(RB_AUTOBOOT);
    if (ret < 0) {
        LOGE("Failed to reboot: %s\n", strerror(errno));
        return -1;
    }
    return 0;
}

static int32_t pm_sleep(void) {
    int fd;
    char cmd[] = "mem";

    fd = open(ENTRY_SLEEP_DEV, O_WRONLY);
    if (fd < 0) {
        LOGE("Failed to open %s: %s\n", \
             ENTRY_SLEEP_DEV, strerror(errno));
        return -1;
    }

    if (strlen(cmd) != write(fd, cmd, strlen(cmd))) {
        LOGE("Failed to entry sleep: %s\n", strerror(errno));
        close(fd);
        return -1;
    }

    close(fd);
    return 0;
}


static int32_t pm_wake_lock_init(struct wake_lock *lock, char *name)
{
    return 0;
}

static int32_t pm_wake_lock(struct wake_lock *lock)
{

    return 0;
}

static int32_t pm_wake_lock_timeout(struct wake_lock *lock, int timout)
{
    return 0;
}

static int32_t pm_wake_unlock(struct wake_lock *lock)
{

    return 0;
}



static struct app_power_manager this = {
    .power_off          = pm_power_off,
    .reboot             = pm_reboot,
    .sleep              = pm_sleep,
    .wake_lock_init     = pm_wake_lock_init,
    .wake_lock          = pm_wake_lock,
    .wake_lock_timeout  = pm_wake_lock_timeout,
    .wake_unlock        = pm_wake_unlock,
};

struct app_power_manager *get_app_power_manager(void)
{
    return &this;
}
