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
#include <fingerprint/fingerprint.h>

#define LOG_TAG "fingerprint"

void construct_fingerprint(struct fingerprint* this) {
    memset(this->name, 0, sizeof(this->name));
    this->group_id = -1;
    this->finger_id = -1;
    this->device_id = -1;
}

void destruct_fingerprint(struct fingerprint* this) {
    memset(this->name, 0, sizeof(this->name));
    this->group_id = -1;
    this->finger_id = -1;
    this->device_id = -1;
}
