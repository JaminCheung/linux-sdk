/*
 *  Copyright (C) 2017, Zhang YanMing <yanmin.zhang@ingenic.com, jamincheung@126.com>
 *
 *  Ingenic Linux plarform SDK project
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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>
#include <pthread.h>
#include <linux/netlink.h>

#include <types.h>
#include <utils/log.h>
#include <utils/assert.h>
#include <netlink/netlink_handler.h>
#include <netlink/netlink_manager.h>

#include "netlink_listener.h"

#define LOG_TAG "netlink_manager"

static struct netlink_listener *nl;
static int local_socket;

static pthread_mutex_t init_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t start_lock = PTHREAD_MUTEX_INITIALIZER;

static uint32_t init_count;
static uint32_t start_count;

static int start(void) {
    int error = 0;

    pthread_mutex_lock(&start_lock);

    if (start_count++ == 0) {
        error = nl->start();
        if (error) {
            LOGE("Failed to start netlink_listener: %s\n", strerror(errno));
            goto error;
        }
    }

    pthread_mutex_unlock(&start_lock);
    return 0;

error:
    start_count = 0;
    pthread_mutex_unlock(&start_lock);
    return -1;
}

static int stop(void) {
    int error = 0;

    pthread_mutex_lock(&start_lock);

    if (--start_count == 0) {
        error = nl->stop();
        if (error) {
            LOGE("Failed to stop netlink_listener: %s\n", strerror(errno));
            goto error;
        }

        close(local_socket);
        local_socket = -1;
        nl = NULL;
    }

    pthread_mutex_unlock(&start_lock);

    return 0;

error:
    start_count = 1;
    pthread_mutex_unlock(&start_lock);
    return -1;
}

static int is_start(void) {
    pthread_mutex_lock(&start_lock);
    int started = nl->is_start();
    pthread_mutex_unlock(&start_lock);

    return started;

}

static int init(void) {
    struct sockaddr_nl nladdr;
    int size = 64 * 1024;

    pthread_mutex_lock(&init_lock);

    if (init_count++ == 0) {
        memset(&nladdr, 0, sizeof(nladdr));
        nladdr.nl_family = AF_NETLINK;
        nladdr.nl_pid = (pthread_self() << 16) | getpid();
        nladdr.nl_groups = 0xffffffff;

        local_socket = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_KOBJECT_UEVENT);
        if (local_socket < 0) {
            LOGE("Unable to create uevent local_socket: %s\n", strerror(errno));
            goto error;
        }

        if (setsockopt(local_socket, SOL_SOCKET, SO_RCVBUFFORCE, &size,
                sizeof(size)) < 0) {
            LOGE("Unable to set uevent local_socket options: %s\n", strerror(errno));
            goto error;
        }

        if (bind(local_socket, (struct sockaddr *)&nladdr, sizeof(nladdr)) < 0) {
            LOGE("Unable to bind uevent local_socket: %s\n", strerror(errno));
            goto error;
        }

        nl = get_netlink_listener();
        if (nl->init(local_socket)) {
            LOGE("Failed to init netlink listener\n");
            goto error;
        }
    }

    pthread_mutex_unlock(&init_lock);

    return 0;

error:
    init_count = 0;
    pthread_mutex_unlock(&init_lock);
    return -1;
}

static int deinit(void) {
    pthread_mutex_lock(&init_lock);

    if (init_count-- == 0) {
        if (is_start())
            stop();
    }

    pthread_mutex_unlock(&init_lock);

    return 0;
}

static void register_handler(struct netlink_handler* handler) {
    assert_die_if(handler == NULL, "Handler is NULL\n");

    nl->register_handler(handler);
}

static void unregister_handler(struct netlink_handler* handler) {
    assert_die_if(handler == NULL, "Handler is NULL\n");

    nl->unregister_handler(handler);
}

static struct netlink_manager this = {
        .init = init,
        .deinit = deinit,
        .start = start,
        .is_start = is_start,
        .stop = stop,
        .register_handler = register_handler,
        .unregister_handler = unregister_handler,
};

struct netlink_manager* get_netlink_manager(void) {
    return &this;
}
