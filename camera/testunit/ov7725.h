/*************************************************************************
	> Filename: ov7725.h
	>   Author: Wang Qiuwei
	>    Email: qiuwei.wang@ingenic.com / panddio@163.com
	> Datatime: Tue 07 Mar 2017 10:24:04 AM CST
 ************************************************************************/

#ifndef _OV7725_H
#define _OV7725_H

static const struct camera_regval_list ov7725_init_regs[] = {
    {0x3d, 0x03},
    {0x17, 0x22},
    {0x18, 0xa4},
    {0x19, 0x07},
    {0x1a, 0xf0},
    {0x32, 0x00},
    {0x29, 0xa0},
    {0x2c, 0xf0},
    {0x2a, 0x00},
    {0x11, 0x07}, // 00/01/03/07 for 60/30/15/7.5fps - set to 7.5fps for VGA
    {0x42, 0x7f},
    {0x4d, 0x09},
    {0x63, 0xe0},
    {0x64, 0xff},
    {0x65, 0x20},
    {0x0c, 0x10}, // swap Y with UV
    {0x66, 0x00}, // swap U with V
    {0x67, 0x48},
    {0x13, 0xf0},
    {0x0d, 0x71}, // 51/61/71 for different AEC/AGC window
    {0x0f, 0xc5},
    {0x14, 0x11},
    {0x22, 0x7f}, // ff/7f/3f/1f for 60/30/15/7.5fps
    {0x23, 0x03}, // 01/03/07/0f for 60/30/15/7.5fps
    {0x24, 0x40},
    {0x25, 0x30},
    {0x26, 0xa1},
    {0x2b, 0x00}, // 00/9e for 60/50Hz
    {0x6b, 0xaa},
    {0x13, 0xff},

    {0x90, 0x05},
    {0x91, 0x01},
    {0x92, 0x03},
    {0x93, 0x00},
    {0x94, 0xb0},
    {0x95, 0x9d},
    {0x96, 0x13},
    {0x97, 0x16},
    {0x98, 0x7b},
    {0x99, 0x91},
    {0x9a, 0x1e},
    {0x9b, 0x08},
    {0x9c, 0x20},
    {0x9e, 0x81},
    {0xa6, 0x04},

    {0x7e, 0x0c},
    {0x7f, 0x16},
    {0x80, 0x2a},
    {0x81, 0x4e},
    {0x82, 0x61},
    {0x83, 0x6f},
    {0x84, 0x7b},
    {0x85, 0x86},
    {0x86, 0x8e},
    {0x87, 0x97},
    {0x88, 0xa4},
    {0x89, 0xaf},
    {0x8a, 0xc5},
    {0x8b, 0xd7},
    {0x8c, 0xe8},
    {0x8d, 0x20},
    ENDMARKER
};

static const struct camera_regval_list ov7725_qvga_regs[] = {
    {0x29, 0x50},
    {0x2c, 0x78},
    {0x2a, 0x00},

    {0x11, 0x03},
    {0x0d, 0x61},
    ENDMARKER
};

static const struct camera_regval_list ov7725_vga_regs[] = {
    {0x29, 0xa0},
    {0x2c, 0xf0},
    {0x2a, 0x00},

    {0x11, 0x01},
    {0x0d, 0x71},
    ENDMARKER
};

static const struct image_fmt ov7725_supported_fmts[] = {
    /* 分辨率从小到大排列 */
    IMAGE_FMT("QVGA", W_QVGA, H_QVGA, ov7725_qvga_regs),
    IMAGE_FMT("VGA", W_VGA, H_VGA, ov7725_vga_regs),
    IMAGE_FMT(0, 0, 0, 0),
};

#endif
