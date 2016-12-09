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

#ifndef CONFIGURE_FILE_H
#define CONFIGURE_FILE_H

struct object {
    char *key;
    char *value;
};

struct bootloader_config {
    struct object name;
    struct object file_path;
    struct object md5;
};

struct kernel_config {
    struct object name;
    struct object file_path;
    struct object md5;
};

struct splash_config {
    struct object name;
    struct object file_path;
    struct object md5;
};

struct rootfs_config {
    struct object name;
    struct object full_upgrade;
    struct object file_path;
    struct object md5;
};

struct userfs_config {
    struct object name;
    struct object full_upgrade;
    struct object file_path;
    struct object md5;
};

struct config {
    struct bootloader_config *bootloader;
    struct kernel_config *kernel;
    struct splash_config* splash;
    struct rootfs_config *rootfs;
    struct userfs_config *userfs;
};

struct configure_data {
    char bootloader_name[NAME_MAX];
    bool bootloader_upgrade;
    char bootloader_path[PATH_MAX];
    char bootloader_md5[PATH_MAX];

    char kernel_name[NAME_MAX];
    bool kernel_upgrade;
    char kernel_path[PATH_MAX];
    char kernel_md5[PATH_MAX];

    char splash_name[NAME_MAX];
    bool splash_upgrade;
    char splash_path[PATH_MAX];
    char splash_md5[PATH_MAX];

    char rootfs_name[NAME_MAX];
    bool rootfs_upgrade;
    bool rootfs_full_upgrade;
    char rootfs_path[PATH_MAX];
    char rootfs_md5[PATH_MAX];

    char userfs_name[NAME_MAX];
    bool userfs_upgrade;
    bool userfs_full_upgrade;
    char userfs_path[PATH_MAX];
    char userfs_md5[PATH_MAX];
};

struct configure_file {
    void (*construct)(struct configure_file* this);
    void (*destruct)(struct configure_file* this);
    struct configure_data* (*load_configure_file)(struct configure_file* this,
            const char* file_name);
    void (*dump)(struct configure_file* this);
    struct configure_data* data;
};

void construct_configure_file(struct configure_file* this);
void destruct_configure_file(struct configure_file* this);

#endif /* CONFIGURE_FILE_H */
