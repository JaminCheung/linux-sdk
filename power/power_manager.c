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

#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <linux/watchdog.h>

#include <utils/log.h>
#include <utils/assert.h>
#include <power/power_manager.h>

/*
 * Macros
 */
#define POWER_OFF_CMD     "echo o > /proc/sysrq-trigger"
#define REBOOT_SYS_CMD    "echo b > /proc/sysrq-trigger"
#define ENTRY_SLEEP_CMD   "echo mem > /sys/power/state"
#define LOG_TAG           "power"


static int pm_power_off(void) {
    pid_t ret;

    ret = system(POWER_OFF_CMD);

    if ((-1 != ret) && \
        (WIFEXITED(ret)) && \
        (WEXITSTATUS(ret) == 0))
        return 0;
    else {
        LOGE("Failed to power off: %s\n", strerror(errno));
        return -1;
    }
}

static int pm_reboot(void) {
    pid_t ret;

    ret = system(REBOOT_SYS_CMD);

    if ((-1 != ret) && \
        (WIFEXITED(ret)) && \
        (WEXITSTATUS(ret) == 0))
        return 0;
    else {
        LOGE("Failed to reboot: %s\n", strerror(errno));
        return -1;
    }
}

static int pm_sleep(void) {
    pid_t ret;

    ret = system(ENTRY_SLEEP_CMD);

    if ((-1 != ret) && \
        (WIFEXITED(ret)) && \
        (WEXITSTATUS(ret) == 0))
        return 0;
    else {
        LOGE("Failed to entry sleep: %s\n", strerror(errno));
        return -1;
    }
}

struct power_manager power_manager = {
    .power_off = pm_power_off,
    .reboot    = pm_reboot,
    .sleep     = pm_sleep,
};

struct power_manager *get_power_manager(void) {
    return &power_manager;
}
