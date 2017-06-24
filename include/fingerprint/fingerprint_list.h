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

#ifndef FINGERPRINT_LIST_H
#define FINGERPRINT_LIST_H

#include <utils/list.h>
#include <fingerprint/fingerprint.h>

struct fingerprint_list {
    void (*construct)(struct fingerprint_list* this);
    void (*destruct)(struct fingerprint_list* this);

    void (*insert)(struct fingerprint_list* this, struct fingerprint* fp);
    int (*size)(struct fingerprint_list* this);
    void (*erase)(struct fingerprint_list* this, int index);
    void (*erase_all)(struct fingerprint_list* this);
    int (*empty)(struct fingerprint_list* this);
    struct fingerprint* (*get)(struct fingerprint_list* this, int index);

    void (*dump)(struct fingerprint_list* this);
    struct list_head list;
    int the_size;
    pthread_mutex_t lock;
};

void construct_fingerprint_list(struct fingerprint_list* this);
void destruct_fingerprint_list(struct fingerprint_list* this);

#endif /* FINGERPRINT_LIST_H */
