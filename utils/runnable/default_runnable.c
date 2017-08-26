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

#include <utils/log.h>
#include <utils/common.h>
#include <utils/runnable/pthread_wrapper.h>
#include <utils/runnable/default_runnable.h>

#define LOG_TAG "default_runnable"

static void set_thread_count(struct default_runnable* this, int count) {
    if (this->thread != NULL) {
        LOGE("Already running, can't set thread count\n");
        return;
    }

    this->thread_count = count;
}

static int start(struct default_runnable* this, void* param) {
    int i = 0;

    if (this->thread != NULL && this->thread_count < 1)
        return -1;

    if (this->thread == NULL) {
        this->thread = calloc(this->thread_count, sizeof(struct pthread_wrapper));
        for (i = 0; i < this->thread_count; i++) {
            this->thread[i].construct = construct_pthread_wrapper;
            this->thread[i].destruct = destruct_pthread_wrapper;
            this->thread[i].construct(&this->thread[i]);
        }
    }

    if (this->thread == NULL) {
        LOGE("Failed to create %d threads\n", this->thread_count);
        return -1;
    }

    for (i = 0; i < this->thread_count; i++) {
        if (this->thread[i].start(&this->thread[i], &this->runnable, param))
            return i;
    }

    this->is_stop = 0;

    return i;
}

static void stop(struct default_runnable* this) {
    if (this->thread != NULL) {
        for (int i = 0; i < this->thread_count; i++) {
            this->thread[i].cancel(&this->thread[i]);
            this->thread[i].join(&this->thread[i]);
        }

        for (int i = 0; i < this->thread_count; i++)
            this->thread[i].destruct(&this->thread[i]);

        free(this->thread);
        this->thread = NULL;
    }

    this->is_stop = 1;
}

static void wait(struct default_runnable* this) {
    if (this->thread != NULL) {
        for (int i = 0; i < this->thread_count; i++)
            this->thread[i].join(&this->thread[i]);
    }
}

void construct_default_runnable(struct default_runnable* this) {
    this->set_thread_count = set_thread_count;
    this->start = start;
    this->stop = stop;
    this->wait = wait;

    this->is_stop = 0;
    this->thread_count = 1;
    this->thread = NULL;
}

void destruct_default_runnable(struct default_runnable* this) {
    this->stop(this);

    this->set_thread_count = NULL;
    this->start = NULL;
    this->stop = NULL;
    this->wait = NULL;

    this->is_stop = 0;
    this->thread_count = -1;
    this->thread = NULL;
}
