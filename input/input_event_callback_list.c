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
#include <stdlib.h>

#include <utils/log.h>

#include "input_event_callback_list.h"
#include "input_event_callback.h"

#define LOG_TAG "input_event_callback_list"

static void dump(struct input_event_callback_list* this) {
    pthread_mutex_lock(&this->lock);

    struct input_event_callback* callback;
    list_for_each_entry(callback, &this->list, node) {
        LOGI("----------------------------------------\n");
        LOGI("name: %s\n", callback->name);
    }

    pthread_mutex_unlock(&this->lock);
}

static void insert_callback(struct input_event_callback_list* this,
        struct input_event_callback* callback) {
    pthread_mutex_lock(&this->lock);

    list_add_tail(&callback->node, &this->list);

    this->the_size++;

    pthread_mutex_unlock(&this->lock);
}

static void remove_callback(struct input_event_callback_list* this,
        struct input_event_callback* callback) {
    struct list_head *pos, *next_pos;

    pthread_mutex_lock(&this->lock);

    if (!list_empty(&this->list)) {
        list_for_each_safe(pos, next_pos, &this->list) {
            struct input_event_callback* cb = list_entry(pos,
                    struct input_event_callback, node);
            if (callback == cb) {
                list_del(&callback->node);

                cb->destruct(cb);
                free(cb);

                this->the_size--;

                break;
            }
        }
    }

    pthread_mutex_unlock(&this->lock);
}

static void remove_all(struct input_event_callback_list* this) {
    struct list_head *pos, *next_pos;

    pthread_mutex_lock(&this->lock);

    list_for_each_safe(pos, next_pos, &this->list) {
        struct input_event_callback* cb = list_entry(pos, struct input_event_callback, node);

        list_del(&cb->node);

        cb->destruct(cb);
        free(cb);
    }

    this->the_size = 0;

    pthread_mutex_unlock(&this->lock);
}

static int size(struct input_event_callback_list* this) {
    pthread_mutex_lock(&this->lock);

    int size = this->the_size;

    pthread_mutex_unlock(&this->lock);

    return size;
}

static struct input_event_callback* get(struct input_event_callback_list* this,
        int index) {
    struct input_event_callback* cb = NULL;
    int i = 0;

    pthread_mutex_lock(&this->lock);

    if (!list_empty(&this->list)) {
        if (index > get_list_size_locked(&this->list) - 1)
            goto out;

        list_for_each_entry(cb, &this->list, node) {
            if (i++ == index)
                break;
        }
    }

out:
    pthread_mutex_unlock(&this->lock);

    return cb;
}

static int empty(struct input_event_callback_list* this) {
    pthread_mutex_lock(&this->lock);

    int empty = this->the_size == 0;

    pthread_mutex_unlock(&this->lock);

    return empty;
}

void construct_input_event_callback_list(struct input_event_callback_list* this) {
    this->insert_callback = insert_callback;
    this->remove_callback = remove_callback;
    this->size = size;
    this->get = get;
    this->empty = empty;
    this->dump = dump;

    INIT_LIST_HEAD(&this->list);

    pthread_mutex_init(&this->lock, NULL);
}

void destruct_input_event_callback_list(struct input_event_callback_list* this) {
    remove_all(this);

    pthread_mutex_destroy(&this->lock);

    this->insert_callback = NULL;
    this->remove_callback = NULL;
    this->remove_all = NULL;
    this->size = NULL;
    this->get = NULL;
    this->empty = NULL;
    this->dump = NULL;

    this->the_size = 0;
}
