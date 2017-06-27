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

#ifndef INPUT_EVENT_CALLBACK_LIST_H
#define INPUT_EVENT_CALLBACK_LIST_H

#include <utils/list.h>

#include "input_event_callback.h"

struct input_event_callback_list {
    void (*construct)(struct input_event_callback_list* this);
    void (*destruct)(struct input_event_callback_list* this);
    void (*insert_callback)(struct input_event_callback_list* this, struct input_event_callback* callback);
    void (*remove_callback)(struct input_event_callback_list* this, struct input_event_callback* callback);
    void (*remove_all)(struct input_event_callback_list* this);
    int (*size)(struct input_event_callback_list* this);
    struct input_event_callback* (*get)(struct input_event_callback_list* this, int index);
    int (*empty)(struct input_event_callback_list* this);
    void (*dump)(struct input_event_callback_list* this);

    int the_size;
    struct list_head list;
    pthread_mutex_t lock;
};

void construct_input_event_callback_list(struct input_event_callback_list* this);
void destruct_input_event_callback_list(struct input_event_callback_list* this);

#endif /* INPUT_EVENT_CALLBACK_LIST_H */
