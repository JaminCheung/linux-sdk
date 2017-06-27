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

#ifndef INPUT_EVENT_CALLBACK_H
#define INPUT_EVENT_CALLBACK_H

#include <linux/input.h>

#include <limits.h>

#include <utils/list.h>

typedef void (*input_event_listener_t)(const char* input_name,
        struct input_event *event);

struct input_event_callback {
    void (*construct)(struct input_event_callback* this,
            const char* name, input_event_listener_t listener);
    void (*destruct)(struct input_event_callback* this);

    input_event_listener_t listener;
    char name[NAME_MAX];

    struct list_head node;
};

void construct_input_event_callback(struct input_event_callback* this,
        const char* name, input_event_listener_t listener);
void destruct_input_event_callback(struct input_event_callback* this);

#endif /* INPUT_EVENT_CALLBACK_H */
