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
#include <stdbool.h>
#include <stdio.h>

#include <netlink/netlink_handler.h>

#define LOG_TAG "netlink_handler"

static char* get_subsystem(struct netlink_handler *this) {
    return this->subsystem;
}

static int get_priority(struct netlink_handler *this) {
    return this->priority;
}

static void* get_private_data(struct netlink_handler* this) {
    return this->private_data;
}

void construct_netlink_handler(struct netlink_handler *this,
        char* subsystem, int priority,
        void (*handle_event)(struct netlink_handler* this,
                struct netlink_event* event), void* param) {
    this->subsystem = subsystem;
    this->priority = priority;
    this->get_subsystem = get_subsystem;
    this->get_priority = get_priority;
    this->handle_event = handle_event;
    this->private_data = param;
    this->get_private_data = get_private_data;
}

void destruct_netlink_handler(struct netlink_handler *this) {
    this->subsystem = NULL;
    this->priority = -1;
    this->get_subsystem = NULL;
    this->get_priority = NULL;
    this->handle_event = NULL;
    this->private_data = NULL;
    this->get_private_data = NULL;
}
