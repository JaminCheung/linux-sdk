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
#include <timer/timer_manager.h>

#define LOG_TAG "timer"

static struct timer_usr timer_usr;

static void sys_timer_async_handle(void *arg) {
    uint8_t timerid;

    if (arg == NULL)
        return;
    timerid = *(uint8_t*)arg;
    struct timer_info *info = &timer_usr.timer_info[timerid];

    pthread_mutex_lock(&info->res_lock);
    info->elapse = 0;
    pthread_mutex_unlock(&info->res_lock);
    free(arg);
    // LOGI("id%d , info->elapse = %d\n", timerid, info->elapse);
}

static void sys_timer_handle(union sigval v) {
    // v.sival_int
    for(int i = 0; i < TIMER_DEFAULT_MAX_CNT; i++) {
        struct timer_info *info = &timer_usr.timer_info[i];
        pthread_mutex_lock(&info->res_lock);

        if (!info->is_init || !info->is_enable) {
            pthread_mutex_unlock(&info->res_lock);
            continue;
        }
        if(info->elapse >= info->interval) {
            pthread_mutex_unlock(&info->res_lock);
            continue;
        }
        info->elapse += timer_usr.interval_ms;
        if(info->elapse >= info->interval) {
            uint8_t *id = malloc(sizeof(*id));
            if (id == NULL) {
                pthread_mutex_unlock(&info->res_lock);
                return;
            }
            *id = i;
            if (timer_usr.thread_pool->add_work(timer_usr.thread_pool,
                    (func_thread_handle)(info->routine), info->arg,
                    sys_timer_async_handle, id)) {
                LOGE("Cannot add worker to thread pool\n");
                return;
            }
        }
        pthread_mutex_unlock(&info->res_lock);
    }
}

// static void sys_timer_handle(int arg) {
//     return;
// }

static int32_t timer_generator_setitimer_thread(struct timer_usr *timer) {
    struct sigevent evp;
    int ret;
    memset (&evp, 0, sizeof (evp));
    evp.sigev_value.sival_ptr = &timer->sys_timer_id;
    evp.sigev_notify = SIGEV_THREAD;
    evp.sigev_notify_function = sys_timer_handle;
    evp.sigev_value.sival_int = 0; //parameter passing to signal function

    ret = timer_create(CLOCK_REALTIME,
                &evp, &timer->sys_timer_id);
    if(ret < 0) {
        LOGE("Cannot call timer create, errno:%d\n", errno);
        goto out;
    }
    timer->value.it_value.tv_sec = timer->start_ms / 1000;
    timer->value.it_value.tv_nsec = (timer->start_ms % 1000) * 1000 * 1000;
    timer->value.it_interval.tv_sec = timer->interval_ms / 1000;
    timer->value.it_interval.tv_nsec = (timer->interval_ms % 1000) * 1000 *1000;
    ret = timer_settime(timer->sys_timer_id,
                TIMER_ABSTIME, &timer->value, &timer->ovalue);
    if(ret < 0) {
        LOGE("Cannot call timer settime, errno:%d\n", errno);
        goto out;
    }
    return 0;
out:
    return -1;
}

#if 0
static int32_t timer_generator_setitimer_signal(struct timer_usr *timer) {
    int ret;
    if((timer->old_sigfunc
            = signal(SIGALRM, sys_timer_handle)) == SIG_ERR) {
        LOGE("Failed to call signal function\n");
        goto out;
    }
    timer->value.it_value.tv_sec = timer->start_ms / 1000;
    timer->value.it_value.tv_usec =  (timer->start_ms % 1000) * 1000;
    timer->value.it_interval.tv_sec =  timer->interval_ms / 1000;
    timer->value.it_interval.tv_usec = (timer->interval_ms % 1000) * 1000;
    ret = setitimer(ITIMER_REAL, &timer->value, &timer->ovalue);
    if (ret < 0) {
        LOGE("Failed to call setitimer\n");
        goto out;
    }
    return 0;
out:
    return -1;
}
#endif

static int32_t timer_low_layer_init(struct timer_usr *timer) {
    int32_t ret;

    pthread_mutex_init(&timer->mutex_lock, NULL);
    for (int i = 0; i < TIMER_DEFAULT_MAX_CNT; i++) {
        pthread_mutex_init(&timer->timer_info[i].res_lock, NULL);
    }

    timer->start_ms = TIMER_SYS_DEFAULT_START_MS;
    timer->interval_ms = TIMER_SYS_DEFAULT_INTERVAL_MS;
    timer->work_thread_cnt = TIMER_DEFAULT_WORK_THREAD_MAX_CNT;
    LOGI("System timer start time %d ms, interval time %d ms,"
             "%d worker thread will be generated\n",
            timer->start_ms, timer->interval_ms, timer->work_thread_cnt);

    timer->thread_pool = construct_thread_pool_manager();
    if (timer->thread_pool == NULL) {
        LOGE("Cannot alloc timer thread pool\n");
        goto out;
    }
    ret = timer->thread_pool->init(timer->thread_pool,
            timer_usr.work_thread_cnt, TIMER_WORK_THREAD_PRIORITY);
    if (ret < 0) {
        LOGE("Failed to init thread pool\n");
        goto out;
    }
    #if 1
    ret = timer->thread_pool->start(timer->thread_pool);
    if (ret < 0) {
        LOGE("Failed to start thread pool\n");
        goto out;
    }
    #endif

    ret = timer_generator_setitimer_thread(timer);
    if (ret < 0) {
        LOGE("Failed to generate setitimer thread\n");
        goto out;
    }

    return 0;
out:
    if (timer->thread_pool) {
        deconstruct_thread_pool_manager(&timer->thread_pool);
    }
    return -1;
}

static int32_t timer_low_layer_destory(struct timer_usr *timer) {
    int32_t ret;

    ret = timer->thread_pool->destroy(timer->thread_pool,
            timer->work_thread_cnt);
    if (ret < 0) {
        LOGE("Failed to destroy thread pool\n");
        return -1;
    }
    deconstruct_thread_pool_manager(&timer->thread_pool);
#if 1
    ret = timer_settime(timer->sys_timer_id,
                TIMER_ABSTIME, &timer->ovalue, &timer->value);
    if(ret < 0) {
        LOGE("Cannot call timer settime, errno:%d\n", errno);
        return -1;
    }
    ret = timer_delete(timer->sys_timer_id);
    if(ret < 0) {
        LOGE("Cannot call timer delete, errno:%d\n", errno);
        return -1;
    }
#else
    if((signal(SIGALRM, timer->old_sigfunc)) == SIG_ERR) {
        LOGE("Failed to call signal function\n");
        return -1;
    }

    ret = setitimer(ITIMER_REAL, &timer->ovalue, &timer->value);
    if(ret < 0) {
        LOGE("Failed to call setitimer\n");
        return -1;
    }
#endif
    return 0;
}

static int8_t timer_init(int8_t id, uint32_t interval,
        timer_handle routine, void *arg) {

    struct timer_info *time_info = NULL;
    int8_t assigned = -1;
    int i;
    assert_die_if((id >= TIMER_DEFAULT_MAX_CNT) || (id < -1),
        "Parameter id is overlow, auto allocation is -1, fix allocation from %d to %d\n",
        1, TIMER_DEFAULT_MAX_CNT);
    assert_die_if(routine == NULL, "Parameter routine is null\n");

    if (!timer_usr.init_called_times) {
        if (timer_low_layer_init(&timer_usr) < 0) {
            LOGE("Timer low layer init failed\n");
            return -1;
        }
    }
    timer_usr.init_called_times++;

    if (id >= 0) {
        if (timer_usr.timer_info[id].is_init) {
            LOGE("Timer%d is already in use, please swtich timer\n", id);
            return -1;
        }
        time_info = &timer_usr.timer_info[id];
        assigned = id;
    } else if (id == -1) {
        for(i = 0; i < TIMER_DEFAULT_MAX_CNT; i++) {
            if(timer_usr.timer_info[i].is_init)
                continue;
            time_info = &timer_usr.timer_info[i];
            assigned = i;
            break;
        }
        if(i >= TIMER_DEFAULT_MAX_CNT) {
            LOGE("Cannot find available timer resource\n");
            return -1;
        }
    }
    pthread_mutex_lock(&time_info->res_lock);
    time_info->routine = routine;
    time_info->arg = arg;
    time_info->interval = interval;
    time_info->elapse = 0;
    time_info->is_init = true;
    time_info->is_enable = false;
    pthread_mutex_unlock(&time_info->res_lock);
    return assigned;
}

static int8_t timer_deinit(uint8_t id) {
    assert_die_if((id >= TIMER_DEFAULT_MAX_CNT) || (id < 0),
            "Parameter id is overlow, range from %d to %d\n",
            0, TIMER_DEFAULT_MAX_CNT-1);
    struct timer_info *time_info = &timer_usr.timer_info[id];

    pthread_mutex_lock(&time_info->res_lock);
    if (!time_info->is_init) {
        pthread_mutex_unlock(&time_info->res_lock);
        LOGE("This timer_handle hasn't been set! please call timer_init function!");
        return -1;
    }
    time_info->is_init = false;
    time_info->is_enable = false;
    pthread_mutex_unlock(&time_info->res_lock);

    if (!(--timer_usr.init_called_times)) {
        if (timer_low_layer_destory(&timer_usr) < 0) {
            LOGE("Timer low layer destroy failed\n");
            return -1;
        }
    }
    return 0;
}

static int8_t timer_enable(uint8_t id) {
    assert_die_if((id >= TIMER_DEFAULT_MAX_CNT) || (id < 0),
            "Parameter id is overlow, range from %d to %d\n",
            0, TIMER_DEFAULT_MAX_CNT-1);
    struct timer_info *time_info = &timer_usr.timer_info[id];
    pthread_mutex_lock(&time_info->res_lock);
    if (!time_info->is_init) {
        pthread_mutex_unlock(&time_info->res_lock);
        LOGE("This timer_handle hasn't been set! please call timer_init function!");
        return -1;
    }
    time_info->is_enable = true;
    pthread_mutex_unlock(&time_info->res_lock);
    return 0;
}

static int8_t timer_disable(uint8_t id) {
    assert_die_if((id >= TIMER_DEFAULT_MAX_CNT) || (id < 0),
            "Parameter id is overlow, range from %d to %d\n",
            0, TIMER_DEFAULT_MAX_CNT-1);
    struct timer_info *time_info = &timer_usr.timer_info[id];
    pthread_mutex_lock(&time_info->res_lock);
    if (!time_info->is_init) {
        pthread_mutex_unlock(&time_info->res_lock);
        LOGE("This timer_handle hasn't been set! please call timer_init function!");
        return -1;
    }
    time_info->is_enable = false;
    pthread_mutex_unlock(&time_info->res_lock);
    return 0;
}

static int8_t timer_set_counter(uint8_t id, uint32_t interval) {

    assert_die_if((id >= TIMER_DEFAULT_MAX_CNT) || (id < 0),
            "Parameter id is overlow, range from %d to %d\n",
            0, TIMER_DEFAULT_MAX_CNT-1);
    struct timer_info *time_info = &timer_usr.timer_info[id];
    pthread_mutex_lock(&time_info->res_lock);
    if (!time_info->is_init) {
        pthread_mutex_unlock(&time_info->res_lock);
        LOGE("This timer_handle hasn't been set! please call timer_init function!");
        return -1;
    }
    time_info->interval = interval;
    pthread_mutex_unlock(&time_info->res_lock);
    return 0;
}

static int32_t timer_get_counter(uint8_t id) {
    assert_die_if((id >= TIMER_DEFAULT_MAX_CNT) || (id < 0),
            "Parameter id is overlow, range from %d to %d\n",
            0, TIMER_DEFAULT_MAX_CNT-1);
    struct timer_info *time_info = &timer_usr.timer_info[id];
    pthread_mutex_lock(&time_info->res_lock);
    if (!time_info->is_init) {
        pthread_mutex_unlock(&time_info->res_lock);
        LOGE("This timer_handle hasn't been set! please call timer_init function!");
        return 0;
    }
    pthread_mutex_unlock(&time_info->res_lock);
    return (time_info->interval - time_info->elapse);
}

static struct timer_manager timer_manager = {
    .init = timer_init,
    .deinit = timer_deinit,
    .enable = timer_enable,
    .disable = timer_disable,
    .set_counter = timer_set_counter,
    .get_counter = timer_get_counter,
};

struct timer_manager*  get_timer_manager(void) {
    return &timer_manager;
}
