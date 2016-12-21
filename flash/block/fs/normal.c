#include <inttypes.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <types.h>
#include <lib/mtd/jffs2-user.h>
#include <lib/libcommon.h>
#include <lib/mtd/mtd-user.h>
#include <utils/list.h>
#include <flash/block/block_manager.h>
#include <flash/block/fs/fs_manager.h>
#include <flash/block/mtd/mtd.h>

#define LOG_TAG "fs_normal"

static int normal_init(struct filesystem *fs) {
    FS_FLAG_SET(fs, MARKBAD);
    return 0;
};

static int64_t normal_erase(struct filesystem *fs) {
    return mtd_basic_erase(fs);
}

static int64_t normal_read(struct filesystem *fs) {
    return mtd_basic_read(fs);
}

static int64_t normal_write(struct filesystem *fs) {
    return mtd_basic_write(fs);
}

static int64_t normal_get_operate_start_address(struct filesystem *fs) {
    return fs->params->offset;
}

static unsigned long normal_get_leb_size(struct filesystem *fs) {
    struct mtd_dev_info *mtd = FS_GET_MTD_DEV(fs);
    return mtd->eb_size;
}

struct filesystem fs_normal = {
    .name = BM_FILE_TYPE_NORMAL,
    .init = normal_init,
    .alloc_params = fs_alloc_params,
    .free_params = fs_free_params,
    .set_params = fs_set_params,
    .erase = normal_erase,
    .read = normal_read,
    .write = normal_write,
    .get_operate_start_address = normal_get_operate_start_address,
    .get_leb_size = normal_get_leb_size,
};



