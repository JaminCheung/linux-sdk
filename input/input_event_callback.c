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
#include <utils/assert.h>
#include "input_event_callback.h"

#define LOG_TAG "input_event_callback"

void construct_input_event_callback(struct input_event_callback* this,
        const char* name, input_event_listener_t listener) {
    assert_die_if(name == NULL, "name is NULL\n");
    assert_die_if(strlen(name) > NAME_MAX, "name length too long\n");
    assert_die_if(listener == NULL, "listener is NULL\n");

    strncpy(this->name, name, strlen(name));
    this->listener = listener;
}

void destruct_input_event_callback(struct input_event_callback* this) {
    memset(this->name, 0, sizeof(this->name));

}
