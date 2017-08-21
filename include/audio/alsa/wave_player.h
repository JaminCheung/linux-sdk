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

#ifndef WAVE_PLAYER_H
#define WAVE_PLAYER_H

struct wave_player {
    int (*init)(void);
    int (*deinit)(void);
    int (*play_file)(const char* snd_device, int fd);
    int (*set_volume)(const char* name, int volume);
    int (*mute)(const char* name, int mute);
};

struct wave_player* get_wave_player(void);

#endif /* WAVE_PLAYER_H */
