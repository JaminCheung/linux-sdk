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
#include <stdarg.h>
#include <string.h>
#include <pthread.h>

#include <utils/log.h>
#include <utils/assert.h>
#include <utils/file_ops.h>
#include <utils/common.h>
#include <utils/png_decode.h>
#include <graphics/gui.h>

#define LOG_TAG "gui"

#define PROGRESS_SPACE_TO_TIPS  26

#define kMaxCols   96
#define kMaxRows   96

static const char* prefix_image_logo_path = "/res/image/logo.png";
static const char* prefix_image_progress_path = "/res/image/progress_";
static const char* prefix_stage_updating = "Updating...";
static const char* prefix_stage_update_success = "Update Success";
static const char* prefix_stage_update_failure = "Update Failed";

static uint32_t char_width;
static uint32_t char_height;
static uint32_t progress_width;
static uint32_t progress_height;

static int text_rows;
static int text_cols;
static int text_top;
static int text_row;
static int text_col;
static char text[kMaxRows][kMaxCols];

static struct gr_drawer* gr_drawer;
static pthread_mutex_t progress_lock;
static pthread_cond_t progress_cond;
static uint8_t start_progress;
static struct gr_surface** surface_progress;
static uint32_t frame_count;

static int load_progress_image(void) {
    char buf[256] = {0};

    /*
     * Get progress frame count
     */
    for (int i = 0;;i++) {
        sprintf(buf, "%s%02d.png", prefix_image_progress_path, i);
        if (file_exist(buf) < 0) {
            frame_count = i;
            break;
        }
    }

    if (frame_count == 0)
        return -1;

    surface_progress = calloc(frame_count, sizeof(int));
    for (int i = 0; i < frame_count; i++) {
        memset(buf, 0, sizeof(buf));
        sprintf(buf, "%s%02d.png", prefix_image_progress_path, i);
        if (file_exist(buf) == 0) {
            if (png_decode_image(buf, &surface_progress[i]) < 0) {
                LOGE("Failed to decode image: %s\n", buf);
                return -1;
            }
        }
    }

    return 0;
}

static int show_log(struct gui* this, const char* fmt, ...) {
    if (!g_data.has_fb)
        return 0;

    gr_drawer->set_pen_color(gr_drawer, 0x0, 0xff, 0x0);

    char buf[256];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, 256, fmt, ap);
    va_end(ap);

    if (text_rows > 0 && text_cols > 0) {
        char *ptr;
        for (ptr = buf; *ptr != '\0'; ++ptr) {
            if (*ptr == '\n' || text_col >= text_cols) {
                text[text_row][text_col] = '\0';
                text_col = 0;
                text_row = (text_row + 1) % text_rows;
                if (text_row == text_top) text_top = (text_top + 1) % text_rows;
            }
            if (*ptr != '\n') text[text_row][text_col++] = *ptr;
        }
        text[text_row][text_col] = '\0';

        int row = (text_top+text_rows-1) % text_rows;
        for (int ty = gr_drawer->get_fb_height(gr_drawer) - char_height, count = 0;
             ty > 2 && count < text_rows;
             ty -= char_height, ++count) {

            gr_drawer->draw_text(gr_drawer, 4, ty, text[row], 1);

            --row;

            if (row < 0)
                row = text_rows-1;
        }

        gr_drawer->display(gr_drawer);
    }

    return 0;
}

static void *progress_loop(void* param) {

    for (;;) {
        for (int i = 0; i < frame_count; i++) {

            pthread_mutex_lock(&progress_lock);
            while (!start_progress)
                pthread_cond_wait(&progress_cond, &progress_lock);
            pthread_mutex_unlock(&progress_lock);

            progress_width = surface_progress[i]->width;
            progress_height = surface_progress[i]->height;

            uint32_t pos_x = (gr_drawer->get_fb_width(gr_drawer)
                    - progress_width) / 2;

            uint32_t pos_y = (gr_drawer->get_fb_height(gr_drawer)
                    - (char_height + progress_height
                            + PROGRESS_SPACE_TO_TIPS)) / 2 + char_height + PROGRESS_SPACE_TO_TIPS;

            if (gr_drawer->draw_png(gr_drawer, surface_progress[i], pos_x, pos_y)
                    < 0) {
                LOGW("Failed to draw png image number: %d\n", i);
                continue;
            }

            msleep(200);

        }
    }

    return NULL;
}

static int start_show_progress(struct gui* this) {
    if (!g_data.has_fb)
        return 0;

    pthread_mutex_lock(&progress_lock);
    start_progress = 1;
    pthread_cond_signal(&progress_cond);
    pthread_mutex_unlock(&progress_lock);

    return 0;
}

static int stop_show_progress(struct gui* this) {
    if (!g_data.has_fb)
        return 0;

    pthread_mutex_lock(&progress_lock);
    start_progress = 0;
    pthread_cond_signal(&progress_cond);
    pthread_mutex_unlock(&progress_lock);

    return 0;
}

static int show_logo(struct gui* this, uint32_t pos_x, uint32_t pos_y) {
    if (!g_data.has_fb)
        return 0;

    int error = 0;
    struct gr_surface* surface = NULL;

    if (file_exist(prefix_image_logo_path) < 0) {
        LOGE("File not exist: %s\n", prefix_image_logo_path);
        error = -1;
        goto out;
    }

    if (png_decode_image(prefix_image_logo_path, &surface) < 0) {
        LOGE("Failed to decode image: %s\n", prefix_image_logo_path);
        error = -1;
        goto out;
    }

    if (gr_drawer->draw_png(gr_drawer, surface, pos_x, pos_y) < 0) {
        LOGE("Failed to draw png image: %s\n", prefix_image_logo_path);
        error = -1;
        goto out;
    }

out:
    if (surface)
        free(surface);

    return error;
}

static int show_tips(struct gui* this, enum update_stage_t stage) {
    if (!g_data.has_fb)
        return 0;

    const char* tips = NULL;
    uint32_t tips_len = 0;
    uint32_t pos_x = 0;
    uint32_t pos_y = 0;

    pos_y = (gr_drawer->get_fb_height(gr_drawer)
            - (char_height + progress_height
                    + PROGRESS_SPACE_TO_TIPS)) / 2;

    gr_drawer->set_pen_color(gr_drawer, 0, 0, 0);
    if (gr_drawer->fill_rect(gr_drawer, 0, pos_y,
            gr_drawer->get_fb_width(gr_drawer), pos_y + char_height) < 0) {
        LOGE("Failed to file rectangle\n");
        return -1;
    }

    switch (stage) {
    case UPDATING:
        gr_drawer->set_pen_color(gr_drawer, 0x00, 0xff, 0x00);
        tips = prefix_stage_updating;
        break;

    case UPDATE_SUCCESS:
        gr_drawer->set_pen_color(gr_drawer, 0x00, 0xff, 0x00);
        tips = prefix_stage_update_success;
        break;

    case UPDATE_FAILURE:
        gr_drawer->set_pen_color(gr_drawer, 0xff, 0x00, 0x00);
        tips = prefix_stage_update_failure;
        break;

    default:
        return -1;
    }

    tips_len = strlen(tips);

     pos_x = (gr_drawer->get_fb_width(gr_drawer)
            - (char_width * tips_len)) / 2;

    if (gr_drawer->draw_text(gr_drawer, pos_x, pos_y, tips, 1) < 0) {
        LOGE("Failed to draw text\n");
        return -1;
    }

    gr_drawer->display(gr_drawer);

    return 0;
}

static void clear(struct gui* this) {
    if (!g_data.has_fb)
        return;

    gr_drawer->set_pen_color(gr_drawer, 0x00, 0x00, 0x00);
    gr_drawer->fill_screen(gr_drawer);

    gr_drawer->display(gr_drawer);
}

static int init(struct gui* this) {
    if (!g_data.has_fb)
        return 0;

    int error = 0;

    gr_drawer = _new(struct gr_drawer, gr_drawer);
    if (gr_drawer->init(gr_drawer) < 0) {
        _delete(gr_drawer);
        gr_drawer = NULL;
        return -1;
    }

    gr_drawer->get_font_size(&char_width, &char_height);

    text_col = text_row = 0;
    text_rows = gr_drawer->get_fb_height(gr_drawer) / char_height;
    if (text_rows > kMaxRows)
        text_rows = kMaxRows;

    text_top = 1;

    text_cols = gr_drawer->get_fb_width(gr_drawer) / char_width;
    if (text_cols > kMaxCols - 1)
        text_cols = kMaxCols - 1;


    if (load_progress_image() < 0)
        return -1;

    pthread_t tid;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    error = pthread_create(&tid, &attr, progress_loop, (void *) this);
    if (error) {
        LOGE("pthread_create failed: %s", strerror(errno));
        pthread_attr_destroy(&attr);
        return -1;
    }

    pthread_mutex_init(&progress_lock, NULL);
    pthread_cond_init(&progress_cond, NULL);

    pthread_attr_destroy(&attr);

    return 0;
}

static int deinit(struct gui* this) {
    if (!g_data.has_fb)
        return 0;

    if (gr_drawer) {
        gr_drawer->deinit(gr_drawer);
        _delete(gr_drawer);
    }

    gr_drawer = NULL;

    pthread_mutex_destroy(&progress_lock);
    pthread_cond_destroy(&progress_cond);

    return 0;
}

void construct_gui(struct gui* this) {
    this->init = init;
    this->deinit = deinit;

    this->show_logo = show_logo;
    this->start_show_progress = start_show_progress;
    this->stop_show_progress = stop_show_progress;
    this->show_log = show_log;
    this->show_tips = show_tips;
    this->clear = clear;
}

void destruct_gui(struct gui* this) {
    this->init = NULL;
    this->deinit = NULL;

    this->show_logo = NULL;
    this->show_log = NULL;
    this->start_show_progress = NULL;
    this->stop_show_progress = NULL;
    this->show_tips = NULL;
    this->clear = NULL;
}
