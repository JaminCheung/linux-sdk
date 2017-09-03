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
#include <string.h>

#include <utils/log.h>
#include <utils/common.h>
#include <utils/assert.h>
#include <ring_buffer/ring_buffer.h>

#define LOG_TAG "ring_buffer"

int set_capacity(struct ring_buffer* this, int capacity) {
    assert_die_if(capacity <= 0, "Invalid capacity.\n");
    assert_die_if(this->capacity > 0, "Capacity already set.\n");

    this->buffer = calloc(1, capacity);
    if (this->buffer == NULL) {
        LOGE("Failed to allocate memory\n");
        return -1;
    }
    this->capacity = capacity;
    this->head = 0;
    this->tail = 0;

    return 0;
}

int get_capacity(struct ring_buffer* this) {
    return this->capacity;
}

int push(struct ring_buffer* this, char* buffer, int size) {
    assert_die_if(this->capacity <= 0, "Invalid capacity.\n");
    assert_die_if(this->buffer == NULL , "buffer is NULL\n");

    if (size > this->capacity) {

        /*
         * resize buffer
         */
        this->buffer = realloc(this->buffer, size);
        if (this->buffer == NULL) {
            LOGE("Failed to reallocate memory\n");
            return -1;
        }

        this->capacity = size;
        this->tail = 0;
        this->head = size - 1;
        this->available_size = size;

        memcpy(this->buffer, buffer, size);

        return size;
    }

    if (size == this->capacity) {
        this->tail = 0;
        this->head = size - 1;
        this->available_size = size;
        memcpy(this->buffer, buffer, size);

        return size;
    }

    int count = MIN(this->capacity - this->available_size, size);

    for (int i = 0; i < count; i++) {
        LOGD("%s, head=%d, wr_data=0x%x\n", __FUNCTION__, this->head, buffer[i]);

        this->buffer[this->head] = buffer[i];
        this->head = (this->head + 1) % (this->capacity);
    }

    this->available_size = MIN(this->available_size + count, this->capacity);

    LOGD("%s: available size: %d\n", __FUNCTION__, this->available_size);

    return count;
}

int pop(struct ring_buffer* this, char* buffer, int size) {
    assert_die_if(this->capacity <= 0, "Invalid capacity.\n");
    assert_die_if(size > this->capacity, "Invalid size.\n");

    int count = MIN(this->available_size, size);

    for (int i = 0; i < count; i++) {
        LOGD("%s tail=%d, rd_data=0x%x\n", __FUNCTION__, this->tail, this->buffer[this->tail]);

        buffer[i] = this->buffer[this->tail];
        this->tail = (this->tail + 1) % (this->capacity);
    }

    this->available_size -= count;

    LOGD("%s: available size: %d\n", __FUNCTION__, this->available_size);

    return count;
}

int empty(struct ring_buffer* this) {
    return this->available_size == 0;
}

int full(struct ring_buffer* this) {
    return this->capacity == this->available_size;
}

int get_available_size(struct ring_buffer* this) {
    return this->available_size;
}

int get_free_size(struct ring_buffer* this) {
    return this->capacity - this->available_size;
}

void construct_ring_buffer(struct ring_buffer* this) {
    this->set_capacity = set_capacity;
    this->get_capacity = get_capacity;
    this->push = push;
    this->pop = pop;
    this->empty = empty;
    this->full = full;
    this->get_available_size = get_available_size;
    this->get_free_size = get_free_size;

    this->buffer = NULL;
    this->capacity = 0;
    this->available_size = 0;
    this->head = 0;
}

void destruct_ring_buffer(struct ring_buffer* this) {
    this->set_capacity = NULL;
    this->get_capacity = NULL;
    this->push = NULL;
    this->pop = NULL;
    this->empty = NULL;
    this->full = NULL;
    this->get_available_size = NULL;

    if (this->buffer)
        free(this->buffer);
    this->buffer = NULL;
    this->capacity = 0;
    this->available_size = 0;
    this->head = 0;
}
