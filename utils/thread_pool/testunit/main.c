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
#include <utils/thread_pool.h>

#if 0
#define THREAD_POOL_SINGLETON
#else
#undef THREAD_POOL_SINGLETON
#endif
#define LOG_TAG   "thread_pool"
#define MAX_THREAD_COUNT_REQUEST              3
#define MAX_WORK_COUNT_TO_BE_DONE           500

void test_routine (void *arg) {
    printf ("user routine: thread_id is 0x%x, working on task %d, time %d\n",
        (unsigned int)pthread_self (),*(int *)arg, (unsigned int)time(NULL));
    sleep(1);
    return;
}

int main (int argc, char **argv) {
    struct thread_pool_manager* thread_pool = NULL;
    int *worker_id = NULL;
    int i;
    int ret;

#ifdef THREAD_POOL_SINGLETON
    thread_pool = get_thread_pool_manager();
#else
    thread_pool = construct_thread_pool_manager();
#endif
    if (thread_pool == NULL) {
        LOGE("Cannot alloc thread pool\n");
        goto out;
    }
    worker_id = (int *) malloc(sizeof (int) * MAX_WORK_COUNT_TO_BE_DONE);
    if (worker_id == NULL) {
        LOGE("Cannot alloc more memory\n");
        goto out;
    }

    LOGI("=====create %d thread ======\n", MAX_THREAD_COUNT_REQUEST);
    ret = thread_pool->init(thread_pool, MAX_THREAD_COUNT_REQUEST, -1, -1);
    if (ret < 0) {
        LOGE("Failed to init thread pool\n");
        goto out;
    }
    LOGI("wait for 2 seconds\n");
    sleep(2);
    LOGI("=====add %d piece of work ======\n", MAX_WORK_COUNT_TO_BE_DONE);
    for (i = 0; i < MAX_WORK_COUNT_TO_BE_DONE; i++) {
        worker_id[i] = i;
        if (thread_pool->add_work(thread_pool, test_routine, &worker_id[i], NULL, NULL) < 0) {
            LOGE("Cannot alloc more memory\n");
            goto out;
        }
    }
    LOGI("wait for 3 seconds\n");
    sleep(3);
    LOGI("=====enable worker thread pool ======\n");
    if (thread_pool->start(thread_pool) < 0) {
        LOGE("Failed to start thread pool\n");
        goto out;
    }
    LOGI("wait for 5 seconds\n");
    sleep(5);

    LOGI("=====stop worker thread pool ======\n");
    if (thread_pool->stop(thread_pool) < 0) {
        LOGE("Failed to stop thread pool\n");
        goto out;
    }
    LOGI("wait for 2 seconds\n");
    sleep(2);

    LOGI("=====destroy worker thread poll ======\n");
    ret = thread_pool->destroy(thread_pool, MAX_THREAD_COUNT_REQUEST);
    if (ret < 0) {
        LOGE("Failed to destroy thread pool\n");
        goto out;
    }
    if (worker_id) {
        free (worker_id);
        worker_id = NULL;
    }
    return 0;
out:
    if (worker_id) {
        free (worker_id);
        worker_id = NULL;
    }
#ifndef THREAD_POOL_SINGLETON
    if (thread_pool) {
        deconstruct_thread_pool_manager(&thread_pool);
    }
#endif
    return -1;
}