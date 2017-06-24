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
#include <fingerprint/fingerprint_userstate.h>

#define LOG_TAG "fingerprint_userstate"

static void add_fingerprint(struct fingerprint_userstate* this, int finger_id, int group_id) {

}

static void remove_fingerprint(struct fingerprint_userstate* this, int finger_id) {

}

static void rename_fingerprint(struct fingerprint_userstate* this, int finger_id, const char* name) {

}

static struct fingerprint_list* get_fingerprints(struct fingerprint_userstate* this) {
    return NULL;
}


void construct_fingerprint_userstate(struct fingerprint_userstate* this) {
    this->add_fingerprint = add_fingerprint;
    this->remove_fingerprint = remove_fingerprint;
    this->rename_fingerprint = rename_fingerprint;
    this->get_fingerprints = get_fingerprints;
}

void destruct_fingerprint_userstate(struct fingerprint_userstate* this) {
    this->add_fingerprint = NULL;
    this->remove_fingerprint = NULL;
    this->rename_fingerprint = NULL;
    this->get_fingerprints = NULL;
}
