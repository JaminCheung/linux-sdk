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

#include <string.h>
#include <stdlib.h>

#include <utils/log.h>
#include <utils/assert.h>
#include <utils/common.h>
#include <lib/png/png.h>
#include <graphics/gr_drawer.h>

#define LOG_TAG "png_decode"

#define SURFACE_DATA_ALIGNMENT 8

struct image_info {
    char name[PATH_MAX];
    uint32_t height;
    uint32_t width;
    int color_type;
    uint32_t channels;
    int bit_depth;
    uint32_t row_bytes;
};

static void dump_image_info(struct image_info *info) {
    LOGD("=========================\n");
    LOGD("Dump image info\n");
    LOGD("Name:       %s\n", info->name);
    LOGD("Height:     %u\n", info->height);
    LOGD("Width:      %u\n", info->width);
    LOGD("Bit depth:  %d\n", info->bit_depth);
    LOGD("Channels:   %u\n", info->channels);
    LOGD("Row bytes:  %u\n", info->row_bytes);
    LOGD("Color type: %d\n", info->color_type);
    LOGD("=========================\n");
}

static struct gr_surface* alloc_surface(uint32_t data_size) {
    uint8_t* temp = calloc(1, sizeof(struct gr_surface) + data_size
            + SURFACE_DATA_ALIGNMENT);
    if (temp == NULL)
        return NULL;

    struct gr_surface* surface = (struct gr_surface *) temp;

    surface->raw_data = temp + sizeof(struct gr_surface) +
        (SURFACE_DATA_ALIGNMENT - (sizeof(struct gr_surface)
                % SURFACE_DATA_ALIGNMENT));

    return surface;
}

static void transform_rgb_to_draw(uint8_t* input_row, uint8_t* output_row,
                                  int channels, uint32_t width) {
    int x;
    uint8_t* ip = input_row;
    uint8_t* op = output_row;

    switch (channels) {
        case 1:
            // expand gray level to RGBX
            for (x = 0; x < width; ++x) {
                *op++ = *ip;
                *op++ = *ip;
                *op++ = *ip;
                *op++ = 0xff;
                ip++;
            }
            break;

        case 3:
            // expand RGBA to RGBX
            for (x = 0; x < width; ++x) {
                *op++ = *ip++;
                *op++ = *ip++;
                *op++ = *ip++;
                *op++ = 0xff;
            }
            break;

        case 4:
            // copy RGBA to RGBX
            memcpy(output_row, input_row, width * 4);
            break;
    }
}

static int png_decode(const char* path, png_structp* png_ptr, png_infop* info_ptr,
        struct image_info* image_info, uint8_t gray_to_rgb) {
    uint8_t header[8];

    FILE* fp = fopen(path, "rb");
    if (fp == NULL) {
        LOGE("Failed to open %s: %s\n", path, strerror(errno));
        return -1;
    }

    uint32_t bytes_readed = fread(header, 1, sizeof(header), fp);
    if (bytes_readed != sizeof(header)) {
        LOGE("Failed to read png image header\n");
        return -1;
    }

    if (png_sig_cmp(header, 0, sizeof(header))) {
        LOGE("Image is not png\n");
        return -1;
    }

    *png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
    if (png_ptr == NULL) {
        LOGI("Failed to create png structure\n");
        return -1;
    }

    if (setjmp(png_jmpbuf(*png_ptr))) {
        LOGE("Failed to set error jump point\n");
        png_destroy_read_struct(png_ptr, info_ptr, NULL);
        return -1;
    }

    png_init_io(*png_ptr, fp);
    *info_ptr = png_create_info_struct(*png_ptr);
    if (info_ptr == NULL) {
        LOGE("Failed create png info structure\n");
        return -1;
    }

    png_set_sig_bytes(*png_ptr, sizeof(header));

    png_read_info(*png_ptr, *info_ptr);

    /*
     * Format png to RGBA8888
     */
    int bit_depth = png_get_bit_depth (*png_ptr, *info_ptr);
    int color_type = png_get_color_type(*png_ptr, *info_ptr);

    if (color_type == PNG_COLOR_TYPE_PALETTE)
        png_set_palette_to_rgb(*png_ptr);

    if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
        png_set_expand_gray_1_2_4_to_8(*png_ptr);

    if (png_get_valid (*png_ptr, *info_ptr, PNG_INFO_tRNS))
        png_set_tRNS_to_alpha(*png_ptr);

    if (gray_to_rgb) {
        if(color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
            png_set_gray_to_rgb(*png_ptr);
    }

    if(bit_depth == 16)
        png_set_strip_16(*png_ptr);
    else if (bit_depth < 8)
        png_set_packing(*png_ptr);

    png_read_update_info(*png_ptr, *info_ptr);

    /*
     * Now format is RGBA8888
     */
    png_get_IHDR(*png_ptr, *info_ptr, &image_info->width, &image_info->height,
            &image_info->bit_depth, &image_info->color_type, NULL, NULL, NULL);

    image_info->row_bytes = png_get_rowbytes(*png_ptr, *info_ptr);
    image_info->channels = png_get_channels(*png_ptr, *info_ptr);

    dump_image_info(image_info);

    return 0;
}

int png_decode_font(const char* path, struct gr_surface** psurface) {
    assert_die_if(path == NULL, "path is NULL\n");

    png_structp png_ptr;
    png_infop info_ptr;

    struct gr_surface* surface = NULL;
    struct image_info image_info;

    sprintf(image_info.name, path, strlen(path) + 1);

    if (png_decode(path, &png_ptr, &info_ptr, &image_info, 0) < 0) {
        LOGE("Failed to decode font image: %s\n", path);
        return -1;
    }

    if (image_info.channels != 1) {
        LOGE("Invalid channels: %d\n", image_info.channels);
        return -1;
    }

    surface = alloc_surface(image_info.width * image_info.height);
    if (surface == NULL) {
        LOGE("Failed to allocate memory\n");
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        return -1;
    }

    surface->width = image_info.width;
    surface->height = image_info.height;
    surface->row_bytes = image_info.width;
    surface->pixel_bytes = 1;

    uint8_t* row;
    for (int i = 0; i < image_info.height; i++) {
        row = surface->raw_data + i * surface->row_bytes;
        png_read_row(png_ptr, row, NULL);
    }

    *psurface = surface;

    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);

    return 0;
}

int png_decode_image(const char* path, struct gr_surface** psurface) {
    assert_die_if(path == NULL, "path is NULL\n");

    png_structp png_ptr;
    png_infop info_ptr;

    struct gr_surface* surface = NULL;
    struct image_info image_info;

    sprintf(image_info.name, path, strlen(path) + 1);

    if (png_decode(path, &png_ptr, &info_ptr, &image_info, 1) < 0) {
        LOGE("Failed to decode png image: %s\n", path);
        return -1;
    }

    surface = alloc_surface(image_info.width * image_info.height * 4);
    if (surface == NULL) {
        LOGE("Failed to allocate memory\n");
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        return -1;
    }

    surface->width = image_info.width;
    surface->height = image_info.height;
    surface->row_bytes = image_info.width * 4;
    surface->pixel_bytes = 4;

    uint8_t* row = (uint8_t *) malloc(image_info.width * 4);
    for (int i = 0; i < image_info.height; i++) {
        png_read_row(png_ptr, row, NULL);

        transform_rgb_to_draw(row, surface->raw_data + i * surface->row_bytes,
                image_info.channels, image_info.width);
    }
    free(row);

    *psurface = surface;

    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);

    return 0;
}
