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

#ifndef REMOVAL_CLIENT_H
#define REMOVAL_CLIENT_H

#include "client_monitor.h"

struct removal_client {
    struct client_monitor* base;

    void (*construct)(struct removal_client* this, int64_t device_id,
            struct fingerprint_client_sender* sender, int finger_id,
            int group_id, int user_id, const char* owner);
    void (*destruct)(struct removal_client* this);

    int (*send_removed)(struct removal_client* this, int finger_id,
            int group_id);

    int finger_id;
};

void construct_removal_client(struct removal_client* this, int64_t device_id,
        struct fingerprint_client_sender* sender, int finger_id,
        int group_id, int user_id, const char* owner);
void destruct_removal_client(struct removal_client* this);

#endif /* REMOVAL_CLIENT_H */
