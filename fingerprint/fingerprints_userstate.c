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
#include <utils/common.h>
#include <fingerprint/fingerprints_userstate.h>

#define LOG_TAG "fingerprints_userstate"

static pthread_mutex_t this_lock = PTHREAD_MUTEX_INITIALIZER;

static struct fingerprint_list* get_copy(struct fingerprints_userstate* this,
        struct fingerprint_list* list) {

    struct fingerprint_list* copy_list = _new(struct fingerprint_list,
            fingerprint_list);

    for (int i = 0; i < copy_list->size(copy_list); i++) {
        struct fingerprint* fp = list->get(list, i);

        struct fingerprint* copy = calloc(1, sizeof(struct fingerprint));
        copy->construct = construct_fingerprint;
        copy->destruct = destruct_fingerprint;
        copy->construct(copy, fp->get_name(fp), fp->get_group_id(fp),
                fp->get_finger_id(fp), fp->get_device_id(fp));

        copy_list->insert(copy_list, copy);
    }

    return copy_list;
}

static FILE* get_file_for_user(struct fingerprints_userstate* this, int user_id) {
    return NULL;
}

static void read_state_sync(struct fingerprints_userstate* this) {
    pthread_mutex_lock(&this_lock);

    pthread_mutex_unlock(&this_lock);
}

static const char* get_unique_name(struct fingerprints_userstate* this) {
    return NULL;
}

static int is_unique(struct fingerprints_userstate* this, const char* name) {
    for (int i = 0; i < this->fingerprints->size(this->fingerprints); i++) {
        struct fingerprint* fp = this->fingerprints->get(this->fingerprints, i);
        if (!strcmp(fp->get_name(fp), name))
            return 1;
    }

    return 0;
}

static void schedule_write_state(struct fingerprints_userstate* this) {

}

static void add_fingerprint(struct fingerprints_userstate* this, int finger_id, int group_id) {
    pthread_mutex_lock(&this_lock);

    struct fingerprint* fp = calloc(1, sizeof(struct fingerprint));
    fp->construct = construct_fingerprint;
    fp->destruct = destruct_fingerprint;

    fp->construct(fp, get_unique_name(this), group_id, finger_id, 0);
    schedule_write_state(this);

    pthread_mutex_unlock(&this_lock);
}

static void remove_fingerprint(struct fingerprints_userstate* this, int finger_id) {
    pthread_mutex_lock(&this_lock);

    for (int i = 0; i < this->fingerprints->size(this->fingerprints); i++) {
        struct fingerprint* fp = this->fingerprints->get(this->fingerprints, i);
        if (fp->get_finger_id(fp) == finger_id) {
            this->fingerprints->erase(this->fingerprints, i);
            schedule_write_state(this);
        }
    }

    pthread_mutex_unlock(&this_lock);
}

static void rename_fingerprint(struct fingerprints_userstate* this, int finger_id, const char* name) {
    pthread_mutex_lock(&this_lock);

    for (int i = 0; i < this->fingerprints->size(this->fingerprints); i++) {
        struct fingerprint* fp = this->fingerprints->get(this->fingerprints, i);
        if (fp->get_finger_id(fp) == finger_id) {
            fp->set_name(fp, name);
            schedule_write_state(this);
        }
    }

    pthread_mutex_unlock(&this_lock);
}

static struct fingerprint_list* get_fingerprints(struct fingerprints_userstate* this) {
    pthread_mutex_lock(&this_lock);
    struct fingerprint_list* copy_list = get_copy(this, this->fingerprints);
    pthread_mutex_unlock(&this_lock);

    return copy_list;
}

void construct_fingerprints_userstate(struct fingerprints_userstate* this,
        int user_id) {

    this->fp = get_file_for_user(this, user_id);

    read_state_sync(this);

    this->fingerprints = _new(struct fingerprint_list, fingerprint_list);

    this->add_fingerprint = add_fingerprint;
    this->remove_fingerprint = remove_fingerprint;
    this->rename_fingerprint = rename_fingerprint;
    this->get_fingerprints = get_fingerprints;
}

void destruct_fingerprints_userstate(struct fingerprints_userstate* this) {
    _delete(this->fingerprints);

    this->add_fingerprint = NULL;
    this->remove_fingerprint = NULL;
    this->rename_fingerprint = NULL;
    this->get_fingerprints = NULL;
}
