/*
 *  Copyright (C) 2016, Zhang YanMing <yanmin.zhang@ingenic.com, jamincheung@126.com>
 *
 *  Ingenic QRcode SDK Project
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
#ifndef FLASH_MANAGER_H
#define FLASH_MANAGER_H

struct  flash_info {
    struct block_manager *blk_mgr;
    uint32_t init_called;
    struct bm_operation_option operation_option;
    struct bm_operate_prepare_info*  prepare_info;
    uint32_t block_size;
    uint32_t page_size;
    int64_t capacity;
    pthread_mutex_t mutex_lock;
};

#include <ingenic_api.h>
#endif
