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
            container->format.format_size != LE_INT(16) ||
           (container->format.channels != LE_SHORT(1) && container->format.channels != LE_SHORT(2)) ||
            container->chunk_header.type != WAV_DATA ) {

           LOGE("Not standard wave file.\n");
           return -1;
    }

    return 0;
}

int wave_read_header(int fd, WaveContainer* container) {
    assert_die_if(fd < 0, "Invalid fd\n");
    assert_die_if(container == NULL, "container is NULL\n");

    int error = 0;
    uint32_t magic;
    uint32_t length;
    uint32_t type;
    uint32_t fmt_magic;
    uint32_t fmt_size;
    uint16_t audio_fmt;
    uint16_t channels;
    uint32_t sample_fq;
    uint32_t byte_p_sec;
    uint16_t byte_p_spl;
    uint16_t bit_p_spl;
    uint32_t chunk_type;
    uint16_t chunk_type_hi;
    uint16_t chunk_type_low;
    uint32_t chunk_size;
    uint32_t skip;

    error = read(fd, &magic, sizeof(uint32_t));
    if (error != sizeof(uint32_t) || magic != WAV_RIFF) {
        LOGE("Parse wave magic error\n");
        goto error;
    }

    error = read(fd, &length, sizeof(uint32_t));
    if (error != sizeof(uint32_t)) {
        LOGE("Parse wave length error\n");
        goto error;
    }

    error = read(fd, &type, sizeof(uint32_t));
    if (error != sizeof(uint32_t) || type != WAV_WAVE) {
        LOGE("Parse wave type error\n");
        goto error;
    }

    error = read(fd, &fmt_magic, sizeof(uint32_t));
    if (error != sizeof(uint32_t) || fmt_magic != WAV_FMT) {
        LOGE("Parse wave fmt id error\n");
        goto error;
    }

    error = read(fd, &fmt_size, sizeof(uint32_t));
    if (error != sizeof(uint32_t)) {
        LOGE("Parse wave fmt size error\n");
        goto error;
    }

    error = read(fd, &audio_fmt, sizeof(uint16_t));
    if (error != sizeof(uint16_t)) {
        LOGE("Parse wave audio fmt error\n");
        goto error;
    }

    error = read(fd, &channels, sizeof(uint16_t));
    if (error != sizeof(uint16_t)) {
        LOGE("Parse wave channels error\n");
        goto error;
    }

    error = read(fd, &sample_fq, sizeof(uint32_t));
    if (error != sizeof(uint32_t)) {
        LOGE("Parse wave sample rate error\n");
        goto error;
    }

    error = read(fd, &byte_p_sec, sizeof(uint32_t));
    if (error != sizeof(uint32_t)) {
        LOGE("Parse wave byte per second error\n");
        goto error;
    }

    error = read(fd, &byte_p_spl, sizeof(uint16_t));
    if (error != sizeof(uint16_t)) {
        LOGE("Parse wave byte per sample error\n");
        goto error;
    }

    error = read(fd, &bit_p_spl, sizeof(uint16_t));
    if (error != sizeof(uint16_t)) {
        LOGE("Parse wave bits per sample error\n");
        goto error;
    }

    skip = 8 + fmt_size - sizeof(WaveFmtBody);
    lseek(fd, skip, SEEK_CUR);

    for (;;) {

        /*
         * The standard WAVE data chunk area is little endian
         */
        if (read(fd, &chunk_type_low, sizeof(uint16_t)) != sizeof(uint16_t)) {
            LOGE("Parse wave data chunk type low bits error\n");
            goto error;
        }

        if (chunk_type_low == (WAV_DATA & 0xffff)) {
            if (read(fd, &chunk_type_hi, sizeof(uint16_t)) != sizeof(uint16_t)) {
                LOGE("Parse wave data chunk type high bits error\n");
                goto error;
            }
        }

        chunk_type = (chunk_type_hi<< 16 | chunk_type_low);
        if (chunk_type == WAV_DATA) {
            if (read(fd, &chunk_size, sizeof(uint32_t)) != sizeof(uint32_t)) {
                LOGE("Parse wave data chunk size error\n");
                goto error;
            }

            break;
        }
    }

    container->header.magic = magic;
    container->header.length = length;
    container->header.type = type;
    container->format.magic = fmt_magic;
    container->format.format_size = fmt_size;
    container->format.format = audio_fmt;
    container->format.channels = channels;
    container->format.sample_fq = sample_fq;
    container->format.byte_p_sec = byte_p_sec;
    container->format.byte_p_spl = byte_p_spl;
    container->format.bit_p_spl = bit_p_spl;
    container->chunk_header.type = chunk_type;
    container->chunk_header.length = chunk_size;

    dump_header(container);

    return 0;

error:
    LOGE("Not standard wave file.\n");
    return -1;
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
