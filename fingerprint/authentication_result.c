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

#include <fingerprint/authentication_result.h>

#define LOG_TAG "authentication_result"

static struct fingerprint* get_fingerprint(struct authentication_result* this) {
    return this->fp;
}

static int get_user_id(struct authentication_result* this) {
    return this->user_id;
}

void construct_authentication_result(struct authentication_result* this,
        struct fingerprint* fp, int user_id) {
    this->get_fingerprint = get_fingerprint;
    this->get_user_id = get_user_id;
    this->fp = fp;
    this->user_id = user_id;
}

void destruct_authentication_result(struct authentication_result* this) {
    this->get_fingerprint = NULL;
    this->get_user_id = NULL;
    this->fp = NULL;
    this->user_id = -1;
}
