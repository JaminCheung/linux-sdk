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
#include <lib/mtd/jffs2-user.h>
#include <lib/libcommon.h>
#include <lib/mtd/mtd-user.h>
#include <utils/list.h>
#include <flash/block/sysinfo/sysinfo_manager.h>
#include <flash/block/block_manager.h>
#include <flash/block/mtd/mtd.h>

#define LOG_TAG "mtd_base"

int mtd_type_is_nand(struct mtd_dev_info *mtd) {
    return mtd->type == MTD_NANDFLASH || mtd->type == MTD_MLCNANDFLASH;
}

int mtd_type_is_mlc_nand(struct mtd_dev_info *mtd) {
    return mtd->type == MTD_MLCNANDFLASH;
}

int mtd_type_is_nor(struct mtd_dev_info *mtd) {
    return mtd->type == MTD_NORFLASH;
}

int mtd_boundary_is_valid(struct filesystem *fs, int64_t eb_start, int64_t eb_end) {
    struct mtd_dev_info *mtd = FS_GET_MTD_DEV(fs);
    int64_t left_limit = 0;
    int64_t right_limit = MTD_OFFSET_TO_EB_INDEX(mtd, mtd->size);
    if ((eb_start < left_limit) || (eb_end > right_limit)) {
        LOGE("start eb%lld to end eb%lld is invalid, valid boundary is from %lld to %lld\n",
             eb_start, eb_end, left_limit, right_limit);
        return false;
    }
    return true;
}

int64_t mtd_basic_erase(struct filesystem *fs) {
    struct block_manager *bm = FS_GET_BM(fs);
    libmtd_t mtd_desc = BM_GET_MTD_DESC(bm);
    struct mtd_dev_info *mtd = FS_GET_MTD_DEV(fs);

    int is_nand = mtd_type_is_nand(mtd);
    int noskipbad = 0;
    int *fd = &MTD_DEV_INFO_TO_FD(mtd);

    int64_t start, end, total_bytes, erased_bytes;
    int64_t offset, eb;
    int64_t bad_unlock_nerase_ebs = 0, nerase_size = 0;
    int err;

    fs_flags_get(fs, &noskipbad);

    offset = fs->params->offset;
    start = MTD_OFFSET_TO_EB_INDEX(mtd, offset);
    end = MTD_OFFSET_TO_EB_INDEX(mtd,
                                 MTD_BLOCK_ALIGN(mtd, offset + fs->params->length + mtd->eb_size - 1));

    total_bytes = (end - start) * mtd->eb_size;
    start = MTD_EB_ABSOLUTE_TO_RELATIVE(mtd, start);
    end = MTD_EB_ABSOLUTE_TO_RELATIVE(mtd, end);
    if (!mtd_boundary_is_valid(fs, start, end)) {
        LOGE("mtd boundary start eb %lld to end eb %lld is invalid\n",
             start, end - 1);
        goto closeall;
    }
#ifdef MTD_OPEN_DEBUG
    LOGI("MTD \"%s\" is going to erase from eb%lld to eb%lld, total length is %lld bytes\n",
         MTD_DEV_INFO_TO_PATH(mtd), start, end - 1, total_bytes);
#endif
    eb = start;
    erased_bytes = 0;
    while ((erased_bytes < total_bytes)
            && (eb < mtd->eb_cnt)) {
        offset = eb * mtd->eb_size;
        if (is_nand && !noskipbad) {
            err = mtd_is_bad(mtd, *fd, eb);
            if (err == -1) {
                LOGE("mtd block %lld bad detecting wrong\n", eb);
                goto closeall;
            }
            if (err) {
                if (errno == EOPNOTSUPP) {
                    LOGE("Bad block check not available on MTD \"%s\"",
                         MTD_DEV_INFO_TO_PATH(mtd));
                    goto closeall;
                }
                LOGI("Block%lld is bad\n", eb);
                eb++;
                bad_unlock_nerase_ebs++;
                continue;
            }
        }

        if (FS_FLAG_IS_SET(fs, UNLOCK)) {
            if (mtd_unlock(mtd, *fd, eb) != 0) {
                LOGE("MTD \"%s\" unlock failure", MTD_DEV_INFO_TO_PATH(mtd));
                eb++;
                bad_unlock_nerase_ebs++;
                continue;
            }
        }

#ifdef BM_SYSINFO_SUPPORT
        if (bm->sysinfo) {
            err = bm->sysinfo->traversal_save(bm->sysinfo,
                                              offset + MTD_DEV_INFO_TO_START(mtd), mtd->eb_size);
            if (err < 0) {
                LOGE("MTD \"%s\" failed to save system info\n", MTD_DEV_INFO_TO_PATH(mtd));
                goto closeall;
            }
        }
#endif
#ifdef MTD_OPEN_DEBUG
        LOGI("MTD %s: erase at block%lld\n",
                    MTD_DEV_INFO_TO_PATH(mtd), eb);
#endif
        err = mtd_erase(mtd_desc, mtd, *fd, eb);
        if (err) {
            LOGE("MTD \"%s\" failed to erase eraseblock %lld\n",
                 MTD_DEV_INFO_TO_PATH(mtd), eb);
            if (errno != EIO) {
                LOGE("MTD \"%s\" fatal error occured on %lld\n",
                     MTD_DEV_INFO_TO_PATH(mtd), eb);
                goto closeall;
            }
            if (mtd_mark_bad(mtd, *fd, eb)) {
                LOGE("MTD \"%s\" mark bad block failed on %lld\n",
                     MTD_DEV_INFO_TO_PATH(mtd), eb);
                goto closeall;
            }
            eb++;
            bad_unlock_nerase_ebs++;
            continue;
        }
        erased_bytes += mtd->eb_size;
        eb++;
    }

    nerase_size = bad_unlock_nerase_ebs * mtd->eb_size;
    if ((eb >= mtd->eb_cnt)
            && (erased_bytes + nerase_size < total_bytes)) {
        LOGE("The erase length you have requested is too large to issuing\n");
        LOGE("Request length: %lld, erase length: %lld, non erase length: %lld\n",
             total_bytes, erased_bytes, nerase_size);
        goto closeall;
    }

    start = MTD_EB_RELATIVE_TO_ABSOLUTE(mtd, eb) * mtd->eb_size;
    return start;
closeall:
    LOGE("%s has crashed\n", __func__);
    if (*fd) {
        close(*fd);
        *fd = 0;
    }
    return -1;
}


int64_t mtd_basic_write(struct filesystem *fs) {
    struct block_manager *bm = FS_GET_BM(fs);
    libmtd_t mtd_desc = BM_GET_MTD_DESC(bm);
    struct mtd_dev_info *mtd = FS_GET_MTD_DEV(fs);

    int is_nand = mtd_type_is_nand(mtd);
    int *fd = &MTD_DEV_INFO_TO_FD(mtd);

    int noecc, autoplace, writeoob, oobsize, pad, markbad, pagelen;
    unsigned int write_mode;
    long long mtd_start, w_length, w_offset, blockstart = -1, writen;
    char *w_buffer, *oobbuf;
    int ret;

    w_length = FS_GET_PARAM(fs)->length;
    w_buffer = FS_GET_PARAM(fs)->buf;
    w_offset = FS_GET_PARAM(fs)->offset;
    mtd_start = MTD_DEV_INFO_TO_START(mtd);
    w_offset -= mtd_start;

    fs_write_flags_get(fs, &noecc, &autoplace, &writeoob, &oobsize, &pad, &markbad);

#ifdef MTD_IO_SIZE_ARBITRARY
    if (writeoob) {
        LOGE("Cannot support writing oob when macro MTD_IO_SIZE_ARBITRARY is opened\n");
        goto closeall;
    }
#endif
    if (w_offset < 0) {
        LOGE("The start address  %lld is negative.\n",
             w_offset);
        goto closeall;
    }
    if (w_length <=0) {
         LOGE("The write length %lld is not plus number.\n",
             w_length);
        goto closeall;
    }
#ifndef MTD_IO_SIZE_ARBITRARY
    if (w_offset & (mtd->min_io_size - 1)) {
        LOGE("The start address is not page-aligned !"
             "The pagesize of this Flash is 0x%x.\n",
             mtd->min_io_size);
        goto closeall;
    }
#endif
    if (noecc)  {
        ret = ioctl(*fd, MTDFILEMODE, MTD_FILE_MODE_RAW);
        if (ret) {
            switch (errno) {
            case ENOTTY:
                LOGE("ioctl MTDFILEMODE is missing\n");
                goto closeall;
            default:
                LOGE("MTDFILEMODE\n");
                goto closeall;
            }
        }
    }

    pagelen = mtd->min_io_size;
    if (is_nand)
        pagelen = mtd->min_io_size + ((writeoob) ? oobsize : 0);

#ifndef MTD_IO_SIZE_ARBITRARY
    if (!pad && (w_length % pagelen) != 0) {
        LOGE("Writelength is not page-aligned. Use the padding "
             "option.\n");
        goto closeall;
    }
    if ((w_length / pagelen) * mtd->min_io_size
            > mtd->size - w_offset) {
        LOGE("MTD name \"%s\" overlow, write %lld bytes, page %d bytes, OOB area %d"
             " bytes, device size %lld bytes\n",
             MTD_DEV_INFO_TO_PATH(mtd), w_length, pagelen, oobsize, mtd->size);
        LOGE("Write length does not fit into device\n");
        goto closeall;
    }
#else
    if ((w_length + w_offset) > mtd->size) {
        LOGE("MTD name \"%s\" overlow, write %lld bytes, page %d bytes, OOB area %d"
             " bytes, device size %lld bytes\n",
             MTD_DEV_INFO_TO_PATH(mtd), w_length, pagelen, oobsize, mtd->size);
        LOGE("Write length does not fit into device\n");
        goto closeall;
    }
#endif
    if (noecc)
        write_mode = MTD_OPS_RAW;
    else if (autoplace)
        write_mode = MTD_OPS_AUTO_OOB;
    else
        write_mode = MTD_OPS_PLACE_OOB;
#ifdef MTD_OPEN_DEBUG
    LOGI("MTD \"%s\" write at 0x%llx, totally %lld bytes is starting\n",
         MTD_DEV_INFO_TO_PATH(mtd), w_offset, w_length);
#endif
    while (w_length > 0 && w_offset < mtd->size) {
        while (blockstart != MTD_BLOCK_ALIGN(mtd, w_offset)) {
            blockstart = MTD_BLOCK_ALIGN(mtd, w_offset);
#ifdef MTD_OPEN_DEBUG
            LOGI("Writing data to block %lld at offset 0x%llx\n",
                 blockstart / mtd->eb_size, w_offset);
#endif
            if (!is_nand)
                continue;
            do {
                ret = mtd_is_bad(mtd, *fd, MTD_OFFSET_TO_EB_INDEX(mtd, w_offset));
                if (ret == -1) {
                    LOGE("mtd block %lld bad detecting wrong\n", MTD_OFFSET_TO_EB_INDEX(mtd, w_offset));
                    goto closeall;
                }
                if (ret) {
                    if (errno == EOPNOTSUPP) {
                        LOGE("Bad block check not available on MTD \"%s\"",
                             MTD_DEV_INFO_TO_PATH(mtd));
                        goto closeall;
                    }
                    LOGI("Block%lld is bad\n", MTD_OFFSET_TO_EB_INDEX(mtd, w_offset));
                    w_offset += mtd->eb_size;
                    w_offset = MTD_BLOCK_ALIGN(mtd, w_offset);
                    continue;
                }
                break;
            } while (w_offset < mtd->size);

            if (w_offset >= mtd->size) {
                LOGE("write boundary is overlow\n");
                goto closeall;
            }
            blockstart = MTD_BLOCK_ALIGN(mtd, w_offset);
            continue;
        }
        writen = MIN(w_length, pagelen);
        if ((writen > mtd->min_io_size)
                && writeoob) {
            oobbuf = writeoob ? w_buffer + mtd->min_io_size : NULL;
        } else {
            oobbuf = NULL;
            writeoob = 0;
        }

        int64_t flash_in_len = MIN(writen, mtd->min_io_size);
#ifdef BM_SYSINFO_SUPPORT
        if (bm->sysinfo) {
            ret = bm->sysinfo->traversal_merge(bm->sysinfo, w_buffer,
                                               w_offset + MTD_DEV_INFO_TO_START(mtd),
                                               flash_in_len);
            if (ret < 0) {
                LOGE("MTD \"%s\" failed to merge system info\n", MTD_DEV_INFO_TO_PATH(mtd));
                goto closeall;
            }
        }
#endif
        ret = mtd_write(mtd_desc, mtd, *fd, MTD_OFFSET_TO_EB_INDEX(mtd, w_offset),
                        w_offset % mtd->eb_size,
                        w_buffer,
                        flash_in_len,
                        writeoob ? oobbuf : NULL,
                        writeoob ? oobsize : 0,
                        write_mode);
        if (ret) {
            if (errno != EIO) {
                LOGE("MTD \"%s\" write failure\n", MTD_DEV_INFO_TO_PATH(mtd));
                goto closeall;
            }
            LOGW("Erasing failed write at 0x%llx\n", MTD_OFFSET_TO_EB_INDEX(mtd, w_offset));
            if (!is_nand)
                goto closeall;
            if (mtd_erase(mtd_desc, mtd, *fd, MTD_OFFSET_TO_EB_INDEX(mtd, w_offset))) {
                int errno_tmp = errno;
                LOGE("MTD \"%s\" Erase failure\n", MTD_DEV_INFO_TO_PATH(mtd));
                if (errno_tmp != EIO)
                    goto closeall;
            }
            if (markbad) {
                LOGW("Marking block at %llx bad\n", MTD_BLOCK_ALIGN(mtd, w_offset));
                if (mtd_mark_bad(mtd, *fd, MTD_OFFSET_TO_EB_INDEX(mtd, w_offset))) {
                    LOGE("MTD \"%s\" Mark bad block failure\n", MTD_DEV_INFO_TO_PATH(mtd));
                    goto closeall;
                }
            }
            w_offset += mtd->eb_size;
            w_offset = MTD_BLOCK_ALIGN(mtd, w_offset);
            continue;
        }
        w_offset += flash_in_len;
        w_buffer += writen;
        w_length -= writen;
    }
    return w_offset + mtd_start;
closeall:
    LOGE("%s has crashed\n", __func__);
    if (*fd) {
        close(*fd);
        *fd = 0;
    }
    return -1;
}

int64_t mtd_basic_read(struct filesystem *fs) {
    struct mtd_dev_info *mtd = FS_GET_MTD_DEV(fs);
    int *fd = &MTD_DEV_INFO_TO_FD(mtd);
    long long mtd_start, length, offset, blockstart = -1, read_unit = 0;
    char *buffer;
    int is_nand = mtd_type_is_nand(mtd);
    int ret;

    length = fs->params->length;
    buffer = fs->params->buf;
    offset = fs->params->offset;
    mtd_start = MTD_DEV_INFO_TO_START(mtd);
    offset -= mtd_start;

#ifdef MTD_OPEN_DEBUG
    LOGI("MTD \"%s\" read at 0x%llx, totally %lld bytes is starting\n",
         MTD_DEV_INFO_TO_PATH(mtd), offset, length);
#endif
    while (length > 0 && offset < mtd->size) {
        while (blockstart != MTD_BLOCK_ALIGN(mtd, offset)) {
            blockstart = MTD_BLOCK_ALIGN(mtd, offset);
            // LOGI("Reading data from block %lld at offset 0x%llx\n",
            //      blockstart / mtd->eb_size, offset);
            if (!is_nand)
                continue;
            do {
                ret = mtd_is_bad(mtd, *fd, MTD_OFFSET_TO_EB_INDEX(mtd, offset));
                if (ret == -1) {
                    LOGE("mtd block %lld bad detecting wrong\n", MTD_OFFSET_TO_EB_INDEX(mtd, offset));
                    goto closeall;
                }
                if (ret) {
                    if (errno == EOPNOTSUPP) {
                        LOGE("Bad block check not available on MTD \"%s\"",
                             MTD_DEV_INFO_TO_PATH(mtd));
                        goto closeall;
                    }
                    LOGI("Block%lld is bad\n", MTD_OFFSET_TO_EB_INDEX(mtd, offset));
                    offset += mtd->eb_size;
                    offset = MTD_BLOCK_ALIGN(mtd, offset);
                    continue;
                }
                break;
            } while (offset < mtd->size);

            if (offset >= mtd->size) {
                LOGE("read boundary is overlow\n");
                goto closeall;
            }
            blockstart = MTD_BLOCK_ALIGN(mtd, offset);
            continue;
        }
        read_unit = ((length > (mtd->eb_size - (offset % mtd->eb_size))) ?
                     (mtd->eb_size - (offset % mtd->eb_size)) : length);
#ifdef MTD_OPEN_DEBUG
        LOGI("Reading data at offset 0x%llx with size %lld\n", offset, read_unit);
#endif
        ret = mtd_read(mtd, *fd, MTD_OFFSET_TO_EB_INDEX(mtd, offset),
                       offset % mtd->eb_size, buffer, read_unit);
        if (ret) {
            LOGE("MTD \"%s\" read failure at address 0x%llx with length %lld\n",
                 MTD_DEV_INFO_TO_PATH(mtd), offset, length);
            goto closeall;
        }
        length -= read_unit;
        buffer += read_unit;
        offset += read_unit;
    }

    return offset + mtd_start;
closeall:
    LOGE("%s has crashed\n", __func__);
    if (*fd) {
        close(*fd);
        *fd = 0;
    }
    return -1;
}
