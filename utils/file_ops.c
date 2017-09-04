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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>
#include <limits.h>
#include <fcntl.h>

#include <utils/log.h>
#include <utils/assert.h>
#include <openssl/md5.h>

#define LOG_TAG "file_ops"

#define READ_DATA_SIZE  1024
#define MD5_SIZE        16
#define MD5_STR_LEN     (MD5_SIZE * 2)

int file_exist(const char *path) {

    assert_die_if(path == NULL, "path is NULL");

    if (access(path, F_OK | R_OK))
        return -errno;

    return 0;
}

int file_executable(const char *path) {

    assert_die_if(path == NULL, "path is NULL");

    if (access(path, F_OK | R_OK | X_OK))
        return -errno;

    return 0;
}

int get_file_size(const char *path) {
    struct stat s;
    if(stat(path, &s) < 0)
        return -1;
    return s.st_size;
}
#if 0
int check_file_md5(const char* path, const char* md5) {
    int fd = 0;
    int i;
    int retval = 0;
    unsigned char buf[READ_DATA_SIZE] = { 0 };
    unsigned char md5_value[MD5_SIZE] = { 0 };
    char md5_str[MD5_STR_LEN + 1] = { 0 };

    MD5_CTX md5_ctx;

    fd = open(path, O_RDONLY);
    if (fd < 0)
        return -1;

    MD5_Init(&md5_ctx);

    for (;;) {
        retval = read(fd, buf, sizeof(buf));
        if (retval < 0) {
            LOGE("Failed to read %s: %s\n", path, strerror(errno));
            goto error;
        }

        MD5_Update(&md5_ctx, buf, retval);

        if (retval == 0 || retval < READ_DATA_SIZE)
            break;
    }

    close(fd);

    MD5_Final(md5_value, &md5_ctx);

    for (i = 0; i < MD5_SIZE; i++)
        snprintf(md5_str + i * 2, 2 + 1, "%02x", md5_value[i]);

    md5_str[MD5_STR_LEN] = '\0';

    if (!strcmp(md5_str, md5))
        return 0;

    return -1;

error:
    close(fd);

    return -1;
}
#endif

int dir_exist(const char* path) {
    assert_die_if(path == NULL, "path is NULL");

    DIR* dp;

    dp = opendir(path);
    if (dp == NULL)
        return -1;

    closedir(dp);

    return 0;
}

int dir_create(const char* path) {
    char *buffer;
    char *p;

    assert_die_if(path == NULL, "path is NULL\n");

    int len = (int) strlen(path);

    if (len <= 0)
        return -1;

    buffer = (char*) calloc(1, len + 1);
    if (buffer == NULL)
        return -1;

    strcpy(buffer, path);

    if (buffer[len - 1] == '/')
        buffer[len - 1] = '\0';

    if (mkdir(buffer, 0775) == 0) {
        free(buffer);
        return 0;
    }

    p = buffer + 1;

    while (1) {
        char hold;
        while (*p && *p != '\\' && *p != '/')
            p++;

        hold = *p;
        *p = 0;
        if ((mkdir(buffer, 0775) == -1) && (errno == ENOENT)) {
            free(buffer);
            return -1;
        }
        if (hold == 0)
            break;
        *p++ = hold;
    }
    free(buffer);

    return 0;
}

int dir_delete(const char *path) {
    struct stat st;
    DIR *dir;
    struct dirent *de;
    int fail = 0;

    assert_die_if(path == NULL, "path is NULL");

    /* is it a file or directory? */
    if (lstat(path, &st) < 0)
        return -1;

    /* a file, so unlink it */
    if (!S_ISDIR(st.st_mode))
        return unlink(path);

    /* a directory, so open handle */
    dir = opendir(path);
    if (dir == NULL)
        return -1;

    /* recurse over components */
    errno = 0;
    while ((de = readdir(dir)) != NULL) {
        char dn[PATH_MAX];
        if (!strcmp(de->d_name, "..") || !strcmp(de->d_name, "."))
            continue;

        snprintf(dn, sizeof(dn), "%s/%s", path, de->d_name);
        if (dir_delete(dn) < 0) {
            fail = 1;
            break;
        }
        errno = 0;
    }

    /* in case readdir or unlink_recursive failed */
    if (fail || errno < 0) {
        int save = errno;
        closedir(dir);
        errno = save;
        return -1;
    }

    /* close directory handle */
    if (closedir(dir) < 0)
        return -1;

    /* delete target directory */
    return rmdir(path);
}
