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

#include <errno.h>
#include <limits.h>
#include <unistd.h>
#include <string.h>
#include <dlfcn.h>

#include <utils/common.h>
#include <utils/log.h>
#include "hardware.h"

#define LOG_TAG "HAL"

#define HAL_LIBRARY_PATH1 "/lib"
#define HAL_LIBRARY_PATH2 "/lib/hw"
#define HAL_LIBRARY_PATH3 "/usr/lib"
#define HAL_LIBRARY_PATH4 "/usr/lib/hw"

static const char* vendor_values[] = {
        "microarray",
        "goodix",
        "default"
};

static int load(const char *id, const char *path,
        const struct hw_module_t **pHmi) {

    int status = -EINVAL;
    void* handle = NULL;
    struct hw_module_t *hmi = NULL;

    handle = dlopen(path, RTLD_NOW);
    if (handle == NULL) {
        char const* err_str = dlerror();
        LOGE("load: module=%s: %s\n", path, err_str ? err_str : "unknown");
        status = -EINVAL;
        goto done;
    }

    const char* sym = HAL_MODULE_INFO_SYM_AS_STR;
    hmi = (struct hw_module_t *)dlsym(handle, sym);
    if (hmi == NULL) {
        LOGE("load: couldn't find symbol %s\n", sym);
        status = -EINVAL;
        goto done;
    }

    if (strcmp(id, hmi->id) != 0) {
        LOGE("load: id=%s != hmi->id=%s\n", id, hmi->id);
        status = -EINVAL;
        goto done;
    }

    hmi->dso = handle;

    status = 0;

done:
    if (status != 0) {
        hmi = NULL;
        if (handle != NULL) {
            dlclose(handle);
            handle = NULL;
        }
    } else {
        LOGV("loaded HAL id=%s path=%s hmi=%p handle=%p\n", id, path, *pHmi,
                handle);
    }

    *pHmi = hmi;

    return status;
}

static int hw_module_exists(char *path, size_t path_len, const char *name,
        const char* subname) {

    snprintf(path, path_len, "%s/%s.%s.so", HAL_LIBRARY_PATH2, name, subname);
    LOGI("path=%s\n", path);
    if (access(path, R_OK) == 0)
        return 0;

    snprintf(path, path_len, "%s/%s.%s.so", HAL_LIBRARY_PATH1, name, subname);
    LOGI("path=%s\n", path);
    if (access(path, R_OK) == 0)
        return 0;

    snprintf(path, path_len, "%s/%s.%s.so", HAL_LIBRARY_PATH4, name, subname);
    LOGI("path=%s\n", path);
    if (access(path, R_OK) == 0)
        return 0;

    snprintf(path, path_len, "%s/%s.%s.so", HAL_LIBRARY_PATH3, name, subname);
    LOGI("path=%s\n", path);
    if (access(path, R_OK) == 0)
        return 0;

    return -ENOENT;
}

int hw_get_module_by_class(const char *class_id, const char *inst,
                           const struct hw_module_t **module) {
    char path[PATH_MAX] = {0};
    char name[PATH_MAX] = {0};

    if (inst)
        snprintf(name, PATH_MAX, "%s.%s", class_id, inst);
    else
        strncpy(name, class_id, PATH_MAX);

    for (int i = 0; i < ARRAY_SIZE(vendor_values); i++) {
        if (hw_module_exists(path, sizeof(path), name, vendor_values[i]) == 0)
            return load(class_id, path, module);
    }

    return -ENOENT;
}

int hw_get_module(const char *id, const struct hw_module_t **module) {
    return hw_get_module_by_class(id, NULL, module);
}
