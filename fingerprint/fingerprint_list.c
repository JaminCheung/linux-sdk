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

#include <stdlib.h>

#include <utils/log.h>
#include <fingerprint/fingerprint_list.h>

#define LOG_TAG "fingerprint_list"

static void dump(struct fingerprint_list* this) {
    pthread_mutex_lock(&this->lock);

    LOGI("========================================\n");
    LOGI("Dump fingers.\n");

    struct fingerprint* fp;
    list_for_each_entry(fp, &this->list, node) {
        LOGI("----------------------------------------\n");
        LOGI("name:       %s\n", fp->get_name(fp));
        LOGI("finger id:  0x%x\n", fp->get_finger_id(fp));
        LOGI("group  id:  0x%x\n", fp->get_group_id(fp));
        LOGI("device id:  0x%x\n", fp->get_device_id(fp));
    }

    LOGI("========================================\n");

    pthread_mutex_unlock(&this->lock);
}

static void insert(struct fingerprint_list* this, struct fingerprint* new_fp) {
    struct fingerprint* fp = NULL;

    pthread_mutex_lock(&this->lock);

    list_for_each_entry(fp, &this->list, node) {
        if (fp->equal(fp, new_fp))
            goto out;
    }

    list_add_tail(&new_fp->node, &this->list);

out:
    pthread_mutex_unlock(&this->lock);
}

static void erase(struct fingerprint_list* this, int index) {
    struct list_head *pos, *next_pos;
    int i = 0;

    pthread_mutex_lock(&this->lock);

    if (!list_empty(&this->list)) {
        if (index > get_list_size_locked(&this->list) - 1)
            goto out;

        list_for_each_safe(pos, next_pos, &this->list) {
            struct fingerprint* fp = list_entry(pos, struct fingerprint, node);
            if (i++ == index) {
                list_del(&fp->node);
                free(fp);

                break;
            }
        }
    }

out:
    pthread_mutex_unlock(&this->lock);
}

static void erase_all(struct fingerprint_list* this) {
    struct list_head *pos, *next_pos;

    pthread_mutex_lock(&this->lock);

    list_for_each_safe(pos, next_pos, &this->list) {
        struct fingerprint* fp = list_entry(pos, struct fingerprint, node);

        list_del(&fp->node);
        free(fp);
    }

    pthread_mutex_unlock(&this->lock);
}


static int size(struct fingerprint_list* this) {

    pthread_mutex_lock(&this->lock);

    int size = this->the_size;

    pthread_mutex_unlock(&this->lock);

    return size;
}

static int empty(struct fingerprint_list* this) {

    pthread_mutex_lock(&this->lock);

    int empty = this->the_size == 0;

    pthread_mutex_unlock(&this->lock);

    return empty;
}

static struct fingerprint* get(struct fingerprint_list* this, int index) {
    struct fingerprint* fp = NULL;
    int i = 0;

    pthread_mutex_lock(&this->lock);

    if (!list_empty(&this->list)) {
        if (index > get_list_size_locked(&this->list) - 1)
            goto out;

        list_for_each_entry(fp, &this->list, node) {
            if (i++ == index)
                break;
        }
    }

out:
    pthread_mutex_unlock(&this->lock);

    return fp;
}

void construct_fingerprint_list(struct fingerprint_list* this) {
    this->insert = insert;
    this->erase = erase;
    this->erase_all = erase_all;
    this->empty = empty;
    this->size = size;
    this->get = get;
    this->the_size = 0;
    this->dump = dump;

    INIT_LIST_HEAD(&this->list);

    pthread_mutex_init(&this->lock, NULL);
}

void destruct_fingerprint_list(struct fingerprint_list* this) {
    pthread_mutex_destroy(&this->lock);
}
