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

#ifndef FINGERPRINT_H
#define FINGERPRINT_H

#include <limits.h>

#include <utils/list.h>

struct fingerprint {
    void (*construct)(struct fingerprint* this);
    void (*destruct)(struct fingerprint* this);

    char name[NAME_MAX];
    int group_id;
    int finger_id;
    int device_id;

    struct list_head node;
};

void construct_fingerprint(struct fingerprint* this);
void destruct_fingerprint(struct fingerprint* this);

#endif /* FINGERPRINT_H */
