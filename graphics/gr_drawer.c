/*
 *  Copyright (C) 2016, Zhang YanMing <jamincheung@126.com>
 *
 *  Linux recovery updater
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

#include <stdlib.h>
#include <string.h>

#include <utils/log.h>
#include <utils/common.h>
#include <utils/assert.h>
#include <utils/png_decode.h>
#include <graphics/gr_drawer.h>
#include <graphics/font_10x18.h>
#include <fb/fb_manager.h>

#define LOG_TAG "gr_drawer"

#define FONT_PATH "/etc/font.png"

struct gr_font {
    uint32_t cwidth;
    uint32_t cheight;
    struct gr_surface* texture;
};

static struct gr_font* gr_font;
static struct fb_manager* fb_manager;
static uint32_t fb_width;
static uint32_t fb_height;
static uint32_t fb_bits_per_pixel;
static uint32_t fb_bytes_per_pixel;
static uint32_t fb_row_bytes;

static uint8_t gr_current_r = 255;
static uint8_t gr_current_g = 255;
static uint8_t gr_current_b = 255;

static int inited;
static uint32_t init_count;
static pthread_mutex_t init_lock = PTHREAD_MUTEX_INITIALIZER;

static int outside(uint32_t pos_x, uint32_t pos_y) {
    if ((pos_y >= fb_height) || (pos_x >= fb_width))
        return 1;

    return 0;
}

static uint32_t  make_pixel(uint8_t red, uint8_t green, uint8_t blue,
        uint8_t alpha) {
    uint32_t redbit_len = fb_manager->get_redbit_length();
    uint32_t redbit_off = fb_manager->get_redbit_offset();

    uint32_t greenbit_len = fb_manager->get_greenbit_length();
    uint32_t greenbit_off = fb_manager->get_greenbit_offset();

    uint32_t bluebit_len = fb_manager->get_bluebit_length();
    uint32_t bluebit_off = fb_manager->get_bluebit_offset();

    uint32_t alphabit_len = fb_manager->get_alphabit_length();
    uint32_t alphabit_off = fb_manager->get_alphabit_offset();

    uint32_t pixel = (uint32_t)(((red >> (8 - redbit_len)) << redbit_off)
            | ((green >> (8 - greenbit_len)) << greenbit_off)
            | ((blue >> (8 - bluebit_len)) << bluebit_off)
            | ((alpha >> (8 - alphabit_len)) << alphabit_off));

    return pixel;
}

static int draw_png(struct gr_surface* surface, uint32_t pos_x, uint32_t pos_y) {
    if (outside(pos_x, pos_y)) {
        LOGE("Image position out bound of screen\n");
        return -1;
    }

    uint8_t *buf = (uint8_t *) fb_manager->get_fbmem();

    for (int i = 0; i < surface->height; i++) {
        if ((i >= fb_height) || (i + pos_y > fb_height))
            break;

        for (int j = 0; j < surface->width; j++) {
            if ((j >= fb_width) || (j + pos_x > fb_width))
                break;

            uint8_t red = surface->raw_data[4 * (j + surface->width * i)];
            uint8_t green = surface->raw_data[4 * (j + surface->width * i) + 1];
            uint8_t blue = surface->raw_data[4 * (j + surface->width * i) + 2];
            uint8_t alpha = surface->raw_data[4 * (j + surface->width * i) + 3];

            uint32_t pos = (i + pos_y ) * fb_width + j + pos_x;
            uint32_t pixel = make_pixel(red, green, blue, alpha);

            for (int x = 0; x < fb_bytes_per_pixel; x++)
                buf[fb_bytes_per_pixel * pos + x] = pixel >> (fb_bits_per_pixel -
                        (fb_bytes_per_pixel -x) * 8);

        }
    }

    fb_manager->display();

    return 0;
}

static int blank(uint8_t blank) {
    return fb_manager->blank(blank);
}

static void set_pen_color(uint8_t red,
        uint8_t green, uint8_t blue) {

    gr_current_r = red;
    gr_current_g = green;
    gr_current_b = blue;
}

static void fill_screen(void) {
    uint8_t *buf = (uint8_t *) fb_manager->get_fbmem();

    uint32_t pixel = make_pixel(gr_current_r, gr_current_g, gr_current_b, 0);

    for (int i = 0; i < fb_height; i++) {
        for (int j = 0; j < fb_width; j++) {
            uint32_t pos = i * fb_width + j;

            for (int x = 0; x < fb_bytes_per_pixel; x++)
                buf[fb_bytes_per_pixel * pos + x] = pixel >> (fb_bits_per_pixel -
                        (fb_bytes_per_pixel -x) * 8);
        }
    }

    fb_manager->display();
}

static int fill_rect(uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2) {

    if (outside(x1, y1) || outside(x2 - 1, y2 - 1)) {
        LOGE("Rectangle size out bound of screen\n");
        return -1;
    }

    uint8_t *buf = (uint8_t *) fb_manager->get_fbmem() + y1 * fb_row_bytes +
            x1 * fb_bytes_per_pixel;

    uint32_t pixel = make_pixel(gr_current_r, gr_current_g, gr_current_b, 0);

    for (int y = y1; y < y2; y++) {
        uint8_t* px = buf;

        for (int x = x1; x < x2; x++) {
            for (int i = 0; i < fb_bytes_per_pixel; i++)
                px[i] = pixel >> (fb_bits_per_pixel
                        - (fb_bytes_per_pixel - i) * 8);

            px += fb_bytes_per_pixel;
        }

        buf += fb_row_bytes;
    }

    fb_manager->display();

    return 0;
}

static void text_blend(uint8_t* src_p, int src_row_bytes, uint8_t* dst_p,
        int dst_row_bytes, int width, int height) {

    uint32_t pixel = make_pixel(gr_current_r, gr_current_g, gr_current_b, 0);

    for (int i = 0; i < height; i++) {
        uint8_t* sx = src_p;
        uint8_t* px = dst_p;

        for (int j = 0; j < width; j++) {
            uint8_t alpha = *sx++;

            if (alpha == 255)
                for (int x = 0; x < fb_bytes_per_pixel; x++)
                    px[x] = pixel >> (fb_bits_per_pixel
                            - (fb_bytes_per_pixel -x) * 8);

                px += fb_bytes_per_pixel;
        }

        src_p += src_row_bytes;
        dst_p += dst_row_bytes;
    }
}

static int draw_text(uint32_t pos_x, uint32_t pos_y, const char* text,
        uint8_t bold) {
    assert_die_if(text == NULL, "text is NULL\n");

    uint8_t *buf = (uint8_t *) fb_manager->get_fbmem();

    struct gr_font *font = gr_font;
    uint32_t off;

    bold = bold && (font->texture->height != font->cheight);

    while((off = *text++)) {
        off -= 32;
        if (outside(pos_x, pos_y) ||
                outside(pos_x + font->cwidth - 1, pos_y + font->cheight - 1)) {
            LOGE("Text position out bound of screen\n");
            return -1;
        }

        if (off < 96) {

            uint8_t* src_p = font->texture->raw_data + (off * font->cwidth) +
                (bold ? font->cheight * font->texture->row_bytes : 0);

            uint8_t* dst_p = buf + pos_y * fb_row_bytes +
                    pos_x * fb_bytes_per_pixel;

            text_blend(src_p, font->texture->row_bytes, dst_p, fb_row_bytes,
                    font->cwidth, font->cheight);

        }

        pos_x += font->cwidth;
    }

    return 0;
}

static void display(void) {
    return fb_manager->display();
}

static uint32_t get_fb_width(void) {
    return fb_width;
}

static uint32_t get_fb_height(void) {
    return fb_height;
}

static void get_font_size(uint32_t* width, uint32_t* height) {
    *width = gr_font->cwidth;
    *height = gr_font->cheight;
}

static int init(void) {
    int error = 0;

    pthread_mutex_lock(&init_lock);

    if (init_count++ == 0) {
        fb_manager = get_fb_manager();
        if (fb_manager->init() < 0) {
            fb_manager = NULL;
            goto error;
        }

        fb_width = fb_manager->get_screen_width();
        fb_height = fb_manager->get_screen_height();
        fb_bits_per_pixel = fb_manager->get_bits_per_pixel();
        fb_row_bytes = fb_manager->get_row_bytes();
        fb_bytes_per_pixel = fb_bits_per_pixel / 8;

        gr_font = calloc(1, sizeof(struct gr_font));

        error = png_decode_font(FONT_PATH, &(gr_font->texture));
        if (!error) {
            // The font image should be a 96x2 array of character images.  The
            // columns are the printable ASCII characters 0x20 - 0x7f.  The
            // top row is regular text; the bottom row is bold.
            gr_font->cwidth = gr_font->texture->width / 96;
            gr_font->cheight = gr_font->texture->height / 2;

        } else {
            LOGW("Use default build-in font 10x18.\n");

            gr_font->texture = calloc(1, sizeof(*gr_font->texture));
            gr_font->texture->width = font.width;
            gr_font->texture->height = font.height;
            gr_font->texture->row_bytes = font.width;
            gr_font->texture->pixel_bytes = 1;

            uint8_t* bits = (uint8_t*) calloc(1, font.width * font.height);
            gr_font->texture->raw_data = (void *) bits;

            uint8_t data;
            uint8_t* in = font.rundata;
            while ((data = *in++)) {
                memset(bits, (data & 0x80) ? 0xff : 0, data & 0x7f);
                bits += (data & 0x7f);
            }

            gr_font->cwidth = font.cwidth;
            gr_font->cheight = font.cheight;
        }
    }

    pthread_mutex_unlock(&init_lock);

    return 0;

error:
    pthread_mutex_unlock(&init_lock);

    return -1;
}

static int deinit(void) {
    pthread_mutex_lock(&init_lock);

    if (--init_count == 0) {
        if (fb_manager)
            fb_manager->deinit();

        if (gr_font) {
            if (gr_font->texture)
                free(gr_font->texture);

            free(gr_font);
        }

        fb_manager = NULL;
        gr_font = NULL;
    }

    pthread_mutex_unlock(&init_lock);

    return 0;
}

static struct gr_drawer this = {
        .init = init,
        .deinit = deinit,
        .get_fb_height = get_fb_height,
        .get_fb_width = get_fb_width,
        .get_font_size = get_font_size,
        .set_pen_color = set_pen_color,
        .draw_png = draw_png,
        .draw_text = draw_text,
        .display = display,
        .blank = blank,
        .fill_screen = fill_screen,
        .fill_rect = fill_rect,
};

struct gr_drawer* get_gr_drawer(void) {
    return &this;
}
