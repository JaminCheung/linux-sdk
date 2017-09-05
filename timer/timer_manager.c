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

#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <string.h>

#include <types.h>
#include <utils/log.h>
#include <utils/common.h>
#include <utils/list.h>
#include <utils/assert.h>
#include <thread/thread.h>
#include <timer/timer_manager.h>

#define LOG_TAG "timer_manager"

struct timer_info {
    int id;
    int is_start;
    int init_exp;
    int interval;
    int timer_fd;
    int oneshot;
    struct itimerspec now;
    struct itimerspec old;
    timer_event_listener_t callback;
    struct list_head node;
};

static int free_timer(int timer_id);

static uint32_t init_count;
static pthread_mutex_t init_lock = PTHREAD_MUTEX_INITIALIZER;
static struct thread* thread;
static pthread_mutex_t list_lock = PTHREAD_MUTEX_INITIALIZER;
static LIST_HEAD(timer_list);
static int epoll_fd;

static struct timer_info* find_timer_info_by_id(int id) {
    struct timer_info* info = NULL;

    pthread_mutex_lock(&list_lock);

    list_for_each_entry(info, &timer_list, node)
        if (info->id == id)
            break;

    pthread_mutex_unlock(&list_lock);

    if (info == NULL)
        LOGE("Failed to find timer_info by id: %d\n", id);

    return info;
}

static int epoll_add_event(int id) {
    int error = 0;

    struct epoll_event event;
    struct timer_info* info = find_timer_info_by_id(id);
    if (info == NULL)
        return -1;

    memset(&event, 0, sizeof(struct epoll_event));

    event.events = EPOLLIN | EPOLLET;
    event.data.ptr = info;

    error = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, info->timer_fd, &event);
    if (error < 0) {
        LOGE("Failed to add epoll: %s\n", strerror(errno));
        return -1;
    }

    return 0;
}

static int epoll_del_event(int id) {
    int error = 0;

    struct timer_info* info = find_timer_info_by_id(id);
    if (info == NULL)
        return -1;

    error = epoll_ctl(epoll_fd, EPOLL_CTL_DEL, info->timer_fd, NULL);
    if (error < 0) {
        LOGE("Failed to del epoll: %s\n", strerror(errno));
        return -1;
    }

    return 0;
}

static void thread_loop(struct pthread_wrapper* pthread, void* param) {
    struct epoll_event events[TIMER_MAX_COUNT];
    int count;

    for (;;) {
        do {
            count = epoll_wait(epoll_fd, events, ARRAY_SIZE(events), -1);
        } while (count <= 0 && errno == EINTR);

        for (int i = 0; i < count; i++) {
            struct timer_info* info = events[i].data.ptr;

            if (events[i].events & EPOLLIN) {
                uint64_t exp;

                read(info->timer_fd, &exp, sizeof(uint64_t));

                info->callback(info->id, exp);

                if (info->oneshot)
                    free_timer(info->id);
            }
        }
    }

    pthread_exit(NULL);
}


static int start(int id) {
    int error = 0;

    struct timer_info* info = find_timer_info_by_id(id);
    if (info == NULL)
        return -1;

    if (info->is_start) {
        LOGE("Timer(%d) already start\n", info->id);
        return -1;
    }

    error = epoll_add_event(id);
    if (error < 0)
        return -1;

    uint64_t interval_sec = info->interval / 1000;
    uint64_t interval_nsec = (info->interval % 1000) * 1000000;
    uint64_t init_sec = info->init_exp / 1000;
    uint64_t init_nsec = (info->init_exp % 1000) * 1000000;

    info->now.it_value.tv_sec = init_sec;
    info->now.it_value.tv_nsec = init_nsec;

    info->now.it_interval.tv_sec = info->oneshot ? 0 : interval_sec;
    info->now.it_interval.tv_nsec = info->oneshot ? 0 : interval_nsec;

    pthread_mutex_lock(&list_lock);

    if (get_list_size_locked(&timer_list) == 1)
        thread->start(thread, NULL);

    pthread_mutex_unlock(&list_lock);

    error = timerfd_settime(info->timer_fd, 0, &info->now, &info->old);
    if (error < 0) {
        LOGE("Failed to set time: %s\n", strerror(errno));
        goto error;
    }

    info->is_start = 1;

    return 0;

error:
    epoll_del_event(id);
    if (get_list_size_locked(&timer_list) == 1)
        thread->stop(thread);

    return -1;
}

static int stop(int id) {
    int error = 0;

    struct timer_info* info = find_timer_info_by_id(id);
    if (info == NULL)
        return -1;

    if (!info->is_start) {
        LOGE("Timer(%d) already stop\n", info->id);
        return -1;
    }

    error = timerfd_settime(info->timer_fd, 0, &info->old, &info->now);
    if (error < 0) {
        LOGE("Failed to set time: %s\n", strerror(errno));
        return -1;
    }

    error = epoll_del_event(id);
    if (error < 0)
        return -1;

    info->is_start = 0;

    pthread_mutex_lock(&list_lock);

    if (get_list_size_locked(&timer_list) == 1)
        thread->stop(thread);

    pthread_mutex_unlock(&list_lock);

    return 0;
}

static int is_start(int id) {
    struct timer_info* info = find_timer_info_by_id(id);
    if (info == NULL)
        return -1;

    return info->is_start;
}

static int alloc_timer(int init_exp, int interval, int oneshot,
        timer_event_listener_t listener) {
    assert_die_if(init_exp < 0, "Invaild initial expiration\n");
    assert_die_if(interval < 0, "Invaild interval\n");
    assert_die_if(oneshot < 0, "Invaild oneshot\n");
    assert_die_if(listener == NULL, "listener is NULL\n");

    struct timer_info* info = calloc(1, sizeof(struct timer_info));
    if (info == NULL) {
        LOGE("Failed to allocate memory\n");
        return -1;
    }

    info->timer_fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
    if (info->timer_fd < 0) {
        LOGE("Failed to create timerfd: %s\n", strerror(errno));
        return -1;
    }

    info->id = (int) info & 0xffff;
    info->init_exp = init_exp;
    info->interval = interval;
    info->oneshot = oneshot;
    info->callback = listener;

    pthread_mutex_lock(&list_lock);

    list_add_tail(&info->node, &timer_list);

    pthread_mutex_unlock(&list_lock);

    return info->id;
}

static uint64_t remain_ms(int id) {
    int error = 0;
    struct itimerspec now;

    struct timer_info* info = find_timer_info_by_id(id);
    if (info == NULL)
        return -1;

    if (!info->is_start) {
        LOGE("Timer(%d) already stop\n", info->id);
        return -1;
    }

    error = timerfd_gettime(info->timer_fd, &now);
    if (error < 0) {
        LOGE("Failed to get time: %s\n", strerror(errno));
        return -1;
    }

    return (now.it_value.tv_sec * 1000) + (now.it_value.tv_nsec / 1000000);
}

static int free_timer(int timer_id) {
    struct timer_info* info = NULL, *ninfo = NULL;

    pthread_mutex_lock(&list_lock);

    list_for_each_entry_safe(info, ninfo, &timer_list, node) {
        if (timer_id == info->id) {


            if (info->is_start) {
                pthread_mutex_unlock(&list_lock);

                stop(info->id);

                pthread_mutex_lock(&list_lock);
            }

            close(info->timer_fd);
            info->timer_fd = -1;

            list_del(&info->node);

            free(info);
            info = NULL;

            break;
        }
    }

    pthread_mutex_unlock(&list_lock);

    return 0;
}

static void free_all_timer(void) {
    struct timer_info* info = NULL, *ninfo = NULL;

    pthread_mutex_lock(&list_lock);

    list_for_each_entry_safe(info, ninfo, &timer_list, node) {

        if (info->is_start) {
            pthread_mutex_unlock(&list_lock);

            stop(info->id);

            pthread_mutex_lock(&list_lock);
        }

        close(info->timer_fd);
        info->timer_fd = -1;

        list_del(&info->node);

        free(info);
        info = NULL;
    }

    pthread_mutex_unlock(&list_lock);
}

static int enumerate(int* id_table) {
    assert_die_if(id_table == NULL, "ID table is NULL\n");

    int i = 0;
    struct timer_info* info = NULL;

    pthread_mutex_lock(&list_lock);

    list_for_each_entry(info, &timer_list, node)
        id_table[i++] = info->id;

    pthread_mutex_unlock(&list_lock);

    return i;
}

static int init(void) {
    pthread_mutex_lock(&init_lock);

    if (init_count++ == 0) {
        epoll_fd = epoll_create(TIMER_MAX_COUNT);
        if (epoll_fd < 0) {
            LOGE("Failed to create epoll: %s\n", strerror(errno));
            goto error;
        }

        thread = _new(struct thread, thread);
        thread->runnable.run = thread_loop;
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

    if (--init_count == 0) {
        free_all_timer();

        if (thread->is_running(thread))
            thread->stop(thread);

        _delete(thread);

        close(epoll_fd);
        epoll_fd = -1;
    }

    pthread_mutex_unlock(&init_lock);

    return 0;
}

static int is_init(void) {
    pthread_mutex_lock(&init_lock);
    int inited = init_count != 0;
    pthread_mutex_unlock(&init_lock);

    return inited;
}

struct timer_manager this = {
        .init = init,
        .deinit = deinit,
        .is_init = is_init,
        .alloc_timer = alloc_timer,
        .free_timer = free_timer,
        .enumerate = enumerate,
        .remain_ms = remain_ms,
        .start = start,
        .stop = stop,
        .is_start = is_start,
};

struct timer_manager* get_timer_manager(void) {
    return &this;
}
