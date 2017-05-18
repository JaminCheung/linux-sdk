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

#ifndef TIMER_MANAGER_H
#define TIMER_MANAGER_H

#include <utils/thread_pool.h>
#include <sys/time.h>
#include <ingenic_api.h>

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
#endif