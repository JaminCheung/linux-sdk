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
#include <utils/common.h>
#include <utils/file_ops.h>
#include <lib/mxml/mxml.h>
#include "fingerprints_userstate.h"

#define LOG_TAG "fingerprints_userstate"

#define TAG_FINGERPRINTS "fingerprints"
#define TAG_FINGERPRINT  "fingerprint"
#define ATTR_NAME        "name"
#define ATTR_GROUP_ID    "groupId"
#define ATTR_FINGER_ID   "fingerId"
#define ATTR_DEVICE_ID   "deviceId"

#define FINGERPRINT_FILE     "fingerprint.xml"
#define FINGER_NAME_TEMPLATE "fingerId"

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

static mxml_type_t mxml_type_callback(mxml_node_t* node) {
    const char *type;

    if ((type = mxmlElementGetAttr(node, "type")) == NULL)
      type = node->value.element.name;

    if (!strcmp(type, "integer"))
      return (MXML_INTEGER);
    else if (!strcmp(type, "opaque") || !strcmp(type, "pre"))
      return (MXML_OPAQUE);
    else if (!strcmp(type, "real"))
      return (MXML_REAL);
    else
      return (MXML_TEXT);
}

static void parase_state_locked(struct fingerprints_userstate* this, FILE* fp) {
    mxml_node_t *tree = NULL;
    mxml_node_t *node = NULL;

    tree = mxmlLoadFile(NULL, fp, mxml_type_callback);

    node = mxmlFindElement(tree, tree, TAG_FINGERPRINTS, NULL, NULL, MXML_DESCEND);
    for (node = mxmlFindElement(node, tree, TAG_FINGERPRINT, NULL, NULL, MXML_DESCEND);
        node != NULL;
        node = mxmlFindElement(node, tree, TAG_FINGERPRINT, NULL, NULL, MXML_DESCEND)) {
        const char* name = mxmlElementGetAttr(node, ATTR_NAME);
        const char* group_str = mxmlElementGetAttr(node, ATTR_GROUP_ID);
        const char* finger_str = mxmlElementGetAttr(node, ATTR_FINGER_ID);
        const char* device_str = mxmlElementGetAttr(node, ATTR_DEVICE_ID);

        struct fingerprint* fp = calloc(1, sizeof(struct fingerprint));
        fp->construct = construct_fingerprint;
        fp->destruct = destruct_fingerprint;
        fp->construct(fp, name, strtol(group_str, NULL, 0),
                strtol(finger_str, NULL, 0),
                strtol(device_str, NULL, 0));

        this->fingerprints->insert(this->fingerprints, fp);
    }

    this->fingerprints->dump(this->fingerprints);
}

static void read_state_sync(struct fingerprints_userstate* this) {
    pthread_mutex_lock(&this->lock);

    char* dir = get_user_system_dir(this->user_id);
    sprintf(this->file_path, "%s/%s", dir, FINGERPRINT_FILE);

    if (file_exist(this->file_path) != 0) {
        LOGE("Fingerprint XML %s not exist\n", this->file_path);
        goto out;
    }

    FILE* fp = fopen(this->file_path, "r");
    parase_state_locked(this, fp);
    fclose(fp);

out:
    pthread_mutex_unlock(&this->lock);
}

static int is_unique(struct fingerprints_userstate* this, const char* name) {
    for (int i = 0; i < this->fingerprints->size(this->fingerprints); i++) {
        struct fingerprint* fp = this->fingerprints->get(this->fingerprints, i);
        if (!strcmp(fp->get_name(fp), name))
            return 1;
    }

    return 0;
}

static char* get_unique_name(struct fingerprints_userstate* this) {
    int guess = 1;
    char *name = NULL;

    while (1) {
        asprintf(&name, "%s%d", FINGER_NAME_TEMPLATE, guess);
        if (is_unique(this, name))
            return name;
        guess++;
    }
}

static void write_state_thread(struct pthread_wrapper* thread, void* param) {
    struct fingerprints_userstate* this = (struct fingerprints_userstate*)param;

    pthread_mutex_lock(&this->lock);
    struct fingerprint_list* fingerprints = get_copy(this, this->fingerprints);
    pthread_mutex_unlock(&this->lock);

    mxml_node_t *tree = NULL;
    mxml_node_t *fps_node = NULL;

    tree = mxmlNewXML("1,0");
    fps_node = mxmlNewElement(tree, TAG_FINGERPRINTS);

    for (int i = 0; i < fingerprints->size(fingerprints); i++) {
        char str[256] = {0};
        mxml_node_t *fp_node = NULL;

        fp_node = mxmlNewElement(fps_node, TAG_FINGERPRINTS);

        struct fingerprint* fp = fingerprints->get(fingerprints, i);

        mxmlElementSetAttr(fp_node, ATTR_NAME, fp->get_name(fp));

        sprintf(str, "0x%x", fp->get_group_id(fp));
        mxmlElementSetAttr(fp_node, ATTR_GROUP_ID, str);
        memset(str, 0, sizeof(str));

        sprintf(str, "0x%x", fp->get_finger_id(fp));
        mxmlElementSetAttr(fp_node, ATTR_FINGER_ID, str);
        memset(str, 0, sizeof(str));

        sprintf(str, "0x%x", fp->get_device_id(fp));
        mxmlElementSetAttr(fp_node, ATTR_DEVICE_ID, str);
        memset(str, 0, sizeof(str));
    }

    char* dir = get_user_system_dir(this->user_id);

    sprintf(this->file_path, "%s/%s", dir, FINGERPRINT_FILE);

    FILE* fp = fopen(this->file_path, "w");
    mxmlSaveFile(tree, fp, MXML_NO_CALLBACK);
    fclose(fp);

    return;
}

static void schedule_write_state(struct fingerprints_userstate* this) {
    this->runner->start(this->runner, this);
}

static void add_fingerprint(struct fingerprints_userstate* this, int finger_id, int group_id) {
    pthread_mutex_lock(&this->lock);

    char* name = get_unique_name(this);

    struct fingerprint* fp = calloc(1, sizeof(struct fingerprint));
    fp->construct = construct_fingerprint;
    fp->destruct = destruct_fingerprint;
    fp->construct(fp, name, group_id, finger_id, 0);

    this->fingerprints->insert(this->fingerprints, fp);

    free(name);

    schedule_write_state(this);
    pthread_mutex_unlock(&this->lock);
}

static void remove_fingerprint(struct fingerprints_userstate* this, int finger_id) {
    pthread_mutex_lock(&this->lock);

    for (int i = 0; i < this->fingerprints->size(this->fingerprints); i++) {
        struct fingerprint* fp = this->fingerprints->get(this->fingerprints, i);
        if (fp->get_finger_id(fp) == finger_id) {
            this->fingerprints->erase(this->fingerprints, i);
            schedule_write_state(this);
        }
    }

    pthread_mutex_unlock(&this->lock);
}

static void rename_fingerprint(struct fingerprints_userstate* this, int finger_id, const char* name) {
    pthread_mutex_lock(&this->lock);

    for (int i = 0; i < this->fingerprints->size(this->fingerprints); i++) {
        struct fingerprint* fp = this->fingerprints->get(this->fingerprints, i);
        if (fp->get_finger_id(fp) == finger_id) {
            fp->set_name(fp, name);
            schedule_write_state(this);
        }
    }

    pthread_mutex_unlock(&this->lock);
}

static struct fingerprint_list* get_fingerprints(struct fingerprints_userstate* this) {
    pthread_mutex_lock(&this->lock);
    struct fingerprint_list* copy_list = get_copy(this, this->fingerprints);
    pthread_mutex_unlock(&this->lock);

    return copy_list;
}

void construct_fingerprints_userstate(struct fingerprints_userstate* this,
        int user_id) {
    this->fingerprints = _new(struct fingerprint_list, fingerprint_list);
    this->runner = _new(struct default_runnable, default_runnable);
    this->runner->runnable.run = write_state_thread;

    this->add_fingerprint = add_fingerprint;
    this->remove_fingerprint = remove_fingerprint;
    this->rename_fingerprint = rename_fingerprint;
    this->get_fingerprints = get_fingerprints;
    this->user_id = user_id;

    pthread_mutex_init(&this->lock, NULL);

    read_state_sync(this);
}

void destruct_fingerprints_userstate(struct fingerprints_userstate* this) {
    _delete(this->fingerprints);
    _delete(this->runner);

    this->add_fingerprint = NULL;
    this->remove_fingerprint = NULL;
    this->rename_fingerprint = NULL;
    this->get_fingerprints = NULL;
}
