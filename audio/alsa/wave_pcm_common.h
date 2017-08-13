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

#ifndef WAVE_PCM_COMMON_H
#define WAVE_PCM_COMMON_H

#include <types.h>
#include <utils/wave_parser.h>
#include <lib/alsa/asoundlib.h>

struct snd_pcm_container {
    snd_pcm_t * handle;
    snd_output_t* out_log;
    snd_pcm_uframes_t chunk_size;
    snd_pcm_uframes_t buffer_size;
    snd_pcm_format_t format;
    uint16_t channels;
    uint32_t chunk_bytes;
    uint32_t bits_per_sample;
    uint32_t bits_per_frame;
    uint8_t *data_buf;
};

uint32_t wave_pcm_read(struct snd_pcm_container* pcm_container, uint32_t count);
uint32_t wave_pcm_write(struct snd_pcm_container* pcm_container, uint32_t count);
int wave_pcm_set_params(struct snd_pcm_container* pcm_container, WaveContainer *wave_container);

#endif /* WAVE_PCM_COMMON_H */
