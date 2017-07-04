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

#ifndef CLIENT_MONITOR_H
#define CLIENT_MONITOR_H

#include <utils/linux.h>
#include "fingerprint_client_sender.h"

#define to_this(ptr, type, member) \
    container_of(ptr, type, member)

struct client_monitor {
    void (*construct)(struct client_monitor* this, int64_t device_id,
            struct fingerprint_client_sender* sender, int user_id, int group_id,
            const char* owner);
    void (*destruct)(struct client_monitor* this);

    int (*start)(struct client_monitor* this);
    int (*stop)(struct client_monitor* this, int initiated_by_client);

    int (*on_enroll_result)(struct client_monitor* this, int finger_id, int group_id, int rem);
    int (*on_authenticated)(struct client_monitor* this, int finger_id, int group_id);
    int (*on_removed)(struct client_monitor* this, int finger_id, int group_id);
    int (*on_enumeration_result)(struct client_monitor* this, int finger_id, int group_id);
    int (*on_acquired)(struct client_monitor* this, int acquired_info);
    int (*on_error)(struct client_monitor* this, int error);

    int64_t (*get_device_id)(struct client_monitor* this);
    int (*get_user_id)(struct client_monitor* this);
    int (*get_group_id)(struct client_monitor* this);
    struct fingerprint_client_sender* (*get_sender)(struct client_monitor* this);
    const char* (*get_owner_string)(struct client_monitor* this);

    int64_t device_id;
    int group_id;
    int user_id;
    char* owner;
    struct fingerprint_client_sender* sender;
};

void construct_client_monitor(struct client_monitor* this, int64_t device_id,
        struct fingerprint_client_sender* sender, int user_id, int group_id,
        const char* owner);
void destruct_client_monitor(struct client_monitor* this);

#endif /* CLIENT_MONITOR_H */
