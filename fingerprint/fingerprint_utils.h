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

#ifndef FINGERPRINT_UTILS_H
#define FINGERPRINT_UTILS_H

#include <fingerprint/fingerprint_list.h>

struct fingerprint_utils {
    struct fingerprint_list* (*get_fingerprints_for_user)(int user_id);
    void (*add_fingerprint_for_user)(int finger_id, int user_id);
    void (*remove_fingerprint_id_for_user)(int finger_id, int user_id);
    void (*rename_fingerprint_for_user)(int finger_id, int user_id, const char* name);
};

struct fingerprint_utils* get_fingerprint_utils(void);

#endif /* FINGERPRINT_UTILS_H */
