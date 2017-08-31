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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <getopt.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <malloc.h>
#include <utils/log.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <fb/fb_manager.h>
#include <camera_v4l2/camera_v4l2_manager.h>

/*
 * Macro
 */
#define LOG_TAG                         "camera_v4l2"


#define DEFAULT_WIDTH                    320
#define DEFAULT_HEIGHT                   240
#define DEFAULT_BPP                      16
#define DEFAULT_NBUF                     1
#define DEFAULT_ACTION                   PREVIEW_PICTURE
#define DEFAULT_DOUBLE_CHANNEL           0
#define DEFAULT_CHANNEL                  CHANNEL_SEQUEUE_COLOR

#define MK_PIXEL_FMT(x)                                 \
        do{                                             \
            x.rbit_len = fbm->get_redbit_length();      \
            x.rbit_off = fbm->get_redbit_offset();      \
            x.gbit_len = fbm->get_greenbit_length();    \
            x.gbit_off = fbm->get_greenbit_offset();    \
            x.bbit_len = fbm->get_bluebit_length();     \
            x.bbit_off = fbm->get_bluebit_offset();     \
        }while(0)


typedef enum {
    CHANNEL_SEQUEUE_COLOR             = 0x00,
    CHANNEL_SEQUEUE_BLACK_WHITE       = 0x01
} chselect_m;


typedef enum {
    CAPTURE_PICTURE = 0,
    PREVIEW_PICTURE,
} action_m;

struct camera_v4l2_manager* cimm;
struct fb_manager* fbm;

struct _capt_op{
    action_m action;            // action capture or preview
    chselect_m channel;         // select operate ch for action
    char double_ch;             // channel num
    uint8_t* filename;             // picture save name when action is capture picture
    uint16_t *ppbuf;               // map lcd piexl buf
}capt_op;

static const char short_options[] = "c:phmn:rux:y:ds:";

static const struct option long_options[] = {
    { "help",       0,      NULL,           'h' },
    { "mmap",       0,      NULL,           'm' },
    { "nbuf",       1,      NULL,           'n' },
    { "read",       0,      NULL,           'r' },
    { "userp",      0,      NULL,           'u' },
    { "width",      1,      NULL,           'x' },
    { "height",     1,      NULL,           'y' },
    { "capture",    1,      NULL,           'c' },
    { "preview",    0,      NULL,           'p' },
    { "double",     0,      NULL,           'd' },
    { "select",     1,      NULL,           's' },
    { 0, 0, 0, 0 }
};

/*
 * Functions
 */
static void usage(FILE *fp, int argc, char *argv[])
{
    fprintf(fp,
             "\nUsage: %s [options]\n"
             "Options:\n"
             "-h | --help          Print this message\n"
             "-m | --mmap          Use memory mapped buffers\n"
             "-n | --nbuf          Request buffer numbers\n"
             "-r | --read          Use read() calls\n"
             "-u | --userp         Use application allocated buffers\n"
             "-x | --width         Capture width\n"
             "-y | --height        Capture height\n"
             "-c | --capture       Take picture\n"
             "-p | --preview       Preview picture to LCD\n"
             "-d | --double        Used double camera sensor\n"
             "-s | --select        Select operate channel, 0(color) or 1(black&white)\n"
             "\n", argv[0]);
}


static void frame_receive_cb(uint8_t* buf, uint32_t width, uint32_t height, uint8_t seq)
{
    int ret;
    uint8_t *yuvbuf = buf;
    uint8_t *rgbbuf = NULL;
    struct rgb_pixel_fmt pixel_fmt;

    rgbbuf = (uint8_t *)malloc(width * height * 3);
    if (!rgbbuf) {
        LOGE("malloc rgbbuf failed!!\n");
        return;
    }


    if (capt_op.double_ch != 1) {
        seq = capt_op.channel;
    }

    if (seq == capt_op.channel) {
        ret = cimm->yuv2rgb(yuvbuf, rgbbuf, width, height);
        if (ret < 0){
            LOGE("yuv 2 rgb fail, errno: %d\n", ret);
            free(rgbbuf);
            return;
        }

        if (capt_op.action == CAPTURE_PICTURE){
            ret = cimm->build_bmp(rgbbuf, width, height, (uint8_t*)capt_op.filename);
            if (ret < 0){
                LOGE("make bmp picutre fail, errno: %d\n",ret);
            }
        } else if (capt_op.action == PREVIEW_PICTURE) {
            MK_PIXEL_FMT(pixel_fmt);
            cimm->rgb2pixel(rgbbuf, capt_op.ppbuf, width, height, pixel_fmt);
            fbm->display();
        }
    }

    free(rgbbuf);
    return;
}

int main(int argc, char *argv[])
{
    struct capt_param_t capt_param;
    int ret;

    capt_param.width  = DEFAULT_WIDTH;
    capt_param.height = DEFAULT_HEIGHT;
    capt_param.bpp    = DEFAULT_BPP;
    capt_param.nbuf   = DEFAULT_NBUF;
    capt_param.io     = METHOD_MMAP;
    capt_param.fr_cb  = (frame_receive)frame_receive_cb;

    capt_op.double_ch = DEFAULT_DOUBLE_CHANNEL;
    capt_op.channel   = DEFAULT_CHANNEL;
    capt_op.action    = DEFAULT_ACTION;

    while(1) {
        int oc;

        oc = getopt_long(argc, argv, \
                         short_options, long_options, \
                         NULL);
        if(-1 == oc)
            break;

        switch(oc) {
        case 0:
            break;

        case 'h':
            usage(stdout, argc, argv);
            return 0;

        case 'm':
            capt_param.io = METHOD_MMAP;
            break;

        case 'r':
            capt_param.io = METHOD_READ;
            break;

        case 'u':
            capt_param.io = METHOD_USERPTR;
            break;

        case 'n':
            capt_param.nbuf = atoi(optarg);
            break;

        case 'x':
            capt_param.width = atoi(optarg);
            break;

        case 'y':
            capt_param.height = atoi(optarg);
            break;

        case 'c':
            capt_op.action = CAPTURE_PICTURE;
            capt_op.filename = (uint8_t*)optarg;
            break;

        case 'p':
            capt_op.action = PREVIEW_PICTURE;
            break;

        case 'd':
            capt_op.double_ch = 1;
            break;

        case 's':
            capt_op.channel = atoi(optarg);
            break;
        default:
            usage(stderr, argc, argv);
            LOGE("Invalid parameter %c.\n",oc);
            return -1;
        }
    }

    fbm = get_fb_manager();

    if (fbm->init() < 0) {
        LOGE("Failed to init fb_manager\n");
        return -1;
    }

    capt_op.ppbuf = (uint16_t *)fbm->get_fbmem();
    if (capt_op.ppbuf == NULL) {
        LOGE("Failed to get fbmem\n");
        return -1;
    }

    cimm = get_camera_v4l2_manager();

    ret = cimm->init(&capt_param);
    if (ret < 0) {
        LOGE("Failed to init camera manager\n");
        return -1;
    }

    cimm->start();
    if (ret < 0) {
        LOGE("Failed to start.\n");
        return -1;
    }

    if (capt_op.action == PREVIEW_PICTURE){
        while(getchar() != 'Q')
            LOGI("Enter 'Q' to exit\n");
    } else if (capt_op.action == CAPTURE_PICTURE) {
        sleep(1);
    }

    cimm->stop();
    cimm->deinit();
    return 0;
}
