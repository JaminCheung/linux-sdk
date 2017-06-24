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

#ifndef FINGERPRINT_USERSTATE_H
#define FINGERPRINT_USERSTATE_H

#include <fingerprint/fingerprint_list.h>

struct fingerprint_userstate {
    void (*construct)(struct fingerprint_userstate* this);
    void (*destruct)(struct fingerprint_userstate* this);

    void (*add_fingerprint)(struct fingerprint_userstate* this, int finger_id, int group_id);
    void (*remove_fingerprint)(struct fingerprint_userstate* this, int finger_id);
    void (*rename_fingerprint)(struct fingerprint_userstate* this, int finger_id, const char* name);
    struct fingerprint_list* (*get_fingerprints)(struct fingerprint_userstate* this);
};

void construct_fingerprint_userstate(struct fingerprint_userstate* this);
void destruct_fingerprint_userstate(struct fingerprint_userstate* this);

#endif /* FINGERPRINT_USERSTATE_H */
