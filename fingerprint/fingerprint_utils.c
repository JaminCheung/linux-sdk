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
#include <fingerprint/fingerprint_utils.h>

#define LOG_TAG "fingerprint_utilis"

static struct fingerprint_list* get_fingerprints_for_user(int user_id) {
    return NULL;
}

static void add_fingerprint_for_user(int finger_id, int user_id) {

}

static void remove_fingerprint_id_for_user(int finger_id, int user_id) {

}

static void rename_fingerprint_for_user(int finger_id, int user_id,
        const char* name) {

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
