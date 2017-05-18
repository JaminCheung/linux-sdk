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
#ifndef CAMERA_MANAGER_H
#define CAMERA_MANAGER_H

#include <ingenic_api.h>

/*
 * Ioctl Commands
 */
#define IOCTL_READ_REG            0
#define IOCTL_WRITE_REG           1
#define IOCTL_READ_EEPROM         2
#define IOCTL_WRITE_EEPROM        3
#define IOCTL_SET_ADDR            4
#define IOCTL_SET_CLK             5

#define IOCTL_SET_IMG_FORMAT      8  //arg type: enum imgformat
#define IOCTL_SET_TIMING_PARAM    9  //arg type: timing_param_t
#define IOCTL_SET_IMG_PARAM      10  //arg type: img_param_t
#define IOCTL_GET_FRAME          11
#define IOCTL_GET_FRAME_BLOCK    12

/*
 * Struct
 */
#define SIZE   5
struct reg_msg {
    uint32_t write_size;
    uint32_t read_size;
    uint8_t reg_buf[SIZE];
};

#endif /* CAMERA_MANAGER_H */
