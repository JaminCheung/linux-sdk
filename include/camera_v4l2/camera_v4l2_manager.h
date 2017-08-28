/*
 *  Copyright (C) 2017, Monk Su<rongjin.su@ingenic.com, MonkSu@outlook.com>
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
#ifndef _CAMERA_V4L2_MANAGER_H_
#define _CAMERA_V4L2_MANAGER_H_


typedef enum {
    METHOD_READ,
    METHOD_MMAP,
    METHOD_USERPTR,
} io_m;



typedef void (*frame_receive)(char* buf, uint32_t width, uint32_t height, uint32_t seq);

struct capt_param_t {
    uint32_t width;         // Resolution width(x)
    uint32_t height;        // Resolution height(y)
    uint32_t bpp;           // Bits Per Pixels
    uint32_t nbuf;          // buf queue
    frame_receive fr_cb;
    io_m         io;            // method of get image data
};

struct rgb_pixel_fmt {
    uint32_t rbit_len;
    uint32_t rbit_off;
    uint32_t gbit_len;
    uint32_t gbit_off;
    uint32_t bbit_len;
    uint32_t bbit_off;
};


struct camera_v4l2_manager {
    /**
     *  @brief   初始化camera devide
     *
     *  @param   capt_p - 传入获取图像的参数
     *
     */
    int (*init)(struct capt_param_t *capt_p);
    /**
     *  @brief   开启图像获取 ，目前只支持LCD预览，内部开启线程循环
     *
     */
    int (*start)(void);
    /**
     *  @brief   关闭图像获取 ，关闭线程
     *
     */
    int (*stop)(void);
    /**
     *  @brief   释放模块
     *
     */
    int (*yuv2rgb)(uint8_t* yuv, uint8_t* rgb, uint32_t width, uint32_t height);

    int (*rgb2pixel)(uint8_t* rgb, uint16_t* pbuf, uint32_t width, uint32_t height, struct rgb_pixel_fmt fmt);

    int (*build_bmp)(uint8_t* rgb, uint32_t width, uint32_t height, uint8_t* filename);

    int (*deinit)(void);
};

struct camera_v4l2_manager* get_camera_v4l2_manager(void);

#endif
