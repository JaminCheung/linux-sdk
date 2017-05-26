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

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/mount.h>

#include <utils/log.h>
#include <utils/list.h>
#include <utils/assert.h>
#include <utils/file_ops.h>
#include <mount/mount_manager.h>

#define LOG_TAG "mount_manager"

#define PROC_MOUNTS_FILENAME "/proc/mounts"

static pthread_mutex_t list_lock = PTHREAD_MUTEX_INITIALIZER;

LIST_HEAD(mounted_list);

const char* supported_filesystem_list[] = {
        "vfat",
        "ntfs",
        "ext4",
        "exfat",
        NULL
};

static void dump_mounted_volumes(void) {
    struct list_head* pos;

    pthread_mutex_lock(&list_lock);

    LOGI("========================================\n");
    LOGI("Dump mounted volumes\n");
    list_for_each(pos, &mounted_list) {
        struct mounted_volume* volume = list_entry(pos,
                struct mounted_volume, head);

        LOGI("----------------------------------------\n");
        LOGI("device:      %s\n", volume->device);
        LOGI("mount point: %s\n", volume->mount_point);
        LOGI("file system: %s\n", volume->filesystem);
        LOGI("flags        %s\n", volume->flags);
    }
    LOGI("========================================\n");

    pthread_mutex_unlock(&list_lock);
}

static void scan_mounted_volumes(void) {
    char line[2048] = {0};
    FILE* fp = NULL;

    fp = fopen(PROC_MOUNTS_FILENAME, "r");

    assert_die_if(fp == NULL, "Failed to open %s: %s\n",
            PROC_MOUNTS_FILENAME, strerror(errno));

    /*
     * Clear list first
     */
    pthread_mutex_lock(&list_lock);

    if (!list_empty(&mounted_list)) {
        struct list_head* pos;
        struct list_head* next_pos;

        list_for_each_safe(pos, next_pos, &mounted_list) {
            struct mounted_volume* volume =
                    list_entry(pos, struct mounted_volume, head);

            list_del(&volume->head);
            free(volume);
        }
    }

    /*
     * Scan /proc/mounts
     */
    while (fgets(line, sizeof(line), fp)) {
        int matches;

        struct mounted_volume* volume = calloc(1,
                sizeof(struct mounted_volume));

        matches = sscanf(line, "%63s %63s %63s %127s",
                volume->device, volume->mount_point,
                volume->filesystem, volume->flags);

        if (matches != 4) {
            LOGW("Failed to parse line %s\n", line);
            free(volume);
            continue;
        }

        list_add_tail(&volume->head, &mounted_list);
    }

    pthread_mutex_unlock(&list_lock);

    fclose(fp);
}

static struct mounted_volume* find_mounted_volume_by_device(const char* device) {
    assert_die_if(device == NULL, "device is NULL\n");

    struct list_head* pos;

    pthread_mutex_lock(&list_lock);

    list_for_each(pos, &mounted_list) {
        struct mounted_volume* volume = list_entry(pos,
                struct mounted_volume, head);
        if (volume->device != NULL) {
            if (!strcmp(volume->device, device)) {
                pthread_mutex_unlock(&list_lock);
                return volume;
            }
        }
    }

    pthread_mutex_unlock(&list_lock);

    return NULL;
}

static struct mounted_volume* find_mounted_volume_by_mount_point(const char* mount_point) {
    assert_die_if(mount_point == NULL, "mount_point is NULL\n");

    struct list_head* pos;

    pthread_mutex_lock(&list_lock);

    list_for_each(pos, &mounted_list) {

        struct mounted_volume* volume = list_entry(pos,
                struct mounted_volume, head);
        if (volume->mount_point != NULL) {
            if (!strcmp(volume->mount_point, mount_point)) {
                pthread_mutex_unlock(&list_lock);
                return volume;
            }
        }
    }

    pthread_mutex_unlock(&list_lock);

    return NULL;
}

static int do_umount(struct mounted_volume* volume) {
    assert_die_if(volume == NULL, "volume is NULL\n");

    int error = 0;

    if (!strcmp(volume->filesystem, "ramdisk"))
        return 0;

    error = umount(volume->mount_point);
    if (error < 0) {
        LOGE("Failed to umount \"%s\" from \"%s\": %s\n", volume->device,
                volume->mount_point, strerror(errno));
        return -1;
    }

    dir_delete(volume->mount_point);

    list_del(&volume->head);
    free(volume);

    return 0;
}

static int do_mount(const char* device, const char* mount_point,
        const char* filesystem) {
    int error = 0;

    if (!strcmp(filesystem, "ramdisk"))
        return 0;

    const struct mounted_volume* volume = find_mounted_volume_by_mount_point(mount_point);
    if (volume)
        return 0;

    if (dir_create(mount_point)) {
        LOGE("Failed to create mount point \"%s\": %s\n", mount_point,
                strerror(errno));
        return -1;
    }

    int i;
    for (i = 0; supported_filesystem_list[i]; i++) {
        const char* fs = supported_filesystem_list[i];
        if (!strcmp(fs, filesystem))
            break;
    }

    if (supported_filesystem_list[i] == NULL) {
        LOGE("Unsupport file system: \"%s\" for \"%s\"\n", filesystem ,
                mount_point);
        goto error;
    }

    if (!strcmp(filesystem, "ext4") ||
            !strcmp(filesystem, "vfat") ||
            !strcmp(filesystem, "ntfs") ||
            !strcmp(filesystem, "exfat")) {

        error = mount(device, mount_point, filesystem,
                MS_NOATIME | MS_NODEV | MS_NODIRATIME | MS_RDONLY, "");
        if (error < 0) {
            LOGE("Failed to mount \"%s\" to \"%s\" with \"%s\" : %s\n", device,
                    mount_point, filesystem, strerror(errno));
            goto error;
        }

    } else {
        LOGE("Unsupport mount \"%s\" yet\n", filesystem);
        goto error;
    }

    return 0;

error:
    dir_delete(mount_point);

    return -1;
}

static int mount_volume(const char* device, const char* mount_point) {
    assert_die_if(device == NULL, "device is NULL\n");
    assert_die_if(mount_point == NULL, "mount_point is NULL\n");

    int i = 0;
    int error = 0;

    scan_mounted_volumes();

    for (i = 0; supported_filesystem_list[i]; i++) {
        const char* filesystem = supported_filesystem_list[i];

        error = do_mount(device, mount_point, filesystem);
        if (!error)
            break;
    }

    if (supported_filesystem_list[i] == NULL) {
        LOGE("Faied to mount device \"%s\"\n", device);
        return -1;
    }

    scan_mounted_volumes();

    return 0;
}

static int umount_volume(struct mounted_volume* volume) {
    int error = 0;

    assert_die_if(volume == NULL, "volume is NULL\n");

    error = do_umount(volume);

    scan_mounted_volumes();

    return error;
}

static struct mount_manager this = {
        .dump_mounted_volumes = dump_mounted_volumes,
        .find_mounted_volume_by_device = find_mounted_volume_by_device,
        .find_mounted_volume_by_mount_point = find_mounted_volume_by_mount_point,
        .umount_volume = umount_volume,
        .mount_volume = mount_volume,
};

struct mount_manager* get_mount_manager(void) {
    return &this;
}
