/*************************************************************************
	> Filename: main.c
	>   Author: Wang Qiuwei
	>    Email: qiuwei.wang@ingenic.com / panddio@163.com
	> Datatime: Tue 13 Dec 2016 08:41:46 PM CST
 ************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/types.h>

#include <utils/log.h>
#include <utils/assert.h>
#include <libqrcode_api.h>

#include "gc2155.h"
#include "yuv2bmp.h"
/*
 * Macro
 */
#define PRINT_IMAGE_DATA  0
#define GC2155_ID  0x2155
#define VERSION(pid, ver) ((pid<<8)|(ver&0xFF))

#define LOG_TAG "test_camera"


/*
 * Functions
 */
#if (PRINT_IMAGE_DATA == 1)
static void print_data(unsigned char *pbuf, unsigned int image_size)
{
    int i;

    printf("unsigned char srcBuf[] = {");

    for(i = 0; i < image_size; i++) {
        if(i%46 == 0)
            printf("\n");
        printf("0x%02x,", *(pbuf + i));
    }

    printf("\n};\n");
    printf("\n----------- printf finish ----------\n");
}
#endif

int main(int argc, char *argv[])
{
    int cnt = 0;
    char filename[64] = "";
    unsigned char *yuvbuf = NULL;
    unsigned char *rgbbuf = NULL;
    unsigned char pid, vid;
    struct camera_img_param img;
    struct camera_timing_param timing;
    struct camera_manager *cm;

    /* 获取操作摄像头句柄 */
    cm = get_camera_manager();

    cm->camera_init();

    /* 设置分辨率和像素深度 */
    img.width  = 640;
    img.height = 480;
    img.bpp    = 16;
    img.size   = img.width * img.height * img.bpp / 2;
    cm->set_img_param(&img);


    /* 设置控制器时序和mclk频率 */
    timing.mclk_freq = 24000000;
    timing.pclk_active_level  = 1;
    timing.hsync_active_level = 1;
    timing.vsync_active_level = 1;
    cm->set_timing_param(&timing);

    /* Probe sensor ID */
    pid = cm->sensor_read_reg(0xf0);
    vid = cm->sensor_read_reg(0xf1);
    if (VERSION(pid, vid) != GC2155_ID) {
        LOGE("Sensor probe failed: pid = 0x%02x, vid = 0x%02x\n", pid, vid);
        exit(-1);
    }

    LOGE("Sensor probe successed: pid = 0x%02x, vid = 0x%02x\n", pid, vid);

    /* 配置 sensor 的寄存器 */
    cm->sensor_setup_regs(gc2155_init_regs);
    cm->sensor_setup_regs(gc2155_vga_regs);
    usleep(100);

    yuvbuf = (unsigned char *)malloc(img.size);
    if(!yuvbuf) {
        LOGE("Malloc yuvbuf failed: %s\n", strerror(errno));
        exit(-1);
    }

    rgbbuf = (unsigned char *)malloc(img.width * img.height * 3);
    if(!rgbbuf) {
        LOGE("Malloc rgbbuf failed: %s\n", strerror(errno));
        exit(-1);
    }

    while(1) {
        /* 开始获取图像 */
        cm->camera_read(yuvbuf, img.size);

        /* 处理图像*/
#if (PRINT_IMAGE_DATA == 1)
        print_data(yuvbuf, img.size);
#endif
        sprintf(filename, "test_%d.bmp", cnt);
        LOGI("filename = %s, cnt = %d\n", filename, cnt);

        yuv2rgb(yuvbuf, rgbbuf, img.width, img.height);
        rgb2bmp(filename, img.width, img.height, 24, rgbbuf);

        cnt++;
        if (cnt > 5)
            break;
    }

    free(rgbbuf);
    cm->camera_deinit();
    return 0;
}
