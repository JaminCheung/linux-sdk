/*
 *  Copyright (C) 2017, Zhang YanMing <yanmin.zhang@ingenic.com, jamincheung@126.com>
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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdlib.h>
#include <linux/kd.h>

#include <utils/log.h>
#include <utils/assert.h>
#include <fb/fb_manager.h>

#define LOG_TAG "fb_manager"

const static char* prefix_fb_dev = "/dev/fb0";
const static char* prefix_fb_bak_dev = "/dev/graphics/fd0";
const static char* prefix_vt_dev = "/dev/tty0";

static int fd;
static int vt_fd;
static uint8_t fb_count;
static uint8_t fb_index;
static uint8_t *fbmem;
static uint8_t *fb_curmem;
static uint32_t screen_size;

static struct fb_fix_screeninfo fb_fixinfo;
static struct fb_var_screeninfo fb_varinfo;

static int inited;
static pthread_mutex_t init_lock = PTHREAD_MUTEX_INITIALIZER;

static void dump(void) {
    LOGI("==========================\n");
    LOGI("Dump fb info\n");
    LOGI("Screen count:  %u\n", fb_count);
    LOGI("Screen size:   %u\n", screen_size);
    LOGI("Width:         %u\n", fb_varinfo.xres);
    LOGI("Length:        %u\n", fb_varinfo.yres);
    LOGI("BPP:           %u\n", fb_varinfo.bits_per_pixel);
    LOGI("Row bytes:     %u\n", fb_fixinfo.line_length);
    LOGI("Red offset:    %u\n", fb_varinfo.red.offset);
    LOGI("Red length:    %u\n", fb_varinfo.red.length);
    LOGI("Green offset:  %u\n", fb_varinfo.green.offset);
    LOGI("Green length:  %u\n", fb_varinfo.green.length);
    LOGI("Blue offset:   %u\n", fb_varinfo.blue.offset);
    LOGI("Blue length:   %u\n", fb_varinfo.blue.length);
    LOGI("Alpha offset:  %u\n", fb_varinfo.transp.offset);
    LOGI("Alpha length:  %u\n", fb_varinfo.transp.length);
    LOGI("==========================\n");
}

static int init(void) {
    pthread_mutex_lock(&init_lock);
    if (inited == 1) {
        LOGE("fb manager already init\n");
        return 0;
    }

    fd = open(prefix_fb_dev, O_RDWR | O_SYNC);
    if (fd < 0) {
        LOGW("Failed to open %s, try %s\n", prefix_fb_dev, prefix_fb_bak_dev);

        fd = open(prefix_fb_bak_dev, O_RDWR | O_SYNC);
        if (fd < 0) {
            LOGE("Failed to open %s: %s\n", prefix_fb_bak_dev, strerror(errno));
            goto error;
        }
    }

    if (ioctl(fd, FBIOGET_VSCREENINFO, &fb_varinfo) < 0) {
        LOGE("Failed to get screen var info: %s\n", strerror(errno));
        goto error;
    }

    if(ioctl(fd, FBIOGET_FSCREENINFO, &fb_fixinfo) < 0) {
        LOGE("Failed to get screen fix info: %s\n", strerror(errno));
        goto error;
    }

    fbmem = (uint8_t*) mmap(0, fb_fixinfo.smem_len, PROT_READ | PROT_WRITE,
            MAP_SHARED, fd, 0);
    if (fbmem == MAP_FAILED) {
        LOGE("Failed to mmap frame buffer: %s\n", strerror(errno));
        goto error;
    }

    memset(fbmem, 0, fb_fixinfo.smem_len);

    fb_count = fb_fixinfo.smem_len / (fb_varinfo.yres
            * fb_fixinfo.line_length);

    screen_size = fb_fixinfo.line_length * fb_varinfo.yres;

    fb_curmem = (uint8_t *) calloc(1, fb_fixinfo.line_length
            * fb_varinfo.yres);

    vt_fd = open(prefix_vt_dev, O_RDWR | O_SYNC);
    if (vt_fd < 0) {
        LOGW("Failed to open %s: %s\n", prefix_vt_dev, strerror(errno));
    } else if (ioctl(vt_fd, KDSETMODE, (void*)KD_GRAPHICS) < 0) {
        LOGE("Failed to set KD_GRAPHICS on %s: %s\n", prefix_vt_dev,
                strerror(errno));

        ioctl(vt_fd, KDSETMODE, (void*) KD_TEXT);

        goto error;
    }

    inited = 1;

    pthread_mutex_unlock(&init_lock);

    return 0;

error:
    if (fd > 0)
        close(fd);
    if (vt_fd > 0)
        close(vt_fd);
    if (fb_curmem)
        free(fb_curmem);

    pthread_mutex_unlock(&init_lock);

    return -1;
}

static int deinit(void) {
    pthread_mutex_lock(&init_lock);
    if (inited == 0) {
        LOGE("fb manager already deinit\n");
        pthread_mutex_unlock(&init_lock);
        return 0;
    }

    inited = 0;

    if (munmap(fbmem, fb_fixinfo.smem_len) < 0) {
        LOGE("Failed to mumap frame buffer: %s\n", strerror(errno));
        goto error;
    }

    if (fb_curmem)
        free(fb_curmem);

    close(fd);
    fd = -1;

    ioctl(vt_fd, KDSETMODE, (void*) KD_TEXT);
    close(vt_fd);
    vt_fd = -1;

    pthread_mutex_unlock(&init_lock);

    return 0;

error:
    pthread_mutex_unlock(&init_lock);

    return -1;
}

static int set_displayed_fb(uint8_t index) {
    assert_die_if(index > fb_count, "Invalid frame buffer index\n");

    fb_varinfo.yoffset = index * fb_varinfo.yres;
    if (ioctl(fd, FBIOPAN_DISPLAY, &fb_varinfo) < 0) {
        LOGE("Failed to set displayed frame buffer: %s\n", strerror(errno));
        return -1;
    }

    fb_index = index;

    return 0;
}

static void display(void) {
    if (fb_count > 1) {
        if (fb_index >= fb_count)
            fb_index = 0;

        memcpy(fbmem + fb_index * screen_size, fb_curmem, screen_size);

        set_displayed_fb(fb_index);

        fb_index++;

    } else {
        memcpy(fbmem, fb_curmem, screen_size);
    }
}

static int blank(uint8_t blank) {
    int error = 0;

    error = ioctl(fd, FBIOBLANK, blank ? FB_BLANK_POWERDOWN : FB_BLANK_UNBLANK);
    if (error < 0) {
        LOGE("Failed to blank fb: %s\n", strerror(errno));
        return -1;
    }

    return 0;
}

static uint8_t* get_fbmem(void) {
    return fb_curmem;
}

static uint32_t get_screen_size(void) {
    return screen_size;
}

static uint32_t get_screen_height(void) {
    return fb_varinfo.yres;
}

static uint32_t get_screen_width(void) {
    return fb_varinfo.xres;
}

static uint32_t get_redbit_offset(void) {
    return fb_varinfo.red.offset;
}

static uint32_t get_redbit_length(void) {
    return fb_varinfo.red.length;
}

static uint32_t get_greenbit_offset(void) {
    return fb_varinfo.green.offset;
}

static uint32_t get_greenbit_length(void) {
    return fb_varinfo.green.length;
}

static uint32_t get_bluebit_offset(void) {
    return fb_varinfo.blue.offset;
}

static uint32_t get_bluebit_length(void) {
    return fb_varinfo.blue.length;
}

static uint32_t get_alphabit_offset(void) {
    return fb_varinfo.transp.offset;
}

static uint32_t get_alphabit_length(void) {
    return fb_varinfo.transp.length;
}

static uint32_t get_bits_per_pixel(void) {
    return fb_varinfo.bits_per_pixel;
}

static uint32_t get_row_bytes(void) {
    return fb_fixinfo.line_length;
}

static struct fb_manager this = {
        .init = init,
        .deinit = deinit,
        .display = display,
        .blank = blank,
        .dump = dump,
        .get_fbmem = get_fbmem,
        .get_screen_size = get_screen_size,
        .get_screen_height = get_screen_height,
        .get_screen_width = get_screen_width,

        .get_redbit_offset = get_redbit_offset,
        .get_redbit_length = get_redbit_length,

        .get_greenbit_offset = get_greenbit_offset,
        .get_greenbit_length = get_greenbit_length,

        .get_bluebit_offset = get_bluebit_offset,
        .get_bluebit_length = get_bluebit_length,

        .get_alphabit_offset = get_alphabit_offset,
        .get_alphabit_length = get_alphabit_length,

        .get_bits_per_pixel = get_bits_per_pixel,
        .get_row_bytes = get_row_bytes,
};

struct fb_manager* get_fb_manager(void) {
    return &this;
}
