/*
 *  Copyright (C) 2016, Zhang YanMing <yanmin.zhang@ingenic.com, jamincheung@126.com>
 *
 *  Ingenic QRcode SDK Project
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

#include <types.h>

struct uart_manager {
    int (*init)(uint8_t port, uint32_t baudrate, uint8_t date_bits,
            uint8_t stop_bits);
    int (*deinit)(uint8_t port);
    int (*write)(uint8_t port, const void* buf, uint32_t count,
            uint32_t timeout_ms);
    int (*read)(uint8_t port, void* buf, uint32_t count, uint32_t timeout_ms);
};
