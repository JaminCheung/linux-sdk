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


#define SENSOR_ADDR_LENGTH   8

#if (SENSOR_ADDR_LENGTH == 8)
#define ADDR_END    0xff
#define VAL_END     0xff
#define ENDMARKER   {0xff, 0xff}
struct regval_list {
    unsigned char regaddr;
    unsigned char regval;
};
#elif (SENSOR_ADDR_LENGTH == 16)
#define ADDR_END    0xffff
#define VAL_END     0xff
#define ENDMARKER   {0xffff, 0xff}
struct regval_list {
    unsigned short regaddr;
    unsigned char regval;
};
#endif

#define SIZE   12
struct reg_msg {
    unsigned int write_size;
    unsigned int read_size;
    unsigned char reg_buf[SIZE];
};

/* timing parameters */
struct timing_param_t {
    unsigned long mclk_freq;
    unsigned int pclk_active_level; //0 for rising edge, 1 for falling edge
    unsigned int hsync_active_level;
    unsigned int vsync_active_level;
};

/* image parameters */
struct img_param_t {
    unsigned int width;      /* width */
    unsigned int height;     /* height */
    unsigned int bpp;        /* bits per pixel: 8/16/32 */
    unsigned int size;       /* image size */
};

struct camera_manager {
    /**
     *    Function: camera_init
     * Description: 摄像头初始化
     *       Input: 无
     *      Others: 必须优先调用 camera_init
     *      Return: 0 --> 成功, 非0 --> 失败
     */
    int (*camera_init)(void);

    /**
     *    Function: camera_deinit
     * Description: 摄像头释放
     *       Input: 无
     *      Others: 对应 camera_init, 不再使用camera时调用
     *      Return: 无
     */
    void (*camera_deinit)(void);

    /**
     *    Function: camera_read
     * Description: 读取摄像头采集数据,保存在 yuvbuf 指向的缓存区中
     *       Input:
     *          yuvbuf: 图像缓存区指针, 缓存区必须大于或等于读取的大小
     *            size: 读取数据大小,字节为单位, 一般设为 image_size
     *      Others: 在此函数中会断言yuvbuf是否等于NULL, 如果为NULL, 将推出程序
     *      Return: 返回实际读取到的字节数, 如果返回-1 --> 失败
     */
    int (*camera_read)(unsigned char *yuvbuf, unsigned int size);

    /**
     *    Function: set_img_param
     * Description: 设置控制器捕捉图像的分辨率和像素深度
     *       Input:
     *           img: struct img_param_t 结构指针, 指定图像的分辨率和像素深度
     *      Return: 0 --> 成功, -1 --> 失败
     */
    int (*set_img_param)(struct img_param_t *img);

    /**
     *    Function: set_timing_param
     * Description: 设置控制器时序, 包括mclk 频率、pclk有效电平、hsync有效电平、vsync有效电平
     *       Input:
     *          timing: struct timing_param_t 结构指针, 指定 mclk频率、pclk有效电平、hsync有效电平、vsync有效电平
     *                  在camera_init函数中分别初始化为:24000000、0、1、1, 为0是高电平有效, 为1则是低电平有效
     *      Others: 如果在camera_init函数内默认设置已经符合要求,则不需要调用
     *      Return: 0 --> 成功, -1 --> 失败
     */
    int (*set_timing_param)(struct timing_param_t *timing);

    /**
     *    Function: sensor_setup_addr
     * Description: 设置摄像头sensor的I2C地址 (在 probe 失败时,应该调用此函数设置sensor的I2C地址)
     *       Input:
     *              chip_addr: 摄像头sensor的I2C地址(不需要右移一位)
     *      Return: 0 --> 成功, -1 --> 失败
     */
    int (*sensor_setup_addr)(int chip_addr);

    /**
     *    Function: sensor_setup_regs
     * Description: 设置摄像头sensor的多个寄存器,用于初始化sensor
     *       Input:
     *          vals: struct regval_list 结构指针, 通常传入struct regval_list结构数组
     *      Others: 在开始读取图像数据时, 应该调用此函数初始化sensor寄存器
     *      Return: 0 --> 成功, -1 --> 失败
     */
    int (*sensor_setup_regs)(const struct regval_list *vals);

    /**
     *    Function: sensor_write_reg
     * Description: 设置摄像头sensor的单个寄存器
     *       Input:
     *          regaddr: 摄像头sensor的寄存器地址
     *           regval: 摄像头sensor寄存器的值
     *      Return: 0 --> 成功, -1 --> 失败
     */
    int (*sensor_write_reg)(unsigned int regaddr, unsigned char regval);

    /**
     *    Function: sensor_read_reg
     * Description: 读取摄像头sensor某个寄存器的知
     *       Input:
     *          regaddr: 摄像头sensor的寄存器地址
     *      Return: -1 --> 失败, 其他 --> 寄存器的值
     */
    unsigned char (*sensor_read_reg)(unsigned int regaddr);
};

/**
 *    Function: get_camera_manager
 * Description: 获取 camera_mananger 句柄
 *       Input: 无
 *      Return: 返回 camera_manager 结构体指针
 *      Others: 通过该结构体指针访问内部提供的方法
 */
struct camera_manager *get_camera_manager(void);

#endif /* CAMERA_MANAGER_H */
