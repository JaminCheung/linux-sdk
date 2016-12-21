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
#ifndef I2C_MANAGER_H
#define I2C_MANAGER_H

#include <libqrcode_api.h>

/*
 * Macros
 */
#define CHECK_I2C_FUNC(funcs, lable)                \
    do {                                            \
            assert_die_if(0 == (funcs & lable),     \
            #lable " i2c function is required.");   \
    } while (0);

#endif /* I2C_MANAGER_H */
