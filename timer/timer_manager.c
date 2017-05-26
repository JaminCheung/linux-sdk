/*
 *  Copyright (C) 2016, Zhang YanMing <yanmin.zhang@ingenic.com, jamincheung@126.com>
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
#include <time.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <utime.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <types.h>
#include <signal.h>
#include <pthread.h>
#include <utils/log.h>
#include <utils/assert.h>
#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <utils/thread_pool.h>
#include <timer/timer_manager.h>
#include <utils/thread_pool.h>
#include <sys/time.h>

/* The scheduled policy used for epoll thread */
#define TIMER_WORK_THREAD_POLICY                      SCHED_RR
/* The scheduled policy priority used for epoll thread */
#define TIMER_WORK_THREAD_PRIORITY                   1
/* The reserved timer info struct used for epoll thread exit */
#define TIMER_SYS_RESERVED_CNT                          1

typedef void (*timer_handle)(void *arg);
struct timer_info {
    uint8_t is_init;
    uint8_t is_enable;
    uint32_t interval;
    // uint32_t elapse;
    int32_t fd;
    void *arg;
    uint8_t is_one_time;
    struct itimerspec now, old;
    timer_handle routine;
    pthread_mutex_t res_lock;
};

struct timer_usr {
    struct itimerspec value, ovalue;
    struct timer_info timer_info[TIMER_DEFAULT_MAX_CNT + TIMER_SYS_RESERVED_CNT];
    pthread_t epool_tid;
    int32_t epoll_fd;
    int pipe[2];
    uint32_t sys_reserved_cnt;
    uint32_t max_timers_support;
    uint32_t timer_cnt;
    uint32_t interval_ms;
    uint32_t start_ms;
    pthread_mutex_t mutex_lock;
    struct itimerspec zero;
};

#define LOG_TAG "timer_manager"

static struct timer_usr timer_usr;


static void* epoll_proc(void *arg) {
    struct timer_usr *timer = (struct timer_usr *)arg;
    int max_listen_fd = timer->max_timers_support  + timer->sys_reserved_cnt;
    struct epoll_event *events = calloc(max_listen_fd, sizeof(*events));
    if (events == NULL) {
        LOGE("Cannot alloc more memory for events\n");
        goto out;
    }
    while(1) {
        int nfds =epoll_wait(timer->epoll_fd, events, max_listen_fd, -1);
        for(int i=0; i < nfds; ++i) {
            struct timer_info *info = events[i].data.ptr;
            pthread_mutex_lock(&info->res_lock);
            if ((info->fd == timer->pipe[0])
                && (events[i].events & EPOLLIN))  {
                char c;
                if (!read(timer->pipe[0], &c, 1)) {
                    LOGE("Unable to read pipe: %s\n", strerror(errno));
                    continue;
                }
                LOGI("thread 0x%x is exit\n", (unsigned int)pthread_self());
                pthread_mutex_unlock(&info->res_lock);
                pthread_exit(NULL);
                goto out;
            }else if (events[i].events & EPOLLIN) {
                uint64_t buf;
                read(info->fd, &buf, sizeof(buf));
                pthread_mutex_unlock(&info->res_lock);
                (*info->routine)(info->arg);
            }
        }
    }
out:
    pthread_exit(NULL);
}

static int32_t epoll_add_event(int32_t id) {
    struct epoll_event epe;
    struct timer_info *timer_info = &timer_usr.timer_info[id];
    epe.data.ptr = timer_info;
    epe.events = EPOLLIN | EPOLLET;
    int ret = epoll_ctl(timer_usr.epoll_fd,
            EPOLL_CTL_ADD, timer_info->fd, &epe);
    if(ret == -1) {
        LOGE("Failed to call epoll_add_event: %s\n", strerror(errno));
        return -1;
    }
    return 0;
}

static int32_t epoll_remove_event(int32_t id) {
    struct timer_info *timer_info = &timer_usr.timer_info[id];
    int ret = epoll_ctl(timer_usr.epoll_fd,
            EPOLL_CTL_DEL, timer_info->fd,0);
    if(ret == -1) {
        LOGE("Failed to call epoll_remove_event: %s\n", strerror(errno));
        return -1;
    }
    return 0;
}

static int32_t timer_low_layer_init(struct timer_usr *timer) {
    int ret;

    timer->max_timers_support = TIMER_DEFAULT_MAX_CNT;
    timer->zero.it_value.tv_nsec = 0;
    timer->zero.it_value.tv_sec = 0;
    timer->zero.it_interval.tv_nsec = 0;
    timer->zero.it_interval.tv_sec = 0;

    pthread_mutex_init(&timer->mutex_lock, NULL);
    for (int i = 0; i < timer->max_timers_support; i++) {
        pthread_mutex_init(&timer->timer_info[i].res_lock, NULL);
    }

    if (pipe(timer->pipe)) {
        LOGE("Unable to open pipe: %s\n", strerror(errno));
        goto out;
    }
    struct timer_info * timer_info = &(timer->timer_info[0]);
    timer_info->fd = timer->pipe[0];
    timer->sys_reserved_cnt = 1;
    timer->epoll_fd = epoll_create(timer->max_timers_support);
    if(timer->epoll_fd == -1) {
        LOGE("Failed to call epoll_create: %s\n", strerror(errno));
        goto out;
    }

    if (epoll_add_event(0) < 0){
        LOGE("Cannot call epoll add event on timer%d", 0);
        goto out;
    }

    pthread_attr_t thread_attr;
    pthread_attr_init(&thread_attr);
    int32_t policy = TIMER_WORK_THREAD_POLICY;
    int32_t priority = TIMER_WORK_THREAD_PRIORITY;
    if (priority > 0) {
        if (thread_set_priority(&thread_attr, policy, priority) < 0) {
            pthread_attr_destroy(&thread_attr);
            goto out;
        }
    }

    ret = pthread_create(&timer->epool_tid,
                &thread_attr, epoll_proc, timer);
    if(ret == -1) {
        LOGE("Failed to call pthread_create: %s\n", strerror(errno));
        goto out_event;
    }
    return 0;

out_event:
    if (epoll_remove_event(0) < 0){
        LOGE("Cannot call epoll remove event on timer%d", 0);
    }
out:
    if (timer->epoll_fd > 0) {
        close(timer->epoll_fd);
        timer->epoll_fd = 0;
    }
    if (timer->pipe[0] && timer->pipe[1]) {
        close(timer->pipe[0]);
        close(timer->pipe[1]);
        timer->pipe[0] = 0;
        timer->pipe[1] = 0;
    }
    return -1;
}

static int32_t timer_low_layer_destroy(struct timer_usr *timer) {
    uint8_t c = 0;
    if (!write(timer->pipe[1], &c, 1)) {
        LOGE("Unable to write pipe: %s\n", strerror(errno));
        return -1;
    }
    if (pthread_join (timer->epool_tid, NULL) < 0) {
        LOGE("Thread 0x%x is failed while waiting\n", (unsigned int)timer->epool_tid);
        return -1;
    }
    timer->epool_tid = 0;
    if (timer->epoll_fd > 0) {
        close(timer->epoll_fd);
        timer->epoll_fd = 0;
    }
    if (timer->pipe[0] && timer->pipe[1]) {
        close(timer->pipe[0]);
        close(timer->pipe[1]);
        timer->pipe[0] = 0;
        timer->pipe[1] = 0;
    }
    for (int i = 0; i < timer->max_timers_support; i++) {
        pthread_mutex_destroy(&timer->timer_info[i].res_lock);
        memset(&timer->timer_info[i], 0, sizeof(timer->timer_info[i]));
    }
    timer->sys_reserved_cnt = 0;
    timer->max_timers_support = 0;
    pthread_mutex_destroy(&timer->mutex_lock);
    return 0;
}

static int32_t timer_init(int32_t id, uint32_t interval, uint8_t is_one_time,
    timer_handle routine, void *arg) {
    struct timer_info *timer_info = NULL;
    int assigned = -1;

    if (!timer_usr.timer_cnt) {
        if (timer_low_layer_init(&timer_usr) < 0) {
            LOGE("Timer low layer init failed\n");
            return -1;
        }
    }
    if (id >= timer_usr.sys_reserved_cnt) {
        assert_die_if((id > timer_usr.max_timers_support),
            "Parameter id is overlow, mustn\'t be greater than %d\n",
            timer_usr.max_timers_support);

        if (timer_usr.timer_info[id].is_init) {
            LOGE("Timer%d is already in use, please swtich timer\n", id);
            goto out;
        }
        timer_info = &timer_usr.timer_info[id];
        assigned = id;
    } else {
        for(int i = timer_usr.sys_reserved_cnt;
                i < timer_usr.sys_reserved_cnt + timer_usr.max_timers_support;
                i++) {
            if(timer_usr.timer_info[i].is_init)
                continue;
            timer_info = &timer_usr.timer_info[i];
            assigned = i;
            break;
        }
        if(assigned == -1) {
            LOGE("Cannot find available timer resource\n");
            goto out;
        }
    }

    int32_t fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
    if(fd == -1) {
        LOGE("Falied to call timerfd_create: %s\n", strerror(errno));
        goto out;
    }
    pthread_mutex_lock(&timer_info->res_lock);
    timer_info->is_init = true;
    timer_info->fd = fd;
    timer_info->routine = routine;
    timer_info->arg = arg;
    timer_info->interval = interval;
    timer_info->is_one_time = is_one_time;
    timer_usr.timer_cnt++;
    pthread_mutex_unlock(&timer_info->res_lock);

    return assigned;
out:
    timer_low_layer_destroy(&timer_usr);
    return -1;
}

static int32_t timer_deinit(uint32_t id) {
    assert_die_if((id < timer_usr.sys_reserved_cnt)
                || (id >= (timer_usr.sys_reserved_cnt + timer_usr.max_timers_support)),
        "%s: %d Parameter id is overlow, range from %d to %d\n",
        __FILE__, __LINE__,
        timer_usr.sys_reserved_cnt,
        timer_usr.sys_reserved_cnt + timer_usr.max_timers_support - 1);
    struct timer_info *timer_info = &timer_usr.timer_info[id];
    pthread_mutex_lock(&timer_info->res_lock);
    if (!timer_info->is_init) {
        pthread_mutex_unlock(&timer_info->res_lock);
        LOGE("This timer hasn't been initialed! please call timer_init function!");
        return -1;
    }
    if (timer_info->is_enable) {
        pthread_mutex_unlock(&timer_info->res_lock);
        LOGE("Call timer_stop before timer_deinit on timer%d\n", id);
        return -1;
    }
    timer_usr.timer_cnt--;
    timer_info->is_init = false;
    if (timer_info->fd) {
        close(timer_info->fd);
        timer_info->fd = 0;
    }
    pthread_mutex_unlock(&timer_info->res_lock);

    if (!timer_usr.timer_cnt) {
        if (timer_low_layer_destroy(&timer_usr) < 0) {
            LOGE("Failed to call timer_low_layer_destroy\n");
            return -1;
        }
    }
    return 0;
}

static uint64_t remained_time_to_ms(struct itimerspec *spec) {

    uint64_t ms = 0;
    ms = (spec->it_value.tv_sec * 1000) + (spec->it_value.tv_nsec / 1000000);
    return ms;
}

static int32_t timer_start(uint32_t id) {
    assert_die_if((id < timer_usr.sys_reserved_cnt)
                || (id >= (timer_usr.sys_reserved_cnt + timer_usr.max_timers_support)),
        "%s: %d Parameter id is overlow, range from %d to %d\n",
        __FILE__, __LINE__,
        timer_usr.sys_reserved_cnt,
        timer_usr.sys_reserved_cnt + timer_usr.max_timers_support - 1);

    struct timer_info *timer_info = &timer_usr.timer_info[id];
    pthread_mutex_lock(&timer_info->res_lock);
    if (!timer_info->is_init) {
        LOGE("This timer_handle of timer%d hasn't been set!"
                " please call timer_init function!", id);
        goto out_unlock;
    }
    if (timer_info->is_enable) {
        LOGW("This timer%d is already enabled\n", id);
        goto out_unlock;
    }

    if (epoll_add_event(id) < 0){
        LOGE("Cannot call epoll add event on timer%d", id);
        goto out_unlock;
    }
    struct itimerspec timer;
    uint64_t number_ns_in_ms = 1000000;
    uint32_t sec = timer_info->interval / 1000;
    uint64_t nsec = (timer_info->interval % 1000) * number_ns_in_ms;

    timer.it_value.tv_nsec = nsec;
    timer.it_value.tv_sec = sec;
    timer.it_interval.tv_nsec = timer_info->is_one_time ? 0 : nsec;
    timer.it_interval.tv_sec = timer_info->is_one_time ? 0: sec;
    // LOGI("it_value nsec = %llu\n", (uint64_t)timer.it_value.tv_nsec);
    // LOGI("it_value sec = %llu\n", (uint64_t)timer.it_value.tv_sec);
    // LOGI("it_interval nsec = %llu\n", (uint64_t)timer.it_interval.tv_nsec);
    // LOGI("it_interval sec = %llu\n", (uint64_t)timer.it_interval.tv_sec);
    timer_info->now = timer;
    int ret = timerfd_settime(timer_info->fd, 0,
            &timer, &timer_info->old);
    if(ret == -1) {
        LOGE("Failed to call timerfd_settime: %s\n", strerror(errno));
        goto out_event;
    }
    timer_info->is_enable = true;
    pthread_mutex_unlock(&timer_info->res_lock);
    return 0;

out_event:
    epoll_remove_event(id);
out_unlock:
    pthread_mutex_unlock(&timer_info->res_lock);
    return -1;
}

static int32_t timer_stop(uint32_t id) {
    assert_die_if((id < timer_usr.sys_reserved_cnt)
                || (id >= (timer_usr.sys_reserved_cnt + timer_usr.max_timers_support)),
        "%s: %d Parameter id is overlow, range from %d to %d\n",
        __FILE__, __LINE__,
        timer_usr.sys_reserved_cnt,
        timer_usr.sys_reserved_cnt + timer_usr.max_timers_support - 1);

    struct timer_info *time_info = &timer_usr.timer_info[id];
    pthread_mutex_lock(&time_info->res_lock);
    if (!time_info->is_init) {
        pthread_mutex_unlock(&time_info->res_lock);
        LOGE("This timer_handle hasn't been set! please call timer_init function!");
        return -1;
    }
    if (!time_info->is_enable) {
        pthread_mutex_unlock(&time_info->res_lock);
        LOGE("This timer%d is not enabled before\n", id);
        return -1;
    }

    time_info->is_enable = false;

    int ret = timerfd_settime(time_info->fd, 0,
            &time_info->old, &time_info->now);
    if(ret == -1) {
        LOGE("Failed to call timerfd_settime: %s\n", strerror(errno));
        goto out_unlock;
    }
    if (epoll_remove_event(id) < 0){
        LOGE("Cannot call epoll remove event on timer%d", id);
        goto out_unlock;
    }
    pthread_mutex_unlock(&time_info->res_lock);
    return 0;
out_unlock:
    pthread_mutex_unlock(&time_info->res_lock);
    return -1;
}

static int64_t timer_get_counter(uint32_t id) {
    assert_die_if((id < timer_usr.sys_reserved_cnt)
                || (id >= (timer_usr.sys_reserved_cnt + timer_usr.max_timers_support)),
        "%s: %d Parameter id is overlow, range from %d to %d\n",
        __FILE__, __LINE__,
        timer_usr.sys_reserved_cnt,
        timer_usr.sys_reserved_cnt + timer_usr.max_timers_support - 1);

    struct timer_info *time_info = &timer_usr.timer_info[id];
    pthread_mutex_lock(&time_info->res_lock);
    if (!time_info->is_init) {
        pthread_mutex_unlock(&time_info->res_lock);
        LOGE("This timer_handle hasn't been set! please call timer_init function!");
        return -1;
    }
    if (!time_info->is_enable) {
        pthread_mutex_unlock(&time_info->res_lock);
        LOGE("This timer%d is not enabled before\n", id);
        return -1;
    }
    struct itimerspec curr_value;
    if (timerfd_gettime(time_info->fd, &curr_value) < 0) {
        LOGE("Falied to call timerfd_gettime\n");
        return -1;
    }
    pthread_mutex_unlock(&time_info->res_lock);

    uint64_t ms = remained_time_to_ms(&curr_value);
    return (int64_t)ms;
}

static struct timer_manager timer_manager = {
    .init = timer_init,
    .deinit = timer_deinit,
    .start = timer_start,
    .stop = timer_stop,
    .get_counter = timer_get_counter,
};

struct timer_manager*  get_timer_manager(void) {
    return &timer_manager;
}
