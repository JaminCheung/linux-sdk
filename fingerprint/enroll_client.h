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

#ifndef ENROLL_CLIENT_H
#define ENROLL_CLIENT_H

#include "client_monitor.h"

struct enroll_client {
    struct client_monitor* base;

    void (*construct)(struct enroll_client* this, int64_t device_id,
            struct fingerprint_client_sender* sender, int user_id, int group_id,
            char* token, int token_len);
    void (*destruct)(struct enroll_client* this);

    char* token;
    int token_len;

    int (*send_enroll_result)(struct enroll_client* this, int finger_id,
            int group_id, int remaining);
};

void construct_enroll_client(struct enroll_client* this, int64_t device_id,
        struct fingerprint_client_sender* sender, int user_id, int group_id,
        char* token, int token_len);
void destruct_enroll_client(struct enroll_client* this);

#endif /* ENROLL_CLIENT_H */
