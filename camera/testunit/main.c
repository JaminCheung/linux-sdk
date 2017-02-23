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
#include <getopt.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/types.h>

#include <utils/log.h>
#include <utils/assert.h>
#include <libqrcode_api.h>

#include "sensor.h"
#include "gc2155.h"
#include "bf3703.h"
#include "yuv2bmp.h"
/*
 * Macro
 */
#define PRINT_IMAGE_DATA    0

#define GC2155_ID           0x2155
#define BF3703_ID           0x3703
#define VERSION(pid, ver)   ((pid<<8)|(ver&0xFF))

/*
 * Sensor的I2C地址
 */
#define GC2155_I2C_ADDR  0x3c
#define BF3703_I2C_ADDR  0x6e

#define DEFAULT_WIDTH    320
#define DEFAULT_HEIGHT   240
#define DEFAULT_BPP      16

#define LOG_TAG "test_camera"


static struct sensor_info sensor_info[] = {
    {GC2155, "gc2155"},
    {BF3703, "bf3703"},
};

static const char short_options[] = "hx:y:s:";
static const struct option long_options[] = {
	{ "help",       0,      NULL,           'h' },
	{ "width",      1,      NULL,           'x' },
	{ "height",     1,      NULL,           'y' },
	{ "sensor",     1,      NULL,           's' },
	{ 0, 0, 0, 0 }
};

/*
 * Functions
 */
static void usage(FILE *fp, int argc, char *argv[])
{
    int i;
	fprintf(fp,
			 "\nUsage: %s [options]\n"
			 "Options:\n"
			 "-h | --help          Get the help messages\n"
			 "-x | --width         Set image width\n"
			 "-y | --height        Set image height\n"
             "-s | --sensor        Select sensor\n"
			 "\n", argv[0]);

    fprintf(fp, "Support sensor:\n");
    for(i = 0; i < sizeof(sensor_info)/sizeof(sensor_info[0]); i++) {
        fprintf(fp,
             "%d: %s\n", sensor_info[i].sensor, sensor_info[i].name);
    }
    putchar('\n');
}

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
    int ret;
    int cnt = 0;
    char filename[64] = "";
    uint8_t *yuvbuf = NULL;
    uint8_t *rgbbuf = NULL;
    uint8_t pid, vid;
    enum sensor_list sensor = GC2155;
    const struct image_fmt *fmt;
    struct camera_img_param img;
    struct camera_timing_param timing;
    struct camera_manager *cm;

    /* 获取操作摄像头句柄 */
    cm = get_camera_manager();
    cm->camera_init();

    /* 默认的分辨率和像素深度 */
    img.width  = DEFAULT_WIDTH;
    img.height = DEFAULT_HEIGHT;
    img.bpp    = DEFAULT_BPP;

    while(1) {
        int oc;
        oc = getopt_long(argc, argv, \
                         short_options, long_options, \
                         NULL);
        if (-1 == oc)
            break;

        switch(oc) {
        case 0:
            break;
        case 'h':
            usage(stdout, argc, argv);
            exit(EXIT_SUCCESS);
            break;
        case 'x':
            img.width = atoi(optarg);
            break;
        case 'y':
            img.height = atoi(optarg);
            break;
        case 's':
            sensor = atoi(optarg);
            break;
        default:
            usage(stderr, argc, argv);
            exit(EXIT_FAILURE);
        }
    }

    switch(sensor) {
    case GC2155:
        fmt = select_image_fmt(&img.width, &img.height, gc2155_supported_fmts);

        /* Set sensor device addr */
        cm->sensor_setup_addr(GC2155_I2C_ADDR);

        /* Probe sensor ID */
        pid = cm->sensor_read_reg(0xf0);
        vid = cm->sensor_read_reg(0xf1);
        if (VERSION(pid, vid) != GC2155_ID)
            goto probe_error;
        LOGE("GC2155 probe successed: pid = 0x%02x, vid = 0x%02x\n", pid, vid);

        /* 配置 sensor 的寄存器 */
        cm->sensor_setup_regs(gc2155_init_regs);
        cm->sensor_setup_regs(fmt->regs_val);
        break;

    case BF3703:
        fmt = select_image_fmt(&img.width, &img.height, bf3703_supported_fmts);

        /* Set sensor device addr */
        cm->sensor_setup_addr(BF3703_I2C_ADDR);

        /* Probe sensor ID */
        pid = cm->sensor_read_reg(0xfc);
        vid = cm->sensor_read_reg(0xfd);
        if (VERSION(pid, vid) != BF3703_ID)
            goto probe_error;
        LOGE("BF3703 probe successed: pid = 0x%02x, vid = 0x%02x\n", pid, vid);

        /* Reset all registers */
        cm->sensor_write_reg(0x12, 0x80);

        /* 配置 sensor 的寄存器 */
        cm->sensor_setup_regs(bf3703_init_regs);
        cm->sensor_setup_regs(fmt->regs_val);
        break;

    default:
        fprintf(stderr, "No support sensor: %d\n", sensor);
        exit(EXIT_FAILURE);
    }

    putchar('\0'); // do nothing

    /* 设置控制器时序和mclk频率 */
    timing.mclk_freq = 24000000;
    timing.pclk_active_level  = 0;
    timing.hsync_active_level = 1;
    timing.vsync_active_level = 1;
    cm->set_timing_param(&timing);

    /* 设置分辨率和像素深度 */
    cm->set_img_param(&img);
    img.size = img.width * img.height * img.bpp / 2;

    yuvbuf = (uint8_t *)malloc(img.size);
    if (!yuvbuf) {
        LOGE("Malloc yuvbuf failed: %s\n", strerror(errno));
        exit(-1);
    }

    rgbbuf = (uint8_t *)malloc(img.width * img.height * 3);
    if (!rgbbuf) {
        LOGE("Malloc rgbbuf failed: %s\n", strerror(errno));
        exit(-1);
    }

    while(1) {
        /* 开始获取图像 */
        ret = cm->camera_read(yuvbuf, img.size);
        LOGE("Camera read return value:%d\n", ret);
        if (ret < 0) {
            /**
             * camera read failed
             */
            continue;
        }

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

    free(yuvbuf);
    free(rgbbuf);
    cm->camera_deinit();
    return 0;

probe_error:
    LOGE("Sensor probe failed: pid = 0x%02x, vid = 0x%02x\n", pid, vid);
    cm->camera_deinit();
    exit(EXIT_FAILURE);
}
