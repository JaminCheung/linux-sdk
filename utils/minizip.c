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

#include <sys/stat.h>
#include <sys/types.h>
#include <limits.h>
#include <string.h>
#include <unistd.h>

#include <utils/log.h>
#include <utils/assert.h>
#include <utils/minizip.h>
#include <lib/zip/minizip/zip.h>
#include <lib/zip/minizip/unzip.h>

#define WRITEBUFFERSIZE (8192)

#define LOG_TAG "minizip"

static int do_extract_currentfile(unzFile uf, const int* junk_path,
        const char* password) {
    char filename_inzip[256];
    char* filename_withoutpath;
    char* p;
    int err = UNZ_OK;
    FILE *fout = NULL;
    void* buf;
    uInt size_buf;

    unz_file_info64 file_info;
    err = unzGetCurrentFileInfo64(uf, &file_info, filename_inzip,
            sizeof(filename_inzip), NULL, 0, NULL, 0);

    if (err != UNZ_OK) {
        LOGE("error %d with zipfile in unzGetCurrentFileInfo\n", err);
        return err;
    }

    size_buf = WRITEBUFFERSIZE;
    buf = (void*) calloc(1, size_buf);
    if (buf == NULL) {
        LOGE("Error allocating memory\n");
        return UNZ_INTERNALERROR;
    }

    p = filename_withoutpath = filename_inzip;
    while ((*p) != '\0') {
        if (((*p) == '/') || ((*p) == '\\'))
            filename_withoutpath = p + 1;
        p++;
    }

    if ((*filename_withoutpath) == '\0') {
        if ((*junk_path) == 0) {
            LOGI("creating directory: %s\n", filename_inzip);
            mkdir(filename_inzip, 0755);
        }

    } else {
        const char* write_filename;

        if ((*junk_path) == 0)
            write_filename = filename_inzip;
        else
            write_filename = filename_withoutpath;

        err = unzOpenCurrentFilePassword(uf, password);
        if (err != UNZ_OK)
            LOGE("error %d with zipfile in unzOpenCurrentFilePassword\n", err);

        if (err == UNZ_OK) {
            fout = fopen(write_filename, "wb");
            /* some zipfile don't contain directory alone before file */
            if ((fout == NULL) && ((*junk_path) == 0)
                    && (filename_withoutpath != (char*) filename_inzip)) {
                char c = *(filename_withoutpath - 1);
                *(filename_withoutpath - 1) = '\0';
                mkdir(write_filename, 0755);
                *(filename_withoutpath - 1) = c;
                fout = fopen(write_filename, "wb");
            }

            if (fout == NULL) {
                LOGE("error opening %s\n", write_filename);
            }
        }

        if (fout != NULL) {
            LOGD(" extracting: %s\n", write_filename);

            do {
                err = unzReadCurrentFile(uf, buf, size_buf);
                if (err < 0) {
                    LOGE("error %d with zipfile in unzReadCurrentFile\n", err);
                    break;
                }
                if (err > 0)
                    if (fwrite(buf, err, 1, fout) != 1) {
                        LOGE("error in writing extracted file\n");
                        err = UNZ_ERRNO;
                        break;
                    }
            } while (err > 0);

            if (fout)
                fclose(fout);
        }

        if (err == UNZ_OK) {
            err = unzCloseCurrentFile(uf);
            if (err != UNZ_OK) {
                LOGE("error %d with zipfile in unzCloseCurrentFile\n", err);
            }
        } else
            unzCloseCurrentFile(uf); /* don't lose the error */
    }

    free(buf);

    return err;
}

static int do_extract(unzFile uf, int junk_path, const char* password) {
    uLong i;
    unz_global_info64 gi;
    int error = 0;

    error = unzGetGlobalInfo64(uf, &gi);
    if (error != UNZ_OK) {
        LOGE("error %d with zipfile in unzGetGlobalInfo\n", error);
        return -1;
    }

    for (i = 0; i < gi.number_entry; i++) {
        if (do_extract_currentfile(uf, &junk_path,
                password) != UNZ_OK) {
            error = -1;
            break;
        }

        if ((i + 1) < gi.number_entry) {
            error = unzGoToNextFile(uf);
            if (error != UNZ_OK) {
                LOGE("error %d with zipfile in unzGoToNextFile\n", error);
                error = -1;
                break;
            }
        }
    }

    return error;
}

int unzip(const char* path, const char* dir, const char* password,
        int junk_path) {
    assert_die_if(path == NULL, "path is NULL");

    int error = 0;
    unzFile uf = NULL;
    char filename_try[PATH_MAX] = {0};

    strcpy(filename_try, path);

    uf = unzOpen64(path);
    if (uf == NULL) {
        strcat(filename_try, ".zip");
        uf = unzOpen64(filename_try);
    }

    if (uf == NULL) {
        LOGE("Failed to open %s or %s.zip\n", path, path);
        return -1;
    }

    if (dir != NULL) {
        if (chdir(dir) < 0) {
            LOGE("Failed to changing into %s: %s\n", dir, strerror(errno));
            return -1;
        }
    }

    error = do_extract(uf, junk_path, password);

    unzClose(uf);

    return error;
}
