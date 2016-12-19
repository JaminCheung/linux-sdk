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
# include <unistd.h>
# include <utime.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <types.h>
#include <utils/log.h>
#include <utils/assert.h>
#include <utils/list.h>
#include <utils/thread_pool.h>

#define LOG_TAG "thread_pool"

static void dump_work_queue(struct thread_pool_manager* pm) {
    assert_die_if(pm == NULL, "Paramter pm is null\n");
    struct thread_pool *pool = pm->pool;
    assert_die_if(pool == NULL, "Thread pool struct is lost\n");
    struct thread_work *worker = NULL;
    struct list_head *cell;
    list_for_each(cell, &pool->work_head) {
        worker = list_entry(cell, struct thread_work, list_cell);
        LOGI("work %d\n", *(int*)(worker->arg));
    }
}

static void * thread_routine(void *arg) {
    struct thread_work *worker = NULL;
    struct thread_pool *pool = arg;
    assert_die_if(pool == NULL, "Paramter arg is null\n");
    while (1) {
        pthread_mutex_lock(&(pool->queue_lock));
        while (pool->stop || !pool->queue_size) {
            if (pool->exit) {
                pthread_mutex_unlock(&(pool->queue_lock));
                goto out;
            }
             // LOGI("thread 0x%x is waiting\n", (unsigned int)pthread_self());
             pthread_cond_wait (&(pool->queue_ready), &(pool->queue_lock));
        }
        // LOGI ("thread 0x%x is starting, work queue size %d\n",
        //         (unsigned int)pthread_self(), pool->queue_size);

        if ((worker = list_first_entry_or_null(&(pool->work_head),
                struct thread_work, list_cell))) {
            list_del(&worker->list_cell);
            pool->queue_size--;
        }else {
            pthread_mutex_unlock(&(pool->queue_lock));
            continue;
        }
        pthread_mutex_unlock(&(pool->queue_lock));

        if (worker->routine) {
            (*(worker->routine)) (worker->arg);
        }
        if (worker->async) {
            (*(worker->async)) (worker->async_arg);
        }
        if (worker) {
            free (worker);
            worker = NULL;
        }
    }
out:
    LOGI("thread 0x%x is exit\n", (unsigned int)pthread_self());
    pthread_exit (NULL);
}

static void dump_work_queue(struct thread_pool_manager* pm);

static int32_t thread_pool_destroy(struct thread_pool_manager* pm, uint32_t thread_cnt) {
    struct thread_pool *pool_tmp = NULL;
    struct thread_work *worker = NULL;

    assert_die_if(pm == NULL, "Paramter pm is null\n");
    assert_die_if(thread_cnt <= 0, "Paramter thread count must be plus number\n");

    pool_tmp = pm->pool;
    assert_die_if(pool_tmp == NULL, "Thread pool struct is lost\n");
    if (pool_tmp->exit) {
        LOGE("Function Destroy is already called before\n");
        return -1;
    }

    pthread_mutex_lock (&pool_tmp->queue_lock);
    pool_tmp->stop = true;
    pool_tmp->exit = true;
    pthread_cond_broadcast (&pool_tmp->queue_ready);
    pthread_mutex_unlock (&pool_tmp->queue_lock);

    for (int i = thread_cnt - 1; i >= 0; i--) {
        if (pthread_join (pool_tmp->thread_id[i], NULL) < 0) {
            LOGE("Thread 0x%x is failed while waiting\n", (unsigned int)pool_tmp->thread_id[i]);
            return -1;
        }
    }

    if (pool_tmp->thread_id) {
        free (pool_tmp->thread_id);
        pool_tmp->thread_id = NULL;
    }

    struct list_head *cell;
    struct list_head* next;
    if (!list_empty(&pool_tmp->work_head)) {
        list_for_each_safe(cell, next, &pool_tmp->work_head) {
            worker = list_entry(cell, struct thread_work, list_cell);
            list_del(cell);
            pool_tmp->queue_size--;
            if (worker) {
                free (worker);
            }
        }
    }
    pthread_mutex_destroy(&(pool_tmp->queue_lock));
    pthread_cond_destroy(&(pool_tmp->queue_ready));

    if (pool_tmp) {
        free (pool_tmp);
        pm->pool = NULL;
    }
    return 0;
}

int32_t thread_set_priority(pthread_attr_t *attr, int policy, int priority) {
    int policy_cur, min_priority, max_priority;
    struct sched_param param;
    char *policy_str;
    if ((policy != SCHED_FIFO) && (policy != SCHED_RR)) {
        LOGE("Thread policy must be one of SCHED_FIFO/SCHED_RR\n");
        return -1;
    }
    if (pthread_attr_setschedpolicy(attr, policy)) {
        LOGE("Cannot set pthread sched policy, errno = %d\n", errno);
        return -1;
    }
    min_priority = sched_get_priority_min(policy);
    if(min_priority == -1) {
        LOGE("Get SCHED_RR min priority");
        return -1;
    }
    max_priority = sched_get_priority_max(policy);
    if(max_priority == -1) {
        LOGE("Get SCHED_RR max priority");
        return -1;
    }
    if ((priority > max_priority) || (priority < min_priority)) {
        LOGI("SCHED_RR priority range must from %d to %d\n",
            min_priority, max_priority);
        return -1;
    }
    param.sched_priority = priority;
    pthread_attr_setschedparam(attr, &param);
    pthread_attr_setinheritsched(attr, PTHREAD_EXPLICIT_SCHED);

    if (pthread_attr_getschedpolicy(attr, &policy_cur)) {
        LOGE("Cannot get pthread attribute, errno = %d\n", errno);
        return -1;
    }
    if (pthread_attr_getschedparam(attr, &param)) {
        LOGE("Cannot get pthread sched params, errno = %d\n", errno);
        return -1;
    }
    policy_str =  (policy_cur == SCHED_FIFO ? "FIFO"
         : (policy_cur == SCHED_RR ? "RR"
            : (policy_cur == SCHED_OTHER ? "OTHER"
               : "unknown")));
    LOGI("Thread set priority %s/%d\n",
            policy_str, param.sched_priority);
    return 0;
}


static int32_t thread_pool_init(struct thread_pool_manager* pm,
        uint32_t thread_cnt, int32_t policy, int32_t priority) {
    struct thread_pool *pool_tmp = NULL;
    pthread_attr_t thread_attr;

    int i = 0;

    assert_die_if(pm == NULL, "Paramter pm is null\n");
    assert_die_if(thread_cnt <= 0, "Paramter thread count must be plus number\n");

    pthread_attr_init(&thread_attr);
    if ((priority > 0) && (policy > 0)) {
        if (thread_set_priority(&thread_attr, policy, priority) < 0) {
            pthread_attr_destroy(&thread_attr);
            return -1;
        }
    }

    pool_tmp = (struct thread_pool *)malloc(sizeof(*pool_tmp));
    if (pool_tmp == NULL) {
        LOGE("Cannot alloc more memory\n");
        goto out;
    }
    pthread_mutex_init (&(pool_tmp->queue_lock), NULL);
    pthread_cond_init (&(pool_tmp->queue_ready), NULL);

    pool_tmp->thread_id = (pthread_t *)calloc(thread_cnt, sizeof (pthread_t));
    if (pool_tmp->thread_id == NULL) {
        LOGE("Cannot alloc more memory\n");
        goto out;
    }
    pool_tmp->max_thread_cnt = thread_cnt;
    pool_tmp->queue_head = NULL;
    pool_tmp->queue_end = NULL;
    pool_tmp->queue_size = 0;
    pool_tmp->stop = true;
    pool_tmp->exit = false;
    INIT_LIST_HEAD(&pool_tmp->work_head);
    for (i = 0; i < pool_tmp->max_thread_cnt; i++) {
        if (pthread_create (&(pool_tmp->thread_id[i]), &thread_attr, thread_routine,
                pool_tmp)  < 0) {
            LOGE("Failed to call pthread_create for all %d threads\n",
                    pool_tmp->max_thread_cnt);
            goto out;
        }
    }
    pthread_attr_destroy(&thread_attr);
    pm->pool = pool_tmp;
    return 0;
out:
    pthread_attr_destroy(&thread_attr);
    if (i <= 0) {
        if (pool_tmp->thread_id) {
            free(pool_tmp->thread_id);
            pool_tmp->thread_id = NULL;
        }
        if (pool_tmp) {
            free(pool_tmp);
            pool_tmp = NULL;
        }
        return -1;
    }
    thread_pool_destroy(pm, i);
    return -1;
}

static int32_t thread_pool_start(struct thread_pool_manager* pm) {
    assert_die_if(pm == NULL, "Paramter pm is null\n");
    struct thread_pool *pool = pm->pool;
    assert_die_if(pool == NULL, "Thread pool struct is lost\n");
    pthread_mutex_lock (&pool->queue_lock);
    pool->stop = false;
    pthread_cond_broadcast (&pool->queue_ready);
    pthread_mutex_unlock (&pool->queue_lock);
    return 0;
}

static int32_t thread_pool_stop(struct thread_pool_manager* pm) {
    assert_die_if(pm == NULL, "Paramter pm is null\n");
    struct thread_pool *pool = pm->pool;
    assert_die_if(pool == NULL, "Thread pool struct is lost\n");
    pthread_mutex_lock (&pool->queue_lock);
    pool->stop = true;
    pthread_cond_broadcast (&pool->queue_ready);
    pthread_mutex_unlock (&pool->queue_lock);
    return 0;
}

static int32_t thread_pool_add_work(struct thread_pool_manager* pm, func_thread_handle routine,
            void *arg,  func_thread_async async, void *async_arg) {
    struct thread_work *worker = NULL;

    assert_die_if(pm == NULL, "Paramter pm is null\n");
    assert_die_if(routine == NULL, "Paramter routine is null\n");

    struct thread_pool *pool = pm->pool;
    assert_die_if(pool == NULL, "Thread pool struct is lost\n");

    worker = (struct thread_work *)calloc(1, sizeof (*worker));
    if (worker == NULL) {
        LOGE("Cannot alloc more memory\n");
        goto out;
    }
    worker->routine = routine;
    worker->arg = arg;
    worker->async = async;
    worker->async_arg = async_arg;
    pthread_mutex_lock (&pool->queue_lock);
    list_add_tail(&worker->list_cell, &pool->work_head);
    pool->queue_size++;
    if (!pool->stop)
        pthread_cond_signal (&pool->queue_ready);
    pthread_mutex_unlock (&pool->queue_lock);

    return 0;
out:
    if (worker) {
        free(worker);
        worker = NULL;
    }
    return -1;
}

static struct thread_pool_manager singleton_thread_pool_manager = {
    .init   = thread_pool_init,
    .destroy = thread_pool_destroy,
    .start = thread_pool_start,
    .stop = thread_pool_stop,
    .add_work = thread_pool_add_work,
};

struct thread_pool_manager* get_thread_pool_manager(void) {
    return &singleton_thread_pool_manager;
}

struct thread_pool_manager* construct_thread_pool_manager(void) {
    struct thread_pool_manager* manager  = NULL;
    manager = calloc(1, sizeof(*manager));
    if (manager == NULL) {
        LOGE("Cannot alloc more memory\n");
        goto out;
    }
    manager->init = thread_pool_init;
    manager->destroy = thread_pool_destroy;
    manager->start = thread_pool_start;
    manager->stop = thread_pool_stop;
    manager->add_work = thread_pool_add_work;

    return manager;
out:
    if (manager == NULL) {
        free(manager);
        manager = NULL;
    }
    return NULL;
}

int32_t deconstruct_thread_pool_manager(struct thread_pool_manager** manager) {
    if (manager && *manager) {
        free(*manager);
        *manager = NULL;
    }
    return 0;
}