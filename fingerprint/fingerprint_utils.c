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
#include <utils/list.h>
#include <utils/common.h>
#include "fingerprint_utils.h"
#include "fingerprints_userstate.h"

#define LOG_TAG "fingerprint_utils"

LIST_HEAD(userstate_list);

static pthread_mutex_t list_lock = PTHREAD_MUTEX_INITIALIZER;

struct userstate {
    int id;
    struct fingerprints_userstate* user;
    struct list_head node;
};

static struct fingerprints_userstate* get_state_for_user(int user_id) {
    pthread_mutex_lock(&list_lock);

    struct userstate* userstate = NULL;

    list_for_each_entry(userstate, &userstate_list, node) {
        if (user_id == userstate->id)
            goto out;
    }

    if (userstate == NULL) {
        struct userstate* userstate = calloc(1, sizeof(struct userstate));

        userstate->user = calloc(1, sizeof(struct fingerprints_userstate));
        userstate->user->construct = construct_fingerprints_userstate;
        userstate->user->destruct = destruct_fingerprints_userstate;
        userstate->user->construct(userstate->user, user_id);

        userstate->id = user_id;
        list_add_tail(&userstate->node, &userstate_list);
    }

out:
    pthread_mutex_unlock(&list_lock);

    return userstate->user;
}

static struct fingerprint_list* get_fingerprints_for_user(int user_id) {
    struct fingerprints_userstate* userstate = get_state_for_user(user_id);

    return userstate->get_fingerprints(userstate);
}

static void add_fingerprint_for_user(int finger_id, int user_id) {
    struct fingerprints_userstate* userstate = get_state_for_user(user_id);

    userstate->add_fingerprint(userstate, finger_id, user_id);
}

static void remove_fingerprint_id_for_user(int finger_id, int user_id) {
    struct fingerprints_userstate* userstate = get_state_for_user(user_id);

    userstate->remove_fingerprint(userstate, finger_id);
}

static void rename_fingerprint_for_user(int finger_id, int user_id,
        const char* name) {
    if (name == NULL || strlen(name) == 0)
        return;

    struct fingerprints_userstate* userstate = get_state_for_user(user_id);

    userstate->rename_fingerprint(userstate, finger_id, name);
}

static struct fingerprint_utils this = {
        .get_fingerprints_for_user = get_fingerprints_for_user,
        .add_fingerprint_for_user = add_fingerprint_for_user,
        .remove_fingerprint_id_for_user = remove_fingerprint_id_for_user,
        .rename_fingerprint_for_user = rename_fingerprint_for_user,
};

struct fingerprint_utils* get_fingerprint_utils(void) {
    return &this;
}
