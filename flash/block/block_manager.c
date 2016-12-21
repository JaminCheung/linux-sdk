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
#include <utils/log.h>
#include <utils/assert.h>
#include <utils/common.h>
#include <lib/mtd/mtd-user.h>
#include <utils/list.h>
#include <flash/block/sysinfo/sysinfo_manager.h>
#include <flash/block/block_manager.h>

#define LOG_TAG "block_manager"

extern int mtd_manager_init(void);
extern int mtd_manager_destroy(void);
extern int mmc_manager_init(void);

static LIST_HEAD(block_manager_list);

unsigned long recovery_errorno;

int register_block_manager(struct block_manager* this) {
    struct block_manager *m;
    struct list_head *cell;

    list_for_each(cell, &block_manager_list) {
        m = list_entry(cell, struct block_manager, list_cell);
        if (!strcmp(m->name,  this->name)) {
            LOGE("Block manager \'%s\' is already registered\n", m->name);
            return -1;
        }
    }

    list_add_tail(&this->list_cell, &block_manager_list);

    LOGD("Block manager \'%s\' is registered\n", this->name);

    return 0;
}

int unregister_block_manager(struct block_manager* this) {
    struct block_manager *m;
    struct list_head *cell;
    struct list_head* next;

    list_for_each_safe(cell, next, &block_manager_list) {
        m = list_entry(cell, struct block_manager, list_cell);
        if (!strcmp(m->name,  this->name)) {
            LOGD("Block manager \'%s\' is removed successfully\n", m->name);
            list_del(cell);

            return 0;
        }
    }

    return -1;
}

struct block_manager* get_block_manager(struct block_manager* this,
        const char *name) {

    struct block_manager *m;
    struct list_head *cell;

    list_for_each(cell, &block_manager_list) {
        m = list_entry(cell, struct block_manager, list_cell);
        if (!strcmp(m->name,  name))
            return m;
    }

    return NULL;
}

static void get_supported_filetype(struct block_manager* this, char *buf) {
    BM_MTD_FILE_TYPE_INIT(supported_array);
    char list[128];

    for (int i = 0; i < ARRAY_SIZE(supported_array); i++) {
        if (i == 0)
            strcpy(list, "[");
        else
            strcat(list, ",");

        strcat(list,  supported_array[i]);
    }

    strcat(list, "]");
    strcpy(buf, list);
}

static int set_operation_option(struct block_manager* this,
        struct bm_operation_option* option, int method, char *filetype) {

    if (option == NULL) {
        LOGE("Parameter \"option\" is null\n");
        goto out;
    }

    option->method = method;
    if (strlen(filetype) > sizeof(option->filetype) - 1) {
        LOGE("Parameter filetype is too long\n");
        goto out;
    }

    strcpy(option->filetype, filetype);

    return 0;

out:
    return -1;
}

static const char* operation_to_string(enum bm_operation op) {
    switch(op) {
    case BM_OPERATION_ERASE:
        return "Earse";

    case BM_OPERATION_WRITE:
        return "Write";

    case BM_OPERATION_READ:
        return "Read";

    case BM_OPERATION_ERASE_WRITE:
        return "Earse write";

    case BM_OPERATION_FORMAT:
        return "Format";

    default:
        return "Unknown";
    }
}

static void dump_event(struct block_manager* this, struct bm_event* event) {
    LOGD("=============================\n");
    LOGD("Dump bm event\n");
    LOGD("part name: %s\n", event->part_name);
    LOGD("operation: %s\n", operation_to_string(event->operation));
    LOGD("progress:  %d\n", event->progress);
    LOGD("=============================\n");
}

void construct_block_manager(struct block_manager* this, const char *blockname,
        bm_event_listener_t listener, void* param) {

    int retval;
    retval = mtd_manager_init();
    // retval &= mmc_manager_init();

    if (retval < 0) {
        LOGE("Failed to init mtd block manager\n");
        goto out;
    }

    struct block_manager* bm = get_block_manager(this, blockname);
    if (bm == NULL) {
        LOGE("Block manager you request is not exist\n");
        goto out;
    }

    bm->param = param;
    BM_GET_LISTENER(bm) = listener;
    memcpy(this, bm, sizeof(*this));

    this->construct = construct_block_manager;
    this->destruct = destruct_block_manager;
    this->get_supported_filetype = get_supported_filetype;
    this->set_operation_option = set_operation_option;
    this->dump_event = dump_event;

#ifdef BM_SYSINFO_SUPPORT
    if (get_system_platform() == XBURST)
        sysinfo_manager_bind(GET_SYSINFO_MANAGER(), this);
    else
        bm->sysinfo = NULL;
#endif

    return;

out:
    assert_die_if(1, "%s:%s:%d run crashed\n", __FILE__, __func__, __LINE__);
}

void destruct_block_manager(struct block_manager* this) {
    int retval;

#ifdef BM_SYSINFO_SUPPORT
    if (this->sysinfo->exit(this->sysinfo) < 0) {
        LOGE("Failed to issue sysinfo exit\n");
        goto out;
    }
#endif
    // mmc_manager_destroy();
    retval = mtd_manager_destroy();
    if (retval < 0) {
        LOGE("Failed to destory mtd block manager\n");
        goto out;
    }
    memset(this, 0, sizeof(* this));

    return;

out:
    assert_die_if(1, "%s:%s:%d run crashed\n", __FILE__, __func__, __LINE__);
}






