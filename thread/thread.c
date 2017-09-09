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

#include <string.h>

#include <utils/log.h>
#include <utils/common.h>
#include <utils/assert.h>
#include <thread/thread.h>

#define LOG_TAG "thread"

static void wait_thread_terminate(struct thread* this) {
    while (this->is_running(this));

    for (int i = 0; i < this->pthread_count; i++)
        this->pthreads[i].destruct(&this->pthreads[i]);

    free(this->pthreads);
    this->pthreads = NULL;
}

static void set_thread_count(struct thread* this, int count) {
    assert_die_if(count < 1, "Invalid thread count\n");

    if (this->pthreads != NULL) {
        LOGE("Thread already running, can't set thread count\n");
        return;
    }

    this->pthread_count = count;
}

static int start(struct thread* this, void* param) {
    assert_die_if(this->pthread_count < 1, "Invalid thread count\n");

    int i = 0;
    int error = 0;

    if (this->pthreads != NULL) {
        LOGE("Thread already running, can't start again\n");
        return 0;
    }

    if (this->pthreads == NULL) {
        this->pthreads = calloc(this->pthread_count, sizeof(struct pthread_wrapper));
        for (i = 0; i < this->pthread_count; i++) {
            this->pthreads[i].construct = construct_pthread_wrapper;
            this->pthreads[i].destruct = destruct_pthread_wrapper;
            this->pthreads[i].construct(&this->pthreads[i]);
        }
    }

    if (this->pthreads == NULL) {
        LOGE("Failed to create %d threads\n", this->pthread_count);
        return -1;
    }

    for (i = 0; i < this->pthread_count; i++) {
        error = this->pthreads[i].start(&this->pthreads[i], &this->runnable, param);
        if (error != 0) {
            LOGE("Failed to start thread %d: %s\n", error, strerror(error));
            goto error;
        }
    }

    while (!this->is_running(this));

    return i;

error:
    for (i = 0; i < this->pthread_count; i++)
        this->pthreads[i].destruct(&this->pthreads[i]);

    free(this->pthreads);
    this->pthreads = NULL;

    return -1;
}

static void stop(struct thread* this) {
    if (this->pthreads != NULL) {
        for (int i = 0; i < this->pthread_count; i++) {
            this->pthreads[i].cancel(&this->pthreads[i]);
            this->pthreads[i].join(&this->pthreads[i]);
        }

        wait_thread_terminate(this);
    }
}

static void wait(struct thread* this) {
    if (this->pthreads != NULL) {
        for (int i = 0; i < this->pthread_count; i++)
            this->pthreads[i].join(&this->pthreads[i]);

        wait_thread_terminate(this);
    }
}

static int is_running(struct thread* this) {
    if (this->pthreads != NULL) {
        int i;

        for (i = 0; i < this->pthread_count; i++) {
            if (this->pthreads[i].is_running(&this->pthreads[i]))
                break;
        }

        return (i != this->pthread_count);
    }

    return 0;
}

void construct_thread(struct thread* this) {
    this->set_thread_count = set_thread_count;
    this->start = start;
    this->is_running = is_running;
    this->stop = stop;
    this->wait = wait;

    this->pthread_count = 1;
    this->pthreads = NULL;
}

void destruct_thread(struct thread* this) {
    this->stop(this);

    this->set_thread_count = NULL;
    this->start = NULL;
    this->is_running = NULL;
    this->stop = NULL;
    this->wait = NULL;

    this->pthread_count = -1;
    this->pthreads = NULL;
}
