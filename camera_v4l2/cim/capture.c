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
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <asm/types.h>
#include <linux/videodev2.h>
#include "jz_mem.h"
#include <utils/yuv2bmp.h>
#include "capture.h"
#include <sys/time.h>
#include <utils/log.h>
#include <types.h>

#define TRUE                            1
#define FALSE                           0

#define LOG_TAG                          "capture"


static frame_process frame_process_cb = NULL;

/*
 * Functions
 */
static int init_read(struct capture_t *capt)
{
    capt->pbuf = calloc(1, sizeof(struct buffer));

    if (!capt->pbuf) {
        LOGE("%s %s Out of memory pbuf.\n",capt->dev_name,__FUNCTION__);
        goto err_calloc_pbuf;
    }

    capt->pbuf[0].length = capt->sizeimage;
    capt->pbuf[0].start  = malloc(capt->sizeimage);

    if (!capt->pbuf[0].start) {
        LOGE("%s %s Out of memory start.\n",capt->dev_name,__FUNCTION__);
        goto err_calloc_start;
    }

    return 0;

err_calloc_start:
    free(capt->pbuf);
err_calloc_pbuf:
    return -1;
}

static int init_mmap(struct capture_t *capt)
{
    int i;
    struct v4l2_requestbuffers req;
    struct v4l2_buffer buf;

    memset(&req, 0, sizeof(req));
    req.count  = capt->nbuf;
    req.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;

    /* Request buffer */
    if (-1 == ioctl(capt->fd, VIDIOC_REQBUFS, &req)) {
        LOGE("%s Failed to ioctl: VIDIOC_REQBUFS %d %s\n",
                                    capt->dev_name, errno, strerror(errno));
        goto err_ioctl_reqbufs;
    }

    if (req.count < 1) {
        LOGE("%s Insufficient buffer memory\n", capt->dev_name);
        goto err_buff_insufficient;
    }

    capt->pbuf = calloc(req.count, sizeof(struct buffer));
    if (!capt->pbuf) {
        LOGE("%s %s Out of memory pbuf.\n",capt->dev_name,__FUNCTION__);
        goto err_calloc_pbuf;
    }

    /* Update the value of nbuf */
    capt->nbuf = req.count;
    LOGI("%s req.count: %d\n", __FUNCTION__,req.count);
    for (i = 0; i < req.count; i++) {
        memset(&buf, 0, sizeof(buf));
        buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index  = i;

        /* Get address and length of the frame buffer */
        if (-1 == ioctl(capt->fd, VIDIOC_QUERYBUF, &buf)) {
            LOGE("%s Failed to ioctl: VIDIOC_QUERYBUF %d %s\n",
                                        capt->dev_name, errno, strerror(errno));
            goto err_ioctl_querybuf;
        }

        LOGI("%s index %d offset: 0x%x lenth: %d\n",
                                    __FUNCTION__, i, buf.m.offset ,buf.length);
        /* memory map */
        capt->pbuf[i].length = buf.length;
        capt->pbuf[i].start  = mmap(NULL,
                                    buf.length,
                                    PROT_READ | PROT_WRITE,
                                    MAP_SHARED,
                                    capt->fd, buf.m.offset);

        if (MAP_FAILED == capt->pbuf[i].start) {
            LOGE("%s Failed to mmap %d %s\n",
                                    capt->dev_name, errno, strerror(errno));
            goto err_mmap;
        }
    }
    return 0;

err_mmap:
err_ioctl_querybuf:
    free(capt->pbuf);
err_calloc_pbuf:
err_buff_insufficient:
err_ioctl_reqbufs:
    return -1;
}

static int init_userp(struct capture_t *capt)
{
    int i;
    uint32_t page_size;
    uint32_t buffer_size;
    struct v4l2_requestbuffers req;

    page_size = getpagesize();
    buffer_size = (capt->sizeimage + page_size - 1) & ~(page_size - 1);

    memset(&req, 0, sizeof(req));
    req.count  = capt->nbuf;
    req.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_USERPTR;

    /* Request buffer */
    if (-1 == ioctl(capt->fd, VIDIOC_REQBUFS, &req)) {
        LOGE("%s Failed to ioctl: VIDIOC_REQBUFS %d %s\n",
                                            capt->dev_name, errno, strerror(errno));
        goto err_ioctl_reqbufs;
    }

    if (req.count < 1) {
        LOGE("%s Insufficient buffer memory\n", capt->dev_name);
        goto err_buff_insufficient;
    }

    capt->pbuf = calloc(req.count, sizeof(struct buffer));
    if (!capt->pbuf) {
        LOGE("%s %s Out of memory pbuf.\n",capt->dev_name,__FUNCTION__);
        goto err_calloc_pbuf;
    }

    /* Update the value of nbuf */
    capt->nbuf = req.count;

    for (i = 0; i < capt->nbuf; i++) {
        capt->pbuf[i].length = buffer_size;
        capt->pbuf[i].start  = jz_mem_alloc(128, buffer_size);

        if (!capt->pbuf[i].start) {
            LOGE("%s %s Out of memory start.\n",capt->dev_name,__FUNCTION__);
            goto err_calloc_start;
        }
    }
    return 0;

err_calloc_start:
    free(capt->pbuf);
err_calloc_pbuf:
err_buff_insufficient:
err_ioctl_reqbufs:
    return -1;
}


static void process_image(uint8_t* buf, uint32_t width, uint32_t height, uint32_t seq)
{
    if (frame_process_cb != NULL) {
        frame_process_cb(buf, width, height, seq);
    }
}


static int read_frame(struct capture_t *capt)
{
    struct v4l2_buffer buf;
    uint32_t i, ret;

    switch (capt->io) {
    case IO_METHOD_READ:
        ret = read(capt->fd, capt->pbuf[0].start, capt->pbuf[0].length);
        if (-1 == ret) {
            LOGE("%s Failed to Read frame %d %s\n",
                                    capt->dev_name, errno, strerror(errno));
            return -1;
        }

        process_image(capt->pbuf[0].start, capt->width, capt->height, 0);
        break;

    case IO_METHOD_MMAP:
        memset(&buf, 0, sizeof(buf));
        buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;

        while (-1 != ioctl(capt->fd, VIDIOC_DQBUF, &buf)) {
            assert(buf.index < capt->nbuf);
            process_image(capt->pbuf[buf.index].start, capt->width,
                                        capt->height, buf.sequence);
            if (-1 == ioctl(capt->fd, VIDIOC_QBUF, &buf)) {
                LOGE("%s Failed to ioctl: VIDIOC_QBUF %d %s\n",
                                    capt->dev_name, errno, strerror(errno));
                return -1;
            }
        }
        break;

    case IO_METHOD_USERPTR:
        {
            uint32_t page_size;
            uint32_t buffer_size;
            memset(&buf, 0, sizeof(buf));
            buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            buf.memory = V4L2_MEMORY_USERPTR;

            while (-1 != ioctl(capt->fd, VIDIOC_DQBUF, &buf)) {
                page_size   = getpagesize();
                buffer_size = (buf.length + page_size - 1) & ~(page_size - 1);

                for (i = 0; i < capt->nbuf; i++) {
                    if ((unsigned long)(capt->pbuf[i].start) == buf.m.userptr && \
                       (unsigned long)(capt->pbuf[i].length) == buffer_size)
                        break;
                }

                assert(i < capt->nbuf);
                process_image(capt->pbuf[i].start, capt->width,
                                    capt->height, buf.sequence);

                if (-1 == ioctl(capt->fd, VIDIOC_QBUF, &buf)) {
                    LOGE("%s Failed to ioctl: VIDIOC_QBUF %d %s\n",
                                    capt->dev_name, errno, strerror(errno));
                    return -1;
                }
            }
            break;
        }
    }

    return 0;
}

int v4l2_start_capturing(struct capture_t *capt)
{
    int i;
    enum v4l2_buf_type type;
    struct v4l2_buffer buf;

    switch (capt->io) {
    case IO_METHOD_READ:
        /* Nothing to do */
        break;

    case IO_METHOD_MMAP:
        /* Add to buffer queue */
        for (i = 0; i < capt->nbuf; i++) {
            memset(&buf, 0, sizeof(buf));
            buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            buf.memory = V4L2_MEMORY_MMAP;
            buf.index  = i;

            if (-1 == ioctl(capt->fd, VIDIOC_QBUF, &buf)) {
                LOGE("%s Failed to ioctl: VIDIOC_QBUF %d %s\n",
                                    capt->dev_name, errno, strerror(errno));
                return -1;
            }
        }

        type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if (-1 == ioctl(capt->fd, VIDIOC_STREAMON, &type)) {
            LOGE("%s Failed to ioctl: VIDIOC_STREAMON %d %s\n",
                                    capt->dev_name, errno, strerror(errno));
            return -1;
        }
        break;

    case IO_METHOD_USERPTR:
        /* Add to buffer queue */
        for (i = 0; i < capt->nbuf; i++) {
            memset(&buf, 0, sizeof(buf));
            buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            buf.memory = V4L2_MEMORY_USERPTR;
            buf.index  = i;
            buf.length = capt->pbuf[i].length;
            buf.m.userptr = (unsigned long)capt->pbuf[i].start;

            if (-1 == ioctl(capt->fd, VIDIOC_QBUF, &buf)) {
                LOGE("%s Failed to ioctl: VIDIOC_QBUF %d %s\n",
                                    capt->dev_name, errno, strerror(errno));
                return -1;
            }
        }

        type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if (-1 == ioctl(capt->fd, VIDIOC_STREAMON, &type)) {
            LOGE("%s Failed to ioctl: VIDIOC_STREAMON %d %s\n",
                                    capt->dev_name, errno, strerror(errno));
            return -1;
        }
        break;
    }

    return 0;
}

int v4l2_stop_capturing(struct capture_t *capt)
{
    enum v4l2_buf_type type;

    switch (capt->io) {
    case IO_METHOD_READ:
        /* Nothing to do */
        break;

    case IO_METHOD_MMAP:
    case IO_METHOD_USERPTR:
        type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if (-1 == ioctl(capt->fd, VIDIOC_STREAMOFF, &type)) {
            LOGE("%s Failed to ioctl: VIDIOC_STREAMOFF %d %s\n",
                                    capt->dev_name, errno, strerror(errno));
            return -1;
        }
        break;
    }

    return 0;
}

int v4l2_open_device(struct capture_t *capt)
{
    struct stat st;

    if (-1 == stat(capt->dev_name, &st)) {
        LOGE("%s Cannot identify %d %s\n",
                            capt->dev_name, errno, strerror(errno));
        return -1;
    }

    if (!S_ISCHR(st.st_mode)) {
        LOGE("%s is no device.\n", capt->dev_name);
        return -1;
    }

    capt->fd = open(capt->dev_name, O_RDWR | O_NONBLOCK, 0);

    if (capt->fd < 0) {
        LOGE("%s Cannot open. %d %s\n",
                            capt->dev_name, errno, strerror(errno));
        return -1;
    }

    return 0;
}

int v4l2_close_device(struct capture_t *capt)
{
    if (-1 == close(capt->fd)) {
        LOGE("%s Cannot close. %d %s\n",
                            capt->dev_name, errno, strerror(errno));
        return -1;
    }
    capt->fd = -1;

    free(capt->pbuf);

    return 0;
}

int v4l2_init_device(struct capture_t *capt, frame_process fp_cb)
{
    int ret;
    uint32_t min;
    struct v4l2_capability cap;
    struct v4l2_cropcap cropcap;
    struct v4l2_crop crop;
    struct v4l2_format fmt;

    if (fp_cb != NULL) {
        frame_process_cb = fp_cb;
    }

    if (-1 == ioctl(capt->fd, VIDIOC_QUERYCAP, &cap)) {
        LOGE("%s Failed to ioctl: VIDIOC_QUERYCAP %d %s\n",
                                capt->dev_name, errno, strerror(errno));
        return -1;
    }

    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
        LOGE("%s is no video capture device.\n", capt->dev_name);
        return -1;
    }

    switch (capt->io) {
    case IO_METHOD_READ:
        if (!(cap.capabilities & V4L2_CAP_READWRITE)) {
            LOGE("%s does not support read i/o.\n", capt->dev_name);
            return -1;
        }
        break;

    case IO_METHOD_MMAP:
    case IO_METHOD_USERPTR:
        if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
            LOGE("%s does not support streaming i/o.\n", capt->dev_name);
            return -1;
        }
        break;
    }

    /* Select video input, video standard and tune here */
    memset(&cropcap, 0, sizeof(cropcap));
    cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (0 == ioctl(capt->fd, VIDIOC_CROPCAP, &cropcap)) {
        crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        crop.c = cropcap.defrect; /* reset to default */

        if (-1 == ioctl(capt->fd, VIDIOC_S_CROP, &crop)) {
            LOGE("%s Failed to ioctl: VIDIOC_S_CROP %d %s\n",
                            capt->dev_name, errno, strerror(errno));
        }
    } else {
        /* Errors ignored */
    }

    memset(&fmt, 0, sizeof(fmt));
    fmt.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width       = capt->width;
    fmt.fmt.pix.height      = capt->height;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
    fmt.fmt.pix.field       = V4L2_FIELD_NONE; //V4L2_FIELD_INTERLACED;

    if (-1 == ioctl(capt->fd, VIDIOC_S_FMT, &fmt)) {
        LOGE("%s Failed to ioctl: VIDIOC_S_FMT %d %s\n",
                            capt->dev_name, errno, strerror(errno));
        return -1;
    }

    /* Note VIDIOC_S_FMT may change width and height */
    min = fmt.fmt.pix.width * 2;
    if (fmt.fmt.pix.bytesperline < min)
        fmt.fmt.pix.bytesperline = min;

    min = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height;
    if (fmt.fmt.pix.sizeimage < min)
        fmt.fmt.pix.sizeimage = min;

    /* Save image size */
    capt->sizeimage = min;

    capt->width  = fmt.fmt.pix.width;
    capt->height = fmt.fmt.pix.height;

    LOGI("==> %s L%d: sizeimage = 0x%x, width = %d, height = %d\n", \
        __FUNCTION__, __LINE__, capt->sizeimage, capt->width, capt->height);
    switch (capt->io) {
    case IO_METHOD_READ:
        ret = init_read(capt);
        if (ret < 0) {
            return -1;
        }
        break;

    case IO_METHOD_MMAP:
        ret = init_mmap(capt);
        if (ret < 0) {
            return -1;
        }
        break;

    case IO_METHOD_USERPTR:
        ret = init_userp(capt);
        if (ret < 0) {
            return -1;
        }
        break;
    }

    return 0;
}

int v4l2_free_device(struct capture_t *capt)
{
    int i;

    switch (capt->io) {
    case IO_METHOD_READ:
        free(capt->pbuf[0].start);
        break;

    case IO_METHOD_MMAP:
        for (i = 0; i < capt->nbuf; i++) {
            if (-1 == munmap(capt->pbuf[i].start, \
                            capt->pbuf[i].length)) {
                LOGE("%s Failed to munmap %d %s\n",
                                capt->dev_name, errno, strerror(errno));
                return -1;
            }
        }

        break;

    case IO_METHOD_USERPTR:
        jz_free_alloc_mem();
        break;
    }

    return 0;
}

int v4l2_loop(struct capture_t *capt)
{
    fd_set fds;
    struct timeval tv;
    int ret;
    int count = 1;

    while (count--) {
        FD_ZERO(&fds);
        FD_SET(capt->fd, &fds);

        /* Select time */
        tv.tv_sec  = 2;
        tv.tv_usec = 0;

        ret = select(capt->fd + 1, &fds, NULL, NULL, &tv);
        if (ret <= 0) {
            LOGE("%s Failed to select %d %s\n",
                                capt->dev_name, errno, strerror(errno));
            continue;
        }
        ret = read_frame(capt);
        if (ret < 0) {
            LOGE("Failed to read frame.\n");
        }
    }

    return 0;
}