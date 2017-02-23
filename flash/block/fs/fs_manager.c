#include <inttypes.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <utils/log.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <types.h>
#include <utils/assert.h>
#include <lib/mtd/jffs2-user.h>
#include <lib/libcommon.h>
#include <lib/mtd/libmtd.h>
#include <lib/mtd/mtd-user.h>
#include <utils/list.h>
#include <flash/block/fs/fs_manager.h>

#define LOG_TAG "fs_manager"

int target_endian = __BYTE_ORDER;

extern struct filesystem fs_normal;
static struct filesystem* fs_supported_list[] = {
    &fs_normal,
};

void fs_write_flags_get(struct filesystem *fs,
                        int *noecc, int *autoplace, int *writeoob,
                        int *oobsize, int *pad, int *markbad) {
    struct mtd_dev_info *mtd = FS_GET_MTD_DEV(fs);
    *noecc = FS_FLAG_IS_SET(fs, NOECC);
    *autoplace = FS_FLAG_IS_SET(fs, AUTOPLACE);
    *writeoob = FS_FLAG_IS_SET(fs, WRITEOOB);
    *oobsize = fs->tagsize ? fs->tagsize : mtd->oob_size;
    *pad = FS_FLAG_IS_SET(fs, PAD);
    *markbad = FS_FLAG_IS_SET(fs, MARKBAD);
#ifdef FS_OPEN_DEBUG
    LOGI("noecc = %d\n", *noecc);
    LOGI("autoplace = %d\n", *autoplace);
    LOGI("writeoob = %d\n", *writeoob);
    LOGI("oobsize = %d\n", *oobsize);
    LOGI("pad = %d\n", *pad);
    LOGI("markbad = %d\n", *markbad);
#endif
}

void fs_flags_get(struct filesystem *fs,
                  int *noskipbad) {
    *noskipbad = FS_FLAG_IS_SET(fs, NOSKIPBAD);
#ifdef FS_OPEN_DEBUG
    LOGI("noskipbad = %d\n", *noskipbad);
#endif
}


int fs_alloc_params(struct filesystem *this) {
    if (this->params)
        return 0;
    this->params = calloc(1, sizeof(*this->params));
    if (this->params == NULL) {
        LOGE("Cannot get memory space, request size is %d\n",
             sizeof(*this->params));
        return -1;
    }
    return 0;
}

int fs_free_params(struct filesystem *this) {
    if (this->params) {
        free(this->params);
        this->params = NULL;
    }
    return 0;
}

void fs_set_params(struct filesystem* fs, char *buf, int64_t offset,
                   int64_t length, int op_method, void *p,  void *fs_priv) {
    FS_GET_PARAM(fs)->buf  = buf;
    FS_GET_PARAM(fs)->offset  = offset;
    FS_GET_PARAM(fs)->length  = length;
    FS_GET_PARAM(fs)->operation_method  = op_method;
    FS_GET_PARAM(fs)->priv  = p;
    FS_SET_PRIVATE(fs, fs_priv);
}

int fs_register(struct list_head *head, struct filesystem* this) {
    struct filesystem *m;
    struct list_head *cell;

    if (head == NULL || this == NULL) {
        LOGE("list head or filesystem to be registered is null\n");
        return -1;
    }

    if (this->init(this) < 0) {
        LOGE("Filesystem \'%s\' init failed\n", this->name);
        return -1;
    }

    list_for_each(cell, head) {
        m = list_entry(cell, struct filesystem, list_cell);
        if (!strcmp(m->name,  this->name)) {
            LOGW("Filesystem \'%s\' is already registered", m->name);
            return 0;
        }
    }
    if (this->alloc_params(this) < 0) {
        LOGE("Filesystem \'%s\' alloc parameter failed\n", this->name);
        return -1;
    }
    list_add_tail(&this->list_cell, head);
    return 0;
}

int fs_unregister(struct list_head *head, struct filesystem* this) {
    struct filesystem *m;
    struct list_head *cell;
    struct list_head* next;
    if (head == NULL || this == NULL) {
        LOGE("list head or filesystem to be unregistered is null\n");
        return -1;
    }
    list_for_each_safe(cell, next, head) {
        m = list_entry(cell, struct filesystem, list_cell);
        if (!strcmp(m->name,  this->name)) {
            LOGD("Filesystem \'%s\' is removed successfully\n", m->name);
            this->free_params(this);
            list_del(cell);
            return 0;
        }
    }
    return -1;
}

struct filesystem* fs_get_registered_by_name(struct list_head *head, char *filetype) {
    struct filesystem *m;
    struct list_head *cell;
    list_for_each(cell, head) {
        m = list_entry(cell, struct filesystem, list_cell);
        if (!strcmp(m->name,  filetype)) {
            return m;
        }
    }
    return NULL;
}

struct filesystem* fs_get_suppoted_by_name(char *filetype) {
    int i;
    for (i = 0; i < sizeof(fs_supported_list) / sizeof(fs_supported_list[0]); i++) {
        if (!strcmp(fs_supported_list[i]->name,  filetype)) {
            return fs_supported_list[i];
        }
    }
    return NULL;
}

struct filesystem* fs_new(char *filetype) {
    struct filesystem* fs = fs_get_suppoted_by_name(filetype);
    struct filesystem* newer = NULL;
    if (fs == NULL) {
        LOGE("Filesystem \'%s\' is not supported yet\n", filetype);
        goto out;
    }
    newer = calloc(sizeof(*newer), 1);
    if (newer == NULL) {
        LOGE("Cannot allocate any space to filesystem\n");
        goto out;
    }

    memcpy(newer, fs, sizeof(*fs));
    newer->params = NULL;
    if (fs_alloc_params(newer) < 0)
        goto out;

    return newer;
out:
    return NULL;
}

struct filesystem* fs_derive(struct filesystem *origin) {
    struct filesystem* fs;
    if (origin == NULL) {
        LOGE("Parameter filesystem \'origin\' is null\n");
        goto out;
    }
    fs = calloc(sizeof(*fs), 1);
    if (fs == NULL) {
        LOGE("Cannot allocate any space to filesystem\n");
        goto out;
    }
    memcpy(fs, origin, sizeof(*fs));
    fs->params = NULL;
    if (fs_alloc_params(fs) < 0)
        goto out;
    memcpy(fs->params, origin->params, sizeof(*(fs->params)));

    return fs;
out:
    return NULL;
}

int fs_destroy(struct filesystem** fs) {
    struct filesystem **tmp = fs;

    if ((tmp == NULL) || (*tmp == NULL))
        goto out;

    if ((*tmp)->params)
        free((*tmp)->params);

    free(*tmp);
    *tmp = NULL;
    return 0;
out:
    return -1;
}