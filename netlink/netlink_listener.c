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
#include <netlink/netlink_listener.h>
#include <netlink/netlink_handler.h>
#include <netlink/netlink_event.h>

#define LOG_TAG "netlink_listener"

static int local_socket;
static int local_pipe[2];
static char buffer[64 * 1024];
static struct netlink_handler* head;

static pthread_mutex_t handler_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t init_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t start_lock = PTHREAD_MUTEX_INITIALIZER;

static int inited;
static int started;

static void *thread_loop(void *param);

static void dispatch_event(struct netlink_event *event) {
    struct netlink_handler *nh, *next_nh;

    nh = head;

    while (nh) {
        next_nh = nh->next;

        nh->handle_event(nh, event);

        nh = next_nh;
    }
}

static void *thread_loop(void *param) {
    struct pollfd fds[2];

    fds[0].fd = local_socket;
    fds[0].events = POLLIN;
    fds[0].revents = 0;

    fds[1].fd = local_pipe[0];
    fds[1].events = POLLIN;
    fds[1].revents = 0;

    int count;
    int error;

    for (;;) {
restart:
        do {
            count = poll(fds, 2, -1);
        } while (count < 0 && errno == EINTR);

        if (fds[0].revents & POLLIN) {
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

        if (fds[1].revents & POLLIN) {
            fds[1].revents = 0;
            char c;
            error = read(fds[1].fd, &c, 1);
            if (!error) {
                LOGE("Unable to read pipe: %s\n", strerror(errno));
                continue;
            }

            LOGI("main thread call me break out\n");
            break;
        }
    }

    return NULL;
}

static int init(int socket) {
    pthread_mutex_lock(&init_lock);
    if (inited == 1) {
        LOGE("netlink handler already init\n");
        pthread_mutex_unlock(&init_lock);
        return -1;
    }

    local_socket = socket;
    inited = 1;
    pthread_mutex_unlock(&init_lock);

    return 0;
}

static int deinit(void) {
    pthread_mutex_lock(&init_lock);

    if (inited == 0) {
        LOGE("netlink handler already deinit\n");
        pthread_mutex_unlock(&init_lock);
        return -1;
    }

    inited = 0;

    if (local_pipe[0] > 0)
        close(local_pipe[0]);

    if (local_pipe[1] > 0)
        close(local_pipe[1]);

    pthread_mutex_unlock(&init_lock);

    return 0;
}

static int start_listener(void) {
    int error = 0;

    pthread_mutex_lock(&start_lock);
    if (started == 1) {
        LOGE("netlink listener already start listener\n");
        pthread_mutex_unlock(&start_lock);
        return -1;
    }

    error = pipe(local_pipe);
    if (error) {
        LOGE("Unable to open pipe: %s\n", strerror(errno));
        pthread_mutex_unlock(&start_lock);
        return -1;
    }

    pthread_t tid;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    error = pthread_create(&tid, &attr, thread_loop, NULL);
    if (error) {
        LOGE("pthread_create failed: %s\n", strerror(errno));
        pthread_attr_destroy(&attr);

        pthread_mutex_unlock(&start_lock);
        return -1;
    }

    pthread_attr_destroy(&attr);

    started = 1;

    pthread_mutex_unlock(&start_lock);

    return 0;
}

static int stop_listener(void) {
    char c = 0;

    pthread_mutex_lock(&start_lock);
    if (started == 0) {
        LOGE("netlink listener already stop listener\n");
        pthread_mutex_unlock(&start_lock);
        return -1;
    }

    if (!write(local_pipe[1], &c, 1)) {
        LOGE("Unable to write pipe: %s\n", strerror(errno));
        pthread_mutex_unlock(&start_lock);
        return -1;
    }

    started = 0;

    pthread_mutex_unlock(&start_lock);

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
        .start_listener = start_listener,
        .stop_listener = stop_listener,
        .register_handler = register_handler,
        .unregister_handler = unregister_handler,
};

struct netlink_listener* get_netlink_listener(void) {
    return &this;
}
