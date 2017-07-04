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

#ifndef FINGERPRINT_USERSTATE_H
#define FINGERPRINT_USERSTATE_H

#include <stdio.h>

#include <utils/runnable/default_runnable.h>
#include <fingerprint/fingerprint_list.h>

struct fingerprints_userstate {
    void (*construct)(struct fingerprints_userstate* this, int user_id);
    void (*destruct)(struct fingerprints_userstate* this);

    void (*add_fingerprint)(struct fingerprints_userstate* this, int finger_id, int group_id);
    void (*remove_fingerprint)(struct fingerprints_userstate* this, int finger_id);
    void (*rename_fingerprint)(struct fingerprints_userstate* this, int finger_id, const char* name);
    struct fingerprint_list* (*get_fingerprints)(struct fingerprints_userstate* this);

    int user_id;
    char file_path[PATH_MAX];
    struct default_runnable* runner;
    pthread_mutex_t lock;
    struct fingerprint_list* fingerprints;
};

void construct_fingerprints_userstate(struct fingerprints_userstate* this, int user_id);
void destruct_fingerprints_userstate(struct fingerprints_userstate* this);

#endif /* FINGERPRINT_USERSTATE_H */
