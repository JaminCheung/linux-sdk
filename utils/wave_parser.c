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

#include <unistd.h>

#include <utils/log.h>
#include <utils/assert.h>
#include <utils/wave_parser.h>

#define LOG_TAG "wave_parser"

const char* format_to_string(uint16_t format) {
    switch(format) {
    case WAV_FMT_PCM:
        return "PCM";
    case WAV_FMT_IEEE_FLOAT:
        return "IEEE FLOAT";
    case WAV_FMT_DOLBY_AC3_SPDIF:
        return "DOLBY AC3 SPDIF";
    case WAV_FMT_EXTENSIBLE:
        return "EXTENSIBLE";
    default:
        return "Unknown";
    }
}

static void dump_header(WaveContainer* container) {
    LOGD("========================================\n");
    LOGD("Dump wave header.\n");
    LOGD("File magic:        %c%c%c%c\n",
            (uint8_t)(container->header.magic),
            (uint8_t)(container->header.magic >> 8),
            (uint8_t)(container->header.magic >> 16),
            (uint8_t)(container->header.magic >> 24));
    LOGD("File length:       %d\n", container->header.length);
    LOGD("File type:         %c%c%c%c\n",
            (uint8_t)(container->header.type),
            (uint8_t)(container->header.type >> 8),
            (uint8_t)(container->header.type >> 16),
            (uint8_t)(container->header.type >> 24));
    LOGD("Fmt magic:         %c%c%c%c\n",
            (uint8_t)(container->format.magic),
            (uint8_t)(container->format.magic >> 8),
            (uint8_t)(container->format.magic >> 16),
            (uint8_t)(container->format.magic >> 24));
    LOGD("Fmt size:          %d\n", container->format.format_size);
    LOGD("Fmt format:        %s\n", format_to_string(container->format.format));
    LOGD("Fmt channels:      %d\n", container->format.channels);
    LOGD("Fmt sample rate:   %dHZ\n", container->format.sample_fq);
    LOGD("Fmt bytes per sec: %d\n", container->format.byte_p_sec);
    LOGD("Fmt block align:   %d\n", container->format.byte_p_spl);
    LOGD("Fmt sample length: %d\n", container->format.bit_p_spl);

    LOGD("Chunk type:        %c%c%c%c\n",
            (uint8_t)(container->chunk_header.type),
            (uint8_t)(container->chunk_header.type >> 8),
            (uint8_t)(container->chunk_header.type >> 16),
            (uint8_t)(container->chunk_header.type >> 24));
    LOGD("Chunk length:      %d\n", container->chunk_header.length);
    LOGD("========================================\n");
}

static int check_vaild(WaveContainer* container) {
    if (container->header.magic != WAV_RIFF ||
            container->header.type != WAV_WAVE ||
            container->format.magic != WAV_FMT ||
           (container->format.format_size != LE_INT(16) && container->format.format_size != LE_INT(18)) ||
           (container->format.channels != LE_SHORT(1) && container->format.channels != LE_SHORT(2)) /*||
            container->chunk_header.type != WAV_DATA */) {

           LOGE("Not standard wave file.\n");
           return -1;
    }

    return 0;
}

int wave_read_header(int fd, WaveContainer* container) {
    assert_die_if(fd < 0, "Invalid fd\n");
    assert_die_if(container == NULL, "container is NULL\n");

    if (read(fd, &container->header, sizeof(container->header)) != sizeof(container->header) ||
        read(fd, &container->format, sizeof(container->format)) != sizeof(container->format) ||
        read(fd, &container->chunk_header, sizeof(container->chunk_header)) != sizeof(container->chunk_header)) {

        return -1;
    }

    if (check_vaild(container) < 0)
        return -1;

    dump_header(container);

    return 0;
}

int wave_write_header(int fd, WaveContainer* container) {
    assert_die_if(fd < 0, "Invalid fd\n");
    assert_die_if(container == NULL, "container is NULL\n");

    if (check_vaild(container) < 0)
        return -1;

    if (write(fd, &container->header, sizeof(container->header)) != sizeof(container->header) ||
        write(fd, &container->format, sizeof(container->format)) != sizeof(container->format) ||
        write(fd, &container->chunk_header, sizeof(container->chunk_header)) != sizeof(container->chunk_header)) {

        return -1;
    }

    dump_header(container);

    return 0;
}
