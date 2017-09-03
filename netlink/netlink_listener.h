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

#ifndef NETLINK_LISTENER_H
#define NETLINK_LISTENER_H

#define NETLINK_FORMAT_ASCII 0
#define NETLINK_FORMAT_BINARY 1

#include <netlink/netlink_handler.h>

struct netlink_listener {
    int (*init)(int socket);
    int (*deinit)(void);
    int (*start)(void);
    int (*is_start)(void);
    int (*stop)(void);
    void (*register_handler)(struct netlink_handler *handler);
    void (*unregister_handler)(struct netlink_handler *handler);
};

struct netlink_listener* get_netlink_listener(void);

#endif /* NETLINK_LISTENER_H */
