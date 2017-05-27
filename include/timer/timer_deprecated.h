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
#include <timer/timer_manager.h>

#define TIMER_WORK_THREAD_POLICY                      SCHED_RR
#define TIMER_WORK_THREAD_PRIORITY                   1
/* 系统支持的最大定时器个数 */
#define TIMER_DEFAULT_MAX_CNT                             1
/* 系统支持的最大定时器线程数 */
#define TIMER_DEFAULT_WORK_THREAD_MAX_CNT     1
/* 系统定时器定时周期，该项为定时精度，单位是ms */
#define TIMER_SYS_DEFAULT_INTERVAL_MS               1
/* 系统定时器定时启动等待时间，单位是ms */
#define TIMER_SYS_DEFAULT_START_MS                    1

typedef void (*timer_handle)(void *arg);
struct timer_info {
    uint8_t is_init;
    uint8_t is_enable;
    uint32_t interval;
    uint32_t elapse;
    void *arg;
    timer_handle routine;
    pthread_mutex_t res_lock;
};

struct timer_usr {
    struct itimerspec value, ovalue;
    struct timer_info timer_info[TIMER_DEFAULT_MAX_CNT];
    timer_t sys_timer_id;
    struct thread_pool_manager* thread_pool;
    uint8_t work_thread_cnt;
    uint32_t init_called_times;
    uint32_t interval_ms;
    uint32_t start_ms;
    pthread_mutex_t mutex_lock;
};
#endif
