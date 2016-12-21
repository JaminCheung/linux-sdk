#include <inttypes.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <utils/log.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <types.h>
#include <utils/assert.h>
#include <lib/libcommon.h>
#include <lib/mtd/mtd-user.h>
#include <utils/list.h>
#include <utils/common.h>
#include <flash/block/sysinfo/sysinfo_manager.h>
#include <flash/block/block_manager.h>
#include <flash/block/fs/fs_manager.h>
#include <flash/block/mtd/mtd.h>

#define    LOG_TAG     "sysinfo_manager"

static struct sysinfo_layout layout[] = {
    {SYSINFO_RESERVED_OFFSET,  SYSINFO_RESERVED_SIZE, NULL},
};

static int is_id_valid(int id){
    if (id < 0 || id >= ARRAY_SIZE(layout)) {
        return false;
    }
    return true;
}

static int64_t sysinfo_get_offset(struct sysinfo_manager *this, int id) {
    struct sysinfo_layout *l = NULL;
    if (!is_id_valid(id)) {
        LOGE("id%d should not be recognised\n", id);
        goto out;
    }
    l = &layout[id];
    return l->offset;
out:
    return -1;
}

static int64_t sysinfo_get_length(struct sysinfo_manager *this, int id) {
    struct sysinfo_layout *l = NULL;
    if (!is_id_valid(id)) {
        LOGE("id%d should not be recognised\n", id);
        goto out;
    }
    l = &layout[id];
    return l->length;
out:
    return -1;
}

static char** sysinfo_get_value_address(struct sysinfo_manager *this, int id) {
    struct sysinfo_layout *l = NULL;
    if (!is_id_valid(id)) {
        LOGE("id%d should not be recognised\n", id);
        goto out;
    }
    l = &layout[id];
    return &(l->value);
out:
    return NULL;
}

static int sysinfo_get_value(struct sysinfo_manager *this,
                             int id, char *buf, char flag)
{
    char** data = NULL;
    int64_t offset, len;

    offset = sysinfo_get_offset(this, id);
    len = sysinfo_get_length(this, id);
    if ((offset < 0) || (len < 0))
        goto out;

    data = sysinfo_get_value_address(this, id);
    if (data == NULL)
        goto out;
    if (*data == NULL) {
        *data = calloc(1, len);
        if (*data == NULL) {
            LOGE("Cannot alloc any memory space, requested %lld\n", len);
            goto out;
        }
    }

    if (flag == SYSINFO_OPERATION_DEV) {
        struct filesystem* fs = fs_new(BM_FILE_TYPE_NORMAL);
        struct block_manager *bm = (struct block_manager *)GET_SYSINFO_BINDER(this);
        struct mtd_dev_info *mtd = NULL;
        if (fs == NULL) {
            LOGE("Cannot instance filesystem %s\n", BM_FILE_TYPE_NORMAL);
            goto out;
        }
        if (bm == NULL) {
            LOGE("Cannot get binder bm\n");
            goto  out;
        }
        mtd = mtd_get_dev_info_by_offset(bm, offset);
        if (mtd == NULL) {
            LOGE("offset 0x%llx cannot be recognised by mtd\n", offset);
            goto out;
        }

        fs->set_params(fs, *data, offset,
                       len, BM_OPERATION_METHOD_RANDOM, mtd, bm);
        if (!strcmp(bm->name, BM_BLOCK_TYPE_MTD)) {

            if (fs->get_max_mapped_size_in_partition(fs) <= 0) {
                LOGE("Cannot get max mapped size by fs \'%s\'\n", fs->name);
                goto out;
            }
            if (fs->read(fs) < 0) {
                LOGE("Cannot read at offset 0x%llx by length %lld\n", offset, len);
                goto out;
            }
        }
        // for (int i = 0; i < len; i++) {
        //     if ((i % 32) == 0)
        //         printf("offset: 0x%08llx: ", offset);
        //     printf(" %x ", (*data)[i]);
        //     if (((i + 1) % 32) == 0)
        //         printf("\n");
        // }
        if (buf) {
            memcpy(buf, *data, len);
        }
        fs_destroy(&fs);
    } else if (flag == SYSINFO_OPERATION_RAM) {
        memcpy(buf, *data, len);
    }

    return 0;
out:
    return -1;
}


static int sysinfo_init(struct sysinfo_manager *this) {
    return 0;
}

static int sysinfo_exit(struct sysinfo_manager *this) {
    for (int i = 0;  i < ARRAY_SIZE(layout); i++) {
        char **data = sysinfo_get_value_address(this, i);
        if (data == NULL)
            goto out;
        if (*data) {
            free(*data);
            *data = NULL;
        }
    }
    return 0;
out:
    return -1;
}

static int sysinfo_traversal_save(struct sysinfo_manager *this,
                                  int64_t offset, int64_t length) {
    int64_t start = offset;
    int64_t end = start + length;

    struct sysinfo_layout *last = &layout[ARRAY_SIZE(layout) - 1];

    if (offset > (last->offset + last->length))
        return 0;

    for (int i = 0;  i < ARRAY_SIZE(layout); i++) {
        struct sysinfo_layout *l = &layout[i];
        int64_t l_start = l->offset;
        int64_t l_end = l_start + l->length;
        if ((end <= l_start) || (start >= l_end)) {
            continue;
        }
        LOGI("sysinfo id%d will be saved\n", i);
        if (sysinfo_get_value(this, i, NULL, SYSINFO_OPERATION_DEV) < 0)
            goto out;
    }
    return 0;
out:
    return -1;
}

static int sysinfo_traversal_merge(struct sysinfo_manager *this,
                                   char *buf, int64_t offset, int64_t length) {
    int64_t start = offset;
    int64_t end = start + length;

    struct sysinfo_layout *last = &layout[ARRAY_SIZE(layout) - 1];
    if (offset >= (last->offset + last->length))
        return 0;

    for (int i = 0;  i < ARRAY_SIZE(layout); i++) {
        struct sysinfo_layout *l = &layout[i];
        int64_t l_start = l->offset;
        int64_t l_end = l_start + l->length;
        if ((end <= l_start) || (start >= l_end))
            continue;

        char** v = sysinfo_get_value_address(this, i);
        if (*v == NULL) {
            LOGE("You must call sysinfo_traversal_save before\n");
            goto out;
        }
        int64_t overlap_left = MAX(l_start, start);
        int64_t overlap_right = MIN(end, l_end);
        int64_t overlap = overlap_right  - overlap_left;
        char *t_buf = buf;
        char *l_buf = *v;
        if (buf == NULL) {
            LOGE("Parameter \'buf\' is null\n");
            goto out;
        }
        if (overlap <= 0) {
            LOGE("Should not come here, overlap = %lld\n", overlap);
            goto out;
        }
        t_buf += (l_start > start) ? l_start - start : 0;
        l_buf += (l_start < start) ? start - l_start : 0;

        offset += (l_start > start) ? l_start - start : 0;
        LOGI("offset 0x%llx will be merged, merged size %lld\n", offset, overlap);
        memcpy(t_buf, l_buf, overlap);
    }
    return 0;
out:
    return -1;
}

void sysinfo_manager_bind(struct sysinfo_manager *this, void *target) {
    struct block_manager *bm = (struct block_manager *)target;
    this->binder = bm;
    bm->sysinfo = this;
}

struct sysinfo_manager sysinfo = {
    .get_offset = sysinfo_get_offset,
    .get_length = sysinfo_get_length,
    .get_value = sysinfo_get_value,
    .traversal_save = sysinfo_traversal_save,
    .traversal_merge = sysinfo_traversal_merge,
    .init = sysinfo_init,
    .exit = sysinfo_exit,
};