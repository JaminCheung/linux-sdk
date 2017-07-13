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

#ifndef FINGERPRINT_MANAGER_H
#define FINGERPRINT_MANAGER_H

struct fingerprint_manager {
    int (*init)(void);
    int (*deinit)(void);
    int (*enroll)(void);
    int (*authenticate)(void);
    int (*delete)(void);
    int (*cancel)(void);
};

struct fingerprint_manager* get_fingerprint_manager(void);

#endif /* FINGERPRINT_MANAGER_H */
