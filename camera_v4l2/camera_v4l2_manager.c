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
#include <pthread.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#include <types.h>
#include <thread/thread.h>
#include <utils/log.h>
#include <utils/common.h>
#include <utils/yuv2bmp.h>
#include <utils/assert.h>
#include "cim/capture.h"

#include <camera_v4l2/camera_v4l2_manager.h>

#define LOG_TAG "camera_v4l2_manager"

#define DEFAULT_DEVICE "/dev/video0"

static frame_receive frame_receive_cb = NULL;

struct _camera_v4l2_op
{
    struct capture_t capt;
} camera_v4l2_op;

static pthread_mutex_t init_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t start_lock = PTHREAD_MUTEX_INITIALIZER;

static struct thread* thread;
static int init_count;
static int start_count;

static inline uint32_t  make_pixel(uint32_t r, uint32_t g,
                        uint32_t b, struct rgb_pixel_fmt fmt)
{
    uint32_t pixel = (uint32_t)(((r >> (8 - fmt.rbit_len)) << fmt.rbit_off)
                              | ((g >> (8 - fmt.gbit_len)) << fmt.gbit_off)
                              | ((b >> (8 - fmt.bbit_len)) << fmt.bbit_off));
    return pixel;
}

static void frame_process_cb(char* buf, uint32_t width,
                            uint32_t height, uint8_t seq)
{
    frame_receive_cb(buf, width, height, seq);
}

static void capt_loop(struct pthread_wrapper* pthread, void* param)
{
    int oldstate;

    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &oldstate);
    while(1){
        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &oldstate);
        v4l2_loop(&camera_v4l2_op.capt);
        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &oldstate);
    }

    pthread_exit(NULL);
}

static int camera_v4l2_init(struct capt_param_t *capt_p)
{
    int ret;

    camera_v4l2_op.capt.dev_name = DEFAULT_DEVICE;
    camera_v4l2_op.capt.width    = capt_p->width;
    camera_v4l2_op.capt.height   = capt_p->height;
    camera_v4l2_op.capt.bpp      = capt_p->bpp;
    camera_v4l2_op.capt.nbuf     = capt_p->nbuf;
    camera_v4l2_op.capt.io       = capt_p->io;
    frame_receive_cb             = capt_p->fr_cb;

    pthread_mutex_lock(&init_lock);

    if (init_count++ == 0) {
        ret = v4l2_open_device(&camera_v4l2_op.capt);
        if (ret < 0) {
            LOGE("Failed to open device\n");
            goto error;
        }

        ret = v4l2_init_device(&camera_v4l2_op.capt,
                                (frame_process)frame_process_cb);
        if (ret < 0) {
            LOGE("Failed to init device\n");
            goto error;
        }

        thread = _new(struct thread, thread);
        thread->runnable.run = capt_loop;
    }

    pthread_mutex_unlock(&init_lock);

    return 0;

error:
    pthread_mutex_unlock(&init_lock);
    return -1;
}

static int camera_v4l2_start()
{
    int ret;

    pthread_mutex_lock(&start_lock);

    if (start_count++ == 0) {
        ret = v4l2_start_capturing(&camera_v4l2_op.capt);
        if (ret < 0) {
            LOGE("Failed to start catpture\n");
            goto error;
        }

        thread->start(thread, NULL);
    }

    pthread_mutex_unlock(&start_lock);

    return 0;

error:
    pthread_mutex_unlock(&start_lock);
    return -1;
}


static int camera_v4l2_stop()
{
    int ret;

    pthread_mutex_lock(&start_lock);

    if (--start_count == 0) {
        ret = v4l2_stop_capturing(&camera_v4l2_op.capt);
        if (ret < 0) {
            LOGE("Failed to stop capturing\n");
            goto error;
        }

        thread->stop(thread);
    }

    pthread_mutex_unlock(&start_lock);

    return 0;

error:
    pthread_mutex_unlock(&start_lock);
    return -1;
}

static int camera_v4l2_is_start() {
    pthread_mutex_lock(&start_lock);
    int started = thread->is_running(thread);
    pthread_mutex_unlock(&start_lock);

    return started;
}

static int camera_v4l2_yuv2rgb(uint8_t* yuv, uint8_t* rgb,
                            uint32_t width, uint32_t height)
{
    assert_die_if(yuv == NULL, "yuv is NULL\n");
    assert_die_if(rgb == NULL, "rgb is NULL\n");

    yuv2rgb(yuv, rgb, width, height);
    return 0;
}

static int camera_v4l2_rgb2pixel(uint8_t* rgb, uint16_t* pbuf, uint32_t width,
                                 uint32_t height, struct rgb_pixel_fmt fmt)
{
    uint32_t pos,i,j,pixel;

    assert_die_if(pbuf == NULL, "pbuf is NULL\n");
    assert_die_if(rgb == NULL, "rgb is NULL\n");

    for (i = 0; i < height; i++) {
        for (j = 0; j < width; j++) {
            pos = i * width + j;
            pixel = make_pixel(rgb[pos*3+2], rgb[pos*3+1], rgb[pos*3], fmt);
            pbuf[pos] = pixel;
        }
    }

    return 0;
}

static int camera_v4l2_build_bmp(uint8_t* rgb, uint32_t width,
                            uint32_t height, uint8_t* filename)
{
    int ret;
    char *name;

    assert_die_if(rgb == NULL, "rgb is NULL\n");
    assert_die_if(filename == NULL, "filename is NULL\n");

    ret = asprintf(&name, "%s.bmp", (char*)filename);
    if (ret < 0) {
        LOGE("%s dev file name space malloc fail\n",__FUNCTION__);
        return -1;
    }

    ret = rgb2bmp(name, width, height, 24, rgb);
    if (ret != 0){
        LOGE("Failed to rgb2bmp\n");
        free(name);
        return -1;
    }

    free(name);
    return 0;
}

static int camera_v4l2_deinit()
{
    int ret;

    pthread_mutex_lock(&init_lock);

    if (--init_count == 0) {
        ret = v4l2_free_device(&camera_v4l2_op.capt);
        if (ret < 0) {
            LOGE("Failed to free device\n");
            goto error;
        }

        ret = v4l2_close_device(&camera_v4l2_op.capt);
        if (ret < 0) {
            LOGE("Failed to free device\n");
            goto error;
        }

        if (camera_v4l2_is_start())
            camera_v4l2_stop();
        _delete(thread);
    }

    pthread_mutex_unlock(&init_lock);

    return 0;

error:
    pthread_mutex_unlock(&init_lock);
    return -1;
}

static struct camera_v4l2_manager camera_v4l2_manager = {
    .init      = camera_v4l2_init,
    .start     = camera_v4l2_start,
    .stop      = camera_v4l2_stop,
    .is_start  = camera_v4l2_is_start,
    .yuv2rgb   = camera_v4l2_yuv2rgb,
    .rgb2pixel = camera_v4l2_rgb2pixel,
    .build_bmp = camera_v4l2_build_bmp,
    .deinit    = camera_v4l2_deinit
};

struct camera_v4l2_manager* get_camera_v4l2_manager(void){
    return &camera_v4l2_manager;
}
