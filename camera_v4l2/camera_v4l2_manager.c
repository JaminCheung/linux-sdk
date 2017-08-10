/*
 *  Copyright (C) 2017, Monk Su<rongjin.su@ingenic.com, MonkSu@outlook.com>
 *
 *  Ingenic SDK Project
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
#include <pthread.h>
#include <types.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <utils/log.h>
#include "cim/capture.h"
#include "cim/yuv2bmp.h"
#include <camera_v4l2/camera_v4l2_manager.h>

#define LOG_TAG                          "camera_v4l2_manager"


#define DEFAULT_DEVICE                   "/dev/video0"


static frame_receive frame_receive_cb = NULL;

struct _camera_v4l2_op
{
    struct capture_t capt;
    pthread_t loop_capt_ptid;
}camera_v4l2_op;



pthread_mutex_t init_mutex = PTHREAD_MUTEX_INITIALIZER;         // init mutex lock


static inline uint32_t  make_pixel(uint32_t r, uint32_t g, uint32_t b)
{
    return (uint32_t)(((r >> 3) << 11) | ((g >> 2) << 5 | (b >> 3)));
}

static void frame_process_cb(char* buf, unsigned int width, unsigned int height, unsigned char seq)
{
    frame_receive_cb(buf, width, height, seq);
}

static void* capt_loop(void* p)
{
    int oldstate;

    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &oldstate);
    while(1){
        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &oldstate);
        main_loop(&camera_v4l2_op.capt);
        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &oldstate);
    }

    return NULL;
}

static int camera_v4l2_init(struct capt_param_t *capt_p)
{
    camera_v4l2_op.capt.dev_name = DEFAULT_DEVICE;
    camera_v4l2_op.capt.width    = capt_p->width;
    camera_v4l2_op.capt.height   = capt_p->height;
    camera_v4l2_op.capt.bpp      = capt_p->bpp;
    camera_v4l2_op.capt.nbuf     = capt_p->nbuf;
    camera_v4l2_op.capt.io       = capt_p->io;
    frame_receive_cb     = capt_p->fr_cb;

    if (pthread_mutex_trylock(&init_mutex) != 0) {
        printf("Init pthread tyr lock fail\n");
        exit(EXIT_FAILURE);
    }

    open_device(&camera_v4l2_op.capt);
    init_device(&camera_v4l2_op.capt, frame_process_cb);
}

static void camera_v4l2_start()
{
    start_capturing(&camera_v4l2_op.capt);

    if (pthread_create(&camera_v4l2_op.loop_capt_ptid, NULL, capt_loop, NULL) < 0) {
        LOGE("pthread create fail\n");
        exit(EXIT_FAILURE);
    }
}


static void camera_v4l2_stop()
{
    pthread_cancel(camera_v4l2_op.loop_capt_ptid);
    pthread_join(camera_v4l2_op.loop_capt_ptid, NULL);
    stop_capturing(&camera_v4l2_op.capt);
}



static int camera_v4l2_yuv2rgb(unsigned char* yuv, char* rgb, unsigned int width, unsigned int height)
{
    if (yuv == NULL || rgb == NULL)
        return -1;
    yuv2rgb(yuv, rgb, width, height);
    return 0;
}

static int camera_v4l2_rgb2pixel(unsigned char* rgb, unsigned short* pbuf, unsigned int width, unsigned int height)
{
    unsigned int pos,i,j,pixel;

    if (pbuf == NULL || rgb == NULL)
        return -1;

    for (i = 0; i < height; i++) {
        for (j = 0; j < width; j++) {
            pos = i * width + j;
            pixel = make_pixel(rgb[pos*3+2], rgb[pos*3+1], rgb[pos*3]);
            pbuf[pos] = pixel;
        }
    }

    return 0;
}

static int camera_v4l2_build_bmp(unsigned char* rgb, unsigned int width, unsigned int height, unsigned char* filename)
{
    int ret;
    char name[64] = {0};

    if (filename == NULL || strlen(filename) >= sizeof(name) || rgb == NULL)
        return -1;

    sprintf(name, "%s.bmp", filename);
    ret = rgb2bmp(name, width, height, 24, rgb);
    if (ret != 0)
        return -2;

    return 0;
}

static void camera_v4l2_deinit()
{
    free_device(&camera_v4l2_op.capt);
    close_device(&camera_v4l2_op.capt);
    pthread_mutex_unlock(&init_mutex);
}

static struct camera_v4l2_manager camera_v4l2_manager = {
    .init      = camera_v4l2_init,
    .start     = camera_v4l2_start,
    .stop      = camera_v4l2_stop,
    .yuv2rgb   = camera_v4l2_yuv2rgb,
    .rgb2pixel = camera_v4l2_rgb2pixel,
    .build_bmp = camera_v4l2_build_bmp,
    .deinit    = camera_v4l2_deinit
};

struct camera_v4l2_manager* get_camera_v4l2_manager(void){
    return &camera_v4l2_manager;
}