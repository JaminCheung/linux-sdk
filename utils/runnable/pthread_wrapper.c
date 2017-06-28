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

#include <unistd.h>
#include <sys/types.h>
#include <pthread.h>
#include <sys/syscall.h>

#include <utils/log.h>
#include <utils/runnable/pthread_wrapper.h>
#include <utils/runnable/runnable.h>

#ifdef _syscall0
static _syscall0(pid_t,gettid)
#else
static pid_t gettid(void) {return (pid_t)syscall(__NR_gettid);}
#endif

#define LOG_TAG "pthread_wrapper"

static void* looper(void* param) {
    struct pthread_wrapper* this = (struct pthread_wrapper*) param;

    this->pid = gettid();

    if (this->get_runnable(this))
        this->get_runnable(this)->run(this, this->get_param(this));

    return (void*)NULL;
}

static int start(struct pthread_wrapper* this, struct runnable* runnable,
        void* param) {
    int error = 0;

    this->runnable = runnable;
    this->param = param;

    error = pthread_create(&this->tid, NULL, looper, this);

    return error;
}

static void join(struct pthread_wrapper* this) {
    if (this->tid) {
        pthread_join(this->tid, NULL);
        this->tid = 0;
        this->pid = 0;
    }
}

static struct runnable* get_runnable(struct pthread_wrapper* this) {
    return this->runnable;
}

static void* get_param(struct pthread_wrapper* this) {
    return this->param;
}

static int get_pid(struct pthread_wrapper* this) {
    return this->pid;
}

void construct_pthread_wrapper(struct pthread_wrapper* this) {
    this->start = start;
    this->join = join;
    this->get_runnable = get_runnable;
    this->get_param = get_param;
    this->get_pid = get_pid;

    this->tid = 0;
    this->pid = 0;
    this->param = NULL;
}

void destruct_pthread_wrapper(struct pthread_wrapper* this) {
    this->start = NULL;
    this->join = NULL;
    this->get_runnable = NULL;
    this->get_param = NULL;
    this->get_pid = NULL;

    this->tid = 0;
    this->pid = 0;
    this->param = NULL;
}
