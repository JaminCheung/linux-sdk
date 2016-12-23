/*
 *  Copyright (C) 2016, Wang Qiuwei <qiuwei.wang@ingenic.com, panddio@163.com>
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
#ifndef EFUSE_MANAGER_H
#define EFUSE_MANAGER_H

#include <libqrcode_api.h>

/*
 * Macros
 */
#define CMD_GET_CHIPID     _IOW('E', 0, uint32_t)
#define CMD_GET_RANDOM     _IOW('E', 1, uint32_t)
#define CMD_GET_USERID     _IOR('E', 2, uint32_t)
#define CMD_GET_PROTECTID  _IOR('E', 3, uint32_t)
#define CMD_WRITE          _IOR('E', 4, uint32_t)
#define CMD_READ           _IOR('E', 5, uint32_t)

/*
 * Struct
 */
struct efuse_wr_info {
    uint32_t seg_id;
    uint32_t data_length;
    uint32_t *buf;
};

#endif /* EFUSE_MANAGER_H */
