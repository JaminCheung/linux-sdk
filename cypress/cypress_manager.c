/*
 *  Copyright (C) 2016, Wang Qiuwei <qiuwei.wang@ingenic.com, panddio@163.com>
 *
 *  Ingenic SDK Project
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
#include <signal.h>

#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <linux/input.h>

#include <types.h>
#include <utils/log.h>
#include <utils/assert.h>
#include <cypress/cypress_manager.h>



#define LOG_TAG                  "cypress"
#define CYPRESS_DEFAULT_DEV      "/dev/cypress_psoc4"


struct cypress_dev {
    int dev_fd;
    bool is_init;
    struct sigaction sigact;
    deal_card_report_handler card_report_handler;
};
static struct cypress_dev cypress;


/**
 * Functions
 */
static void cypress_mcu_reset(void) {
    if (!cypress.is_init) {
        LOGE("ERROR: 74HC595 is not initialized\n");
        return;
    }

    ioctl(cypress.dev_fd, CYPRESS_DRV_IOC_RESET_MCU, NULL);
}

static void cypress_sync_handler(int signo) {
    if (signo == SIGIO &&
        cypress.card_report_handler) {
        cypress.card_report_handler(cypress.dev_fd);
    }
}

static int32_t cypress_init(deal_card_report_handler card_handler) {
    if (cypress.is_init) {
        LOGE("Cypress dev has been initialized\n");
        return -EPERM;
    }

    cypress.dev_fd = open(CYPRESS_DEFAULT_DEV, O_RDWR);
    if (cypress.dev_fd < 0) {
        LOGE("Failed to open %s: %s\n", CYPRESS_DEFAULT_DEV, strerror(errno));
        return -1;
    }

    bzero(&cypress.sigact, sizeof(struct sigaction));
    cypress.sigact.sa_handler = cypress_sync_handler;
    cypress.sigact.sa_flags = 0;
    sigaction(SIGIO, &cypress.sigact, NULL);

    fcntl(cypress.dev_fd, F_SETOWN, getpid());
    fcntl(cypress.dev_fd, F_SETFL, fcntl(cypress.dev_fd, F_GETFL) | FASYNC);

    cypress.card_report_handler = card_handler;
    cypress.is_init = true;

    return 0;
}

static void cypress_deinit(void) {
    if (cypress.is_init) {
        close(cypress.dev_fd);
        cypress.card_report_handler = NULL;
        cypress.is_init = false;
    }
}

struct cypress_manager cypress_manager = {
    .init      = cypress_init,
    .deinit    = cypress_deinit,
    .mcu_reset = cypress_mcu_reset,
};

struct cypress_manager *get_cypress_manager(void) {
    return &cypress_manager;
}
