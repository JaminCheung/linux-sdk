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
#ifndef _CAPTURE_H
#define _CAPTURE_H
#include <types.h>

/*
 * Struct
 */
typedef enum {
    IO_METHOD_READ,
    IO_METHOD_MMAP,
    IO_METHOD_USERPTR,
} io_method;


struct buffer {
    void *start;
    size_t length;
};

struct capture_t {
    char *dev_name;
    int fd;
    uint32_t width;
    uint32_t height;
    uint32_t bpp;
    uint32_t sizeimage;
    uint32_t nbuf;
    uint32_t count;

    io_method io;
    struct buffer *pbuf;
};

typedef void (*frame_process)(uint8_t* buf, uint32_t width, uint32_t height, uint8_t seq);

/*
 * Extern functions
 */

int v4l2_start_capturing(struct capture_t *capt);
int v4l2_stop_capturing(struct capture_t *capt);
int v4l2_test_framerate(struct capture_t *capt);
int v4l2_open_device(struct capture_t *capt);
int v4l2_close_device(struct capture_t *capt);
int v4l2_init_device(struct capture_t *capt, frame_process fp_cb);
int v4l2_free_device(struct capture_t *capt);
int v4l2_loop(struct capture_t *capt);

#endif
