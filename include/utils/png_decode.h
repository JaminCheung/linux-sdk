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

#ifndef PNG_DECODE_H
#define PNG_DECODE_H

#include <graphics/gr_drawer.h>

int png_decode_image(const char* path, struct gr_surface** surface);
int png_decode_font(const char* path, struct gr_surface** surface);

#endif /* PNG_DECODE_H */
