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

#include <inttypes.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <pthread.h>
#include <types.h>
#include <utils/log.h>
#include <utils/assert.h>
#include <utils/common.h>
#include <lib/mtd/jffs2-user.h>
#include <lib/mtd/mtd-user.h>
#include <utils/list.h>
#include <flash/block/sysinfo/sysinfo_manager.h>
#include <flash/block/fs/fs_manager.h>
#include <flash/block/block_manager.h>
#include <flash/block/mtd/mtd.h>
#include <flash/flash_manager.h>

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

#define LOG_TAG "flash_manager"

static struct flash_info flash_info;

static int32_t flash_init(void) {
    int32_t ret;
    struct flash_info *info = &flash_info;

    if (info->init_called > 0) {
        LOGE("Flash init had been called before\n");
        goto out;
    }

    pthread_mutex_init(&info->mutex_lock, NULL);
    info->blk_mgr = mtd_manager_init();

    ret = info->blk_mgr->get_blocksize(info->blk_mgr, 0);
    if (ret < 0) {
        LOGE("Failed to get flash block size\n");
        goto out;
    }
    info->block_size = ret;

    ret = info->blk_mgr->get_iosize(info->blk_mgr, 0);
    if (ret < 0) {
        LOGE("Failed to get flash block size\n");
        goto out;
    }
    info->page_size = ret;

    if ((info->capacity
            = info->blk_mgr->get_capacity(info->blk_mgr)) < 0) {
        LOGE("Cannot get flash capacity\n");
        goto out;
    }
    info->init_called++;
    return 0;
out:
    info->blk_mgr = NULL;
    pthread_mutex_destroy(&info->mutex_lock);
    return -1;
}

static int32_t flash_deinit(void) {
    struct flash_info *info = &flash_info;
    if (info->init_called <= 0) {
        LOGE("Flash has not been initialed yet\n");
        goto out;
    }

    if (--info->init_called == 0) {
        info->block_size = 0;
        memset(&info->operation_option,
                0, sizeof(info->operation_option));
        mtd_manager_destroy(info->blk_mgr);
        info->blk_mgr = NULL;
        pthread_mutex_destroy(&info->mutex_lock);
    }
    return 0;
out:
    return -1;
}

static int32_t flash_get_erase_unit(void) {
    struct flash_info *info = &flash_info;
    assert_die_if(info->init_called <= 0, "Flash has not been initialed yet\n");

    return info->block_size;
}

static int64_t flash_erase(int64_t offset,  int64_t length) {
    struct flash_info *info = &flash_info;
    int64_t ret;

    assert_die_if(length == 0, "Parameter length is zero\n");
    assert_die_if(info->init_called <= 0,
            "Flash has not been initialed yet\n");
    assert_die_if((length + offset) > info->capacity, "Size overlow\n");

    if (offset % info->block_size) {
        LOGW("Flash erase offset 0x%llx is not block aligned", offset);
    }
    if (length % info->block_size) {
        LOGW("Flash erase length 0x%llx is not block aligned", length);
    }

    pthread_mutex_lock(&info->mutex_lock);
    ret = info->blk_mgr->erase(info->blk_mgr, offset, length);
    if ((ret < 0) || (ret < (offset + length))) {
        pthread_mutex_unlock(&info->mutex_lock);
        LOGE("FLash erase failed at offset 0x%llx with length 0x%llx\n",
                offset, length);
        return -1;
    }
    pthread_mutex_unlock(&info->mutex_lock);
    return 0;
}

static int64_t flash_read(int64_t offset,  void* buf, int64_t length) {
    struct flash_info *info = &flash_info;
    int64_t ret;

    assert_die_if(length == 0, "Parameter length is zero\n");
    assert_die_if(buf == NULL, "Parameter buf is null\n");
    assert_die_if(info->init_called <= 0,
            "Flash has not been initialed yet\n");
    assert_die_if((length + offset) > info->capacity, "Size overlow\n");

    pthread_mutex_lock(&info->mutex_lock);
    ret = info->blk_mgr->read(info->blk_mgr, offset, buf, length);
    if ((ret < 0) || (ret < (offset + length))) {
        pthread_mutex_unlock(&info->mutex_lock);
        LOGE("FLash read failed at offset 0x%llx\n", offset);
        return -1;
    }
    pthread_mutex_unlock(&info->mutex_lock);
    return length;
}

static int64_t flash_write(int64_t offset,  void* buf, int64_t length) {
    struct flash_info *info = &flash_info;
    int64_t ret;

    assert_die_if(length == 0, "Parameter length is zero\n");
    assert_die_if(buf == NULL, "Parameter buf is null\n");
    assert_die_if(info->init_called <= 0, "Flash has not been initialed yet\n");
    assert_die_if((length + offset) > info->capacity, "Size overlow\n");
#ifndef MTD_IO_SIZE_ARBITRARY
    if (offset % info->page_size) {
        LOGW("Flash write offset 0x%llx is not page aligned", offset);
    }
    if (length % info->page_size) {
        LOGW("Flash write length 0x%llx is not page aligned", length);
    }
#endif
    pthread_mutex_lock(&info->mutex_lock);
    ret = info->blk_mgr->write(info->blk_mgr, offset, buf, length);
    if (ret < 0) {
        pthread_mutex_unlock(&info->mutex_lock);
        LOGE("FLash write failed at offset 0x%llx with length 0x%llx\n",
                offset, length);
        return -1;
    }
    pthread_mutex_unlock(&info->mutex_lock);
    return length;
}

struct flash_manager flash_manager = {
    .init   = flash_init,
    .deinit = flash_deinit,
    .get_erase_unit = flash_get_erase_unit,
    .erase = flash_erase,
    .write  = flash_write,
    .read   = flash_read,
};

struct flash_manager* get_flash_manager(void) {
    return &flash_manager;
}
