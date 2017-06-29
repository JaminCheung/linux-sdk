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

#include "client_monitor.h"

#define LOG_TAG "client_monitor"

static int64_t get_device_id(struct client_monitor* this) {
    return this->device_id;
}

static int get_user_id(struct client_monitor* this) {
    return this->user_id;
}

static int get_group_id(struct client_monitor* this) {
    return this->group_id;
}

static struct fingerprint_client_sender* get_sender(struct client_monitor* this) {
    return this->sender;
}

static void on_error(struct client_monitor* this, int error) {
    this->sender->on_error(this->get_device_id(this), error);
}

static void on_acquired(struct client_monitor* this, int acquired_info) {
    this->sender->on_acquired(this->get_device_id(this), acquired_info);
}

void construct_client_monitor(struct client_monitor* this, int64_t device_id,
        struct fingerprint_client_sender* sender, int user_id, int group_id) {

    this->device_id = device_id;
    this->user_id = user_id;
    this->group_id = group_id;
    this->sender = sender;

    this->get_device_id = get_device_id;
    this->get_group_id = get_group_id;
    this->get_user_id = get_user_id;
    this->get_sender = get_sender;

    this->on_error = on_error;
    this->on_acquired = on_acquired;
}

void destruct_client_monitor(struct client_monitor* this) {
    this->device_id = -1;
    this->user_id = -1;
    this->group_id = -1;
    this->sender = NULL;

    this->get_device_id = NULL;
    this->get_group_id = NULL;
    this->get_user_id = NULL;
    this->get_sender = NULL;
}
