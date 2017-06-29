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
#include "removal_client.h"

#define LOG_TAG "remval_client"

static void on_enroll_result(struct client_monitor* base, int finger_id,
        int group_id, int remaining) {

}

static void on_removed(struct client_monitor* base, int finger_id, int group_id) {

}

static void on_enumeration_result(struct client_monitor* base, int finger_id,
        int group_id) {

}

static void on_authenticated(struct client_monitor* base, int finger_id,
        int group_id) {

}

static void start(struct client_monitor* base) {

}

static void stop(struct client_monitor* base, int initiated_by_client) {

}

static void send_removed(struct removal_client* this, int finger_id, int group_id) {

}

void construct_removal_client(struct removal_client* this, int64_t device_id,
        struct fingerprint_client_sender* sender, int finger_id,
        int group_id, int user_id) {

    this->base = calloc(1, sizeof(struct client_monitor));
    this->base->construct = construct_client_monitor;
    this->base->destruct = destruct_client_monitor;
    this->base->construct(this->base, device_id, sender, user_id, group_id);
    this->base->start = start;
    this->base->stop = stop;
    this->base->on_enroll_result = on_enroll_result;
    this->base->on_removed = on_removed;
    this->base->on_enumeration_result = on_enumeration_result;
    this->base->on_authenticated = on_authenticated;

    this->send_removed = send_removed;
}

void destruct_removal_client(struct removal_client* this) {
    this->base->destruct(this->base);
    free(this->base);
    this->base = NULL;
}
