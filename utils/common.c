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

#include <sys/time.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <errno.h>

#include <types.h>
#include <utils/log.h>
#include <utils/assert.h>
#include <utils/file_ops.h>
#include <utils/common.h>

#define LOG_TAG "common"

static const char* prefix_platform_xburst = "Ingenic Xburst";

void msleep(uint64_t msec) {
    struct timespec ts;
    int err;

    ts.tv_sec = (msec / 1000);
    ts.tv_nsec = (msec % 1000) * 1000 * 1000;

    do {
        err = nanosleep(&ts, &ts);
    } while (err < 0 && errno == EINTR);
}


enum system_platform_t get_system_platform(void) {
    FILE* fp = NULL;
    char line[256] = {0};

    fp = fopen("/proc/cpuinfo", "r");
    if (fp == NULL) {
        LOGE("Failed to open /proc/cpuinfo: %s\n", strerror(errno));
        return UNKNOWN;
    }

    while (fgets(line, sizeof(line), fp)) {
        if (strstr(line, prefix_platform_xburst)) {
            return XBURST;
        }
    }

    return UNKNOWN;
}

static void do_cold_boot(DIR *d, int lvl) {
    struct dirent *de;
    int dfd, fd;

    dfd = dirfd(d);

    fd = openat(dfd, "uevent", O_WRONLY);
    if (fd >= 0) {
        if (write(fd, "add\n", 4) < 0)
            close(fd);
        else
            close(fd);
    }

    while ((de = readdir(d))) {
        DIR *d2;

        if (de->d_name[0] == '.')
            continue;

        if (de->d_type != DT_DIR && lvl > 0)
            continue;

        fd = openat(dfd, de->d_name, O_RDONLY | O_DIRECTORY);
        if (fd < 0)
            continue;

        d2 = fdopendir(fd);
        if (d2 == 0)
            close(fd);
        else {
            do_cold_boot(d2, lvl + 1);
            closedir(d2);
        }
    }
}

void cold_boot(const char *path) {
    DIR *d = opendir(path);
    if (d) {
        do_cold_boot(d, 0);
        closedir(d);
    }
}

#if 0
/**
 * get_multiplier - convert size specifier to an integer multiplier.
 * @str: the size specifier string
 *
 * This function parses the @str size specifier, which may be one of
 * 'KiB', 'MiB', or 'GiB' into an integer multiplier. Returns positive
 * size multiplier in case of success and %-1 in case of failure.
 */
int get_multiplier(const char *str) {
    if (!str)
        return 1;

    /* Remove spaces before the specifier */
    while (*str == ' ' || *str == '\t')
        str += 1;

    if (!strcmp(str, "KiB"))
        return 1024;
    if (!strcmp(str, "MiB"))
        return 1024 * 1024;
    if (!strcmp(str, "GiB"))
        return 1024 * 1024 * 1024;

    return -1;
}

/**
 * ubiutils_get_bytes - convert a string containing amount of bytes into an
 * integer
 * @str: string to convert
 *
 * This function parses @str which may have one of 'KiB', 'MiB', or 'GiB'
 * size specifiers. Returns positive amount of bytes in case of success and %-1
 * in case of failure.
 */
long long get_bytes(const char *str) {
    char *endp;
    long long bytes = strtoull(str, &endp, 0);

    if (endp == str || bytes < 0) {
        LOGE("incorrect amount of bytes: \"%s\"\n", str);
        return -1;
    }

    if (*endp != '\0') {
        int mult = get_multiplier(endp);

        if (mult == -1) {
            LOGE("bad size specifier: \"%s\" - "
                    "should be 'KiB', 'MiB' or 'GiB'\n", endp);
            return -1;
        }
        bytes *= mult;
    }

    return bytes;
}

/**
 * ubiutils_print_bytes - print bytes.
 * @bytes: variable to print
 * @bracket: whether brackets have to be put or not
 *
 * This is a helper function which prints amount of bytes in a human-readable
 * form, i.e., it prints the exact amount of bytes following by the approximate
 * amount of Kilobytes, Megabytes, or Gigabytes, depending on how big @bytes
 * is.
 */
void print_bytes(long long bytes, int bracket) {
    const char *p;

    if (bracket)
        p = " (";
    else
        p = ", ";

    LOGI("%lld bytes", bytes);

    if (bytes > 1024 * 1024 * 1024)
        LOGI("%s%.1f GiB", p, (double)bytes / (1024 * 1024 * 1024));
    else if (bytes > 1024 * 1024)
        LOGI("%s%.1f MiB", p, (double)bytes / (1024 * 1024));
    else if (bytes > 1024 && bytes != 0)
        LOGI("%s%.1f KiB", p, (double)bytes / 1024);
    else
        return;

    if (bracket)
        LOGI(")");
}
#endif
