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

#include "netlink_listener.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <fcntl.h>
#include <errno.h>
#include <poll.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <pthread.h>

#include <utils/log.h>
#include <utils/assert.h>
#include <utils/common.h>
#include <thread/thread.h>
#include <netlink/netlink_handler.h>
#include <netlink/netlink_event.h>

#define LOG_TAG "netlink_listener"

static int local_socket;
static char buffer[64 * 1024];
static struct netlink_handler* head;
static struct thread* thread;
static pthread_mutex_t handler_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t init_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t start_lock = PTHREAD_MUTEX_INITIALIZER;

static uint32_t init_count;
static uint32_t start_count;

static void dispatch_event(struct netlink_event *event) {
    struct netlink_handler *nh, *next_nh;

    nh = head;

    while (nh) {
        next_nh = nh->next;

        nh->handle_event(nh, event);

        nh = next_nh;
    }
}

static void thread_loop(struct pthread_wrapper* pthread, void *param) {
    struct pollfd fds;

    fds.fd = local_socket;
    fds.events = POLLIN;
    fds.revents = 0;

    int count;

    for (;;) {
restart:
        do {
            count = poll(&fds, 1, -1);
        } while (count < 0 && errno == EINTR);

        if (fds.revents & POLLIN) {
            count = recv(local_socket, buffer, sizeof(buffer), 0);
            if (count < 0) {
                LOGE("netlink event recv failed: %s\n", strerror(errno));
                goto restart;
            }

            struct netlink_event* event = _new(struct netlink_event,
                    netlink_event);

            if (event->decode(event, buffer, count, NETLINK_FORMAT_ASCII) < 0) {
                LOGE("Error decoding netlink_event\n");
                _delete(event);
                goto restart;
            }

            dispatch_event(event);

            _delete(event);
        }
    }

    pthread_exit(NULL);
}

static int start(void) {
    pthread_mutex_lock(&start_lock);

    if (start_count++ == 0)
        thread->start(thread, NULL);

    pthread_mutex_unlock(&start_lock);

    return 0;
}

static int is_start(void) {
    pthread_mutex_lock(&start_lock);
    int started = thread->is_running(thread);
    pthread_mutex_unlock(&start_lock);

    return started;
}

static int stop(void) {
    pthread_mutex_lock(&start_lock);

    if (--start_count == 0)
        thread->stop(thread);

    pthread_mutex_unlock(&start_lock);

    return 0;
}

static int init(int socket) {
    pthread_mutex_lock(&init_lock);

    if (init_count++ == 0) {
        local_socket = socket;
        thread = _new(struct thread, thread);
        thread->runnable.run = thread_loop;
    }

    pthread_mutex_unlock(&init_lock);

    return 0;
}

static int deinit(void) {
    pthread_mutex_lock(&init_lock);

    if (--init_count == 0) {
        local_socket = -1;

        if (is_start())
            stop();
        _delete(thread);
    }

    pthread_mutex_unlock(&init_lock);

    return 0;
}

static void register_handler(struct netlink_handler* handler) {
    assert_die_if(handler == NULL, "Handler is NULL\n");

    pthread_mutex_lock(&handler_lock);

    while (head != NULL) {
        if (handler->get_priority(handler) > head->get_priority(handler))
            break;
        head = (head->next);
    }
    handler->next = head;
    head = handler;

    pthread_mutex_unlock(&handler_lock);
}

static void unregister_handler(struct netlink_handler* handler) {
    assert_die_if(handler == NULL, "Handler is NULL\n");

    pthread_mutex_lock(&handler_lock);

    while (head != NULL) {
        if (head == handler) {
            head = handler->next;
            return;
        }
        head = (head->next);
    }

    pthread_mutex_unlock(&handler_lock);
}

static struct netlink_listener this = {
        .init = init,
        .deinit = deinit,
        .start = start,
        .stop = stop,
        .is_start = is_start,
        .register_handler = register_handler,
        .unregister_handler = unregister_handler,
};

struct netlink_listener* get_netlink_listener(void) {
    return &this;
}
