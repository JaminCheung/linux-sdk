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
#include <utils/assert.h>
#include <fingerprint/fingerprint.h>

#define LOG_TAG "fingerprint"

static void dump(struct fingerprint* this) {
    LOGI("========================================\n");
    LOGI("Dump fingerprint.\n");
    LOGI("name:       %s\n", this->name);
    LOGI("finger id:  0x%x\n", this->finger_id);
    LOGI("group  id:  0x%x\n", this->group_id);
    LOGI("device id:  0x%llx\n", this->device_id);
    LOGI("========================================\n");
}

static void set_name(struct fingerprint* this, const char* name) {
    assert_die_if(name == NULL, "name is NULL\n");
    assert_die_if(strlen(name) > NAME_MAX, "name length too long\n");

    memset(this->name, 0, sizeof(this->name));

    strncpy(this->name, name, strlen(name));
}

static const char* get_name(struct fingerprint* this) {
    return this->name;
}

static const int get_group_id(struct fingerprint* this) {
    return this->group_id;
}

static const int get_finger_id(struct fingerprint* this) {
    return this->finger_id;
}

static const int get_device_id(struct fingerprint* this) {
    return this->device_id;
}

static const int equal(struct fingerprint* this, struct fingerprint* other) {
    if (!strncmp(this->name, other->name, strlen(other->name)) &&
            this->device_id == other->device_id &&
            this->finger_id == other->finger_id &&
            this->group_id == other->group_id)
        return 1;

    return 0;
}

void construct_fingerprint(struct fingerprint* this, const char* name,
        int group_id, int finger_id, int64_t device_id) {

    assert_die_if(name == NULL, "name is NULL\n");
    assert_die_if(strlen(name) > NAME_MAX, "name length too long\n");

    strncpy(this->name, name, strlen(name));
    this->group_id = group_id;
    this->finger_id = finger_id;
    this->device_id = device_id;

    this->get_name = get_name;
    this->get_group_id = get_group_id;
    this->get_device_id = get_device_id;
    this->get_finger_id = get_finger_id;

    this->dump = dump;
    this->equal = equal;
    this->set_name = set_name;
}

void destruct_fingerprint(struct fingerprint* this) {
    this->get_name = NULL;
    this->get_group_id = NULL;
    this->get_device_id = NULL;
    this->get_finger_id = NULL;

    this->dump = NULL;
    this->equal = NULL;
    this->set_name = NULL;

    memset(this->name, 0, sizeof(this->name));
    this->group_id = -1;
    this->finger_id = -1;
    this->device_id = -1;
}
