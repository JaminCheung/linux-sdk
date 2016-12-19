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
#include <watchdog/watchdog_manager.h>


/*
 * Macros
 */
#define DEFAULT_WDT_DEV    "/dev/watchdog"
#define LOG_TAG            "wdt"


static int wdt_fd = -1;

/*
 * Functions
 */
static int watchdog_enable(void) {
    int on = WDIOS_ENABLECARD;

    assert_die_if(wdt_fd < 0, "The watchdog is not initialized!\n");
    if (ioctl(wdt_fd, WDIOC_SETOPTIONS, &on) < 0) {
        LOGE("Failed to enable watchdog: %s\n", strerror(errno));
        return -1;
    }

    return 0;
}

static int watchdog_disable(void) {
    int off = WDIOS_DISABLECARD;

    assert_die_if(wdt_fd < 0, "The watchdog is not initialized!\n");
    if (ioctl(wdt_fd, WDIOC_SETOPTIONS, &off) < 0) {
        LOGE("Failed to disable the watchdog: %s\n", strerror(errno));
        return -1;
    }

    return 0;
}

static int watchdog_reset(void) {
    assert_die_if(wdt_fd < 0, "The watchdog is not initialized!\n");
    if (ioctl(wdt_fd, WDIOC_KEEPALIVE, NULL) < 0) {
        LOGE("Failed to reset watchdog: %s\n", strerror(errno));
        return -1;
    }

    return 0;
}

static int watchdog_init(unsigned int timeout) {
    assert_die_if(timeout <= 0, "timeout = %d is invalid!\n", timeout);

    if (wdt_fd < 0) {
        wdt_fd = open(DEFAULT_WDT_DEV, O_RDWR);
        assert_die_if(wdt_fd < 0, "Open %s failed: %s\n", \
                      DEFAULT_WDT_DEV, strerror(errno));
    }

    if (ioctl(wdt_fd, WDIOC_SETTIMEOUT, &timeout) < 0) {
        LOGE("Failed to init watchdog: %s\n", strerror(errno));
        return -1;
    }

    return 0;
}

static void watchdog_deinit(void) {
    watchdog_disable();
    close(wdt_fd);
    wdt_fd = -1;
}

struct watchdog_manager watchdog_manager = {
    .init    = watchdog_init,
    .deinit  = watchdog_deinit,
    .reset   = watchdog_reset,
    .enable  = watchdog_enable,
    .disable = watchdog_disable,
};

struct watchdog_manager *get_watchdog_manager(void) {
    return &watchdog_manager;
}
