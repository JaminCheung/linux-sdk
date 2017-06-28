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

#ifndef DEFAULT_RUNNABLE_H
#define DEFAULT_RUNNABLE_H

#include <utils/runnable/runnable.h>

struct default_runnable {
    struct runnable runnable;

    void (*construct)(struct default_runnable* this);
    void (*destruct)(struct default_runnable* this);

    void (*set_thread_count)(struct default_runnable* this, int count);
    int (*start)(struct default_runnable* this, void* param);
    void (*stop)(struct default_runnable* this);
    void (*wait)(struct default_runnable* this);

    int thread_count;
    int is_stop;
    struct pthread_wrapper* thread;
};

void construct_default_runnable(struct default_runnable* this);
void destruct_default_runnable(struct default_runnable* this);

#endif /* DEFAULT_RUNNABLE_H */
