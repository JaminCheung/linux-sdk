#include <limits.h>
#include <utils/log.h>
#include <utils/list.h>
#include <lib/mxml/mxml.h>

#define LOG_TAG "test_libmxml"

#define UPDATE_FILE "update.xml"
#define DEVICE_FILE "device.xml"
#define GLOBAL_FILE "global.xml"

struct image_info {
    char name[NAME_MAX];
    char fs_type[NAME_MAX];
    uint64_t offset;
    uint64_t size;
    uint32_t update_mode;
    uint32_t chunksize;
    uint32_t chunkcount;
    struct list_head head;
};

struct update_info {
    uint32_t devctl;
    char dev_type[NAME_MAX];
    uint32_t image_count;
    struct list_head image_list;
};

struct partition_info {
    char name[NAME_MAX];
    uint64_t offset;
    uint64_t size;
    char block_name[NAME_MAX];
    struct list_head head;
};

struct device_info {
    char type[NAME_MAX];
    uint64_t capacity;
    uint32_t partition_count;
    struct list_head partition_list;
};

struct global_info {
    char *dev_type[4];
};

static mxml_type_t type_cb(mxml_node_t* node) {
    const char *type;

    if ((type = mxmlElementGetAttr(node, "type")) == NULL)
      type = node->value.element.name;

    if (!strcmp(type, "integer"))
      return (MXML_INTEGER);
    else if (!strcmp(type, "opaque") || !strcmp(type, "pre"))
      return (MXML_OPAQUE);
    else if (!strcmp(type, "real"))
      return (MXML_REAL);
    else
      return (MXML_TEXT);
}

struct update_info update_info;
struct device_info device_info;
struct global_info global_info;

int main(void) {
    FILE* fp = NULL;
    mxml_node_t *tree = NULL;
    mxml_node_t *node = NULL;
    mxml_node_t *sub_node = NULL;

    /* -------------- For global.xml ------------------- */
    fp = fopen(GLOBAL_FILE, "r");
    if (fp == NULL) {
        LOGE("Failed to open %s: %s\n", UPDATE_FILE, strerror(errno));
        return -1;
    }
    tree = mxmlLoadFile(NULL, fp, type_cb);
    fclose(fp);

    /*
     * test global node
     */
    node = mxmlFindElement(tree, tree, "global", NULL, NULL, MXML_DESCEND);
    if (node == NULL) {
        LOGE("Failed to find \"global\" element in %s\n", UPDATE_FILE);
        goto error;
    }

    /*
     * get image info
     */
    int count = 0;
    for (node = mxmlFindElement(node, tree, "device", NULL, NULL, MXML_DESCEND);
            node != NULL;
            node = mxmlFindElement(node, tree, "device", NULL, NULL,MXML_DESCEND)) {
        const char* dev_type = mxmlGetOpaque(node);

        global_info.dev_type[count++] = strdup(dev_type);
    }

    LOGI("=========================\n");
    LOGI("Dump %s\n", GLOBAL_FILE);
    for (int i = 0; i < count; i++)
        LOGI("devive type: %s\n", global_info.dev_type[i]);
    LOGI("=========================\n");

    /* -------------- For update.xml ------------------- */
    INIT_LIST_HEAD(&update_info.image_list);

    fp = fopen(UPDATE_FILE, "r");
    if (fp == NULL) {
        LOGE("Failed to open %s: %s\n", UPDATE_FILE, strerror(errno));
        return -1;
    }

    tree = mxmlLoadFile(NULL, fp, type_cb);
    fclose(fp);

    if (tree == NULL) {
        LOGE("Failed to load %s\n", UPDATE_FILE);
        return -1;
    }

    /*
     * test update node
     */
    node = mxmlFindElement(tree, tree, "update", NULL, NULL, MXML_DESCEND);
    if (node == NULL) {
        LOGE("Failed to find \"update\" element in %s\n", UPDATE_FILE);
        goto error;
    }

    const char* devctl_str = mxmlElementGetAttr(node, "devcontrol");
    if (devctl_str == NULL) {
        LOGE("Failed to find \"devcontrol\" attribute\n");
        goto error;
    }
    update_info.devctl = strtoll(devctl_str, NULL, 0);

    const char* devtype = mxmlElementGetAttr(node, "devtype");
    if (devtype == NULL) {
        LOGE("Failed to find \"devtype\" atribute\n");
        goto error;
    }
    memcpy(update_info.dev_type, devtype, strlen(devtype));

    /*
     * get imagelist count
     */
    node = mxmlFindElement(tree, tree, "imagelist", NULL, NULL, MXML_DESCEND);
    if (node == NULL) {
        LOGE("Failed to find \"imagelist\" element in %s\n", UPDATE_FILE);
        goto error;
    }

    const char* count_str =  mxmlElementGetAttr(node, "count");
    if (count_str == NULL) {
        LOGE("Failed to find \"count\" attribute in %s\n", UPDATE_FILE);
        goto error;
    }
    update_info.image_count = strtoul(count_str, NULL, 0);

    /*
     * get image info
     */
    for (node = mxmlFindElement(node, tree, "image", NULL, NULL, MXML_DESCEND);
            node != NULL;
            node = mxmlFindElement(node, tree, "image", NULL, NULL,MXML_DESCEND)) {

        struct image_info* image = malloc(sizeof(struct image_info));
        memset(image, 0, sizeof(struct image_info));

        /*
         * get name node
         */
        sub_node =  mxmlFindElement(node, node, "name", NULL, NULL,
                MXML_DESCEND);
        if (sub_node == NULL) {
            LOGE("Failed to find \"name\" element in %s\n", UPDATE_FILE);
            free(image);
            break;
        }
        const char* name = mxmlGetOpaque(sub_node);
        memcpy(image->name, name, strlen(name));

        /*
         * get fs type node
         */
        sub_node =  mxmlFindElement(node, node, "type", NULL, NULL,
                MXML_DESCEND);
        if (sub_node == NULL) {
            LOGE("Failed to find \"type\" element in %s\n", UPDATE_FILE);
            free(image);
            break;
        }
        const char* fs_type = mxmlGetOpaque(sub_node);
        memcpy(image->fs_type, fs_type, strlen(fs_type));

        /*
         * get offset node
         */
        sub_node =  mxmlFindElement(node, node, "offset", NULL, NULL,
                MXML_DESCEND);
        if (sub_node == NULL) {
            LOGE("Failed to find \"offset\" element in %s\n", UPDATE_FILE);
            free(image);
            break;
        }
        const char* offset_str = mxmlGetText(sub_node, 0);
        image->offset = strtoull(offset_str, NULL, 0);

        /*
         * get size node
         */
        sub_node =  mxmlFindElement(node, node, "size", NULL, NULL,
                MXML_DESCEND);
        if (sub_node == NULL) {
            LOGE("Failed to find \"size\" element in %s\n", UPDATE_FILE);
            free(image);
            break;
        }
        const char* size_str = mxmlGetText(sub_node, 0);
        image->size = strtoull(size_str, NULL, 0);

        /*
         * get update_mode node
         */
        sub_node =  mxmlFindElement(node, node, "updatemode", NULL, NULL,
                MXML_DESCEND);
        if (sub_node == NULL) {
            LOGE("Failed to find \"updatemode\" element in %s\n", UPDATE_FILE);
            free(image);
            break;
        }

        sub_node = mxmlFindElement(sub_node, sub_node, "type", NULL, NULL,
                MXML_DESCEND);
        if (sub_node == NULL) {
            LOGE("Failed to find \"type\" element in %s\n", UPDATE_FILE);
            free(image);
            break;
        }
        image->update_mode = mxmlGetInteger(sub_node);

        if (image->update_mode == 0x201) {
            sub_node = mxmlGetParent(sub_node);
            sub_node = mxmlFindElement(sub_node, sub_node, "chunksize", NULL, NULL,
                    MXML_DESCEND);
            if (sub_node == NULL) {
                LOGE("Failed to find \"chunksize\" element in %s\n", UPDATE_FILE);
                free(image);
                break;
            }
            image->chunksize = mxmlGetInteger(sub_node);

            sub_node = mxmlGetParent(sub_node);
            sub_node = mxmlFindElement(sub_node, sub_node, "chunkcount", NULL, NULL,
                    MXML_DESCEND);
            if (sub_node == NULL) {
                LOGE("Failed to find \"chunkcount\" element in %s\n", UPDATE_FILE);
                free(image);
                break;
            }
            image->chunkcount = mxmlGetInteger(sub_node);
        }

        list_add_tail(&image->head, &update_info.image_list);
    }

    LOGI("=========================\n");
    LOGI("Dump %s\n", UPDATE_FILE);
    LOGI("devctl: %d\n", update_info.devctl);
    LOGI("devtype: %s\n", update_info.dev_type);
    LOGI("imagelist count: %d\n", update_info.image_count);

    struct list_head* pos;
    struct image_info* image;
    list_for_each(pos, &update_info.image_list) {
        image = list_entry(pos, struct image_info, head);
        LOGI("-------------------------\n");
        LOGI("image name: %s\n", image->name);
        LOGI("image fs type: %s\n", image->fs_type);
        LOGI("image offset: 0x%x\n", (uint32_t) image->offset);
        LOGI("image size: %llu\n", image->size);
        LOGI("image update mode: 0x%x\n", image->update_mode);
        LOGI("image chunksize: %u\n", image->chunksize);
        LOGI("image chunkcount: %u\n", image->chunkcount);
    }

    LOGI("=========================\n");
    /* -------------- For update.xml end ------------------- */

    /* -------------- For device.xml ------------------- */
    INIT_LIST_HEAD(&device_info.partition_list);

    fp = fopen(DEVICE_FILE, "r");
    if (fp == NULL) {
        LOGE("Failed to open %s: %s\n\n", DEVICE_FILE, strerror(errno));
        return -1;
    }

    tree = mxmlLoadFile(NULL, fp, type_cb);
    fclose(fp);

    if (tree == NULL) {
        LOGE("Failed to load %s\n", DEVICE_FILE);
        return -1;
    }

    /*
     * test device node
     */
    node = mxmlFindElement(tree, tree, "device", NULL, NULL, MXML_DESCEND);
    if (node == NULL) {
        LOGE("Failed to find \"device\" element in %s\n", DEVICE_FILE);
        goto error;
    }

    const char* type = mxmlElementGetAttr(node, "type");
    memcpy(device_info.type, type, strlen(type));

    const char* capacity = mxmlElementGetAttr(node, "capacity");
    device_info.capacity = strtoull(capacity, NULL, 0);

    /*
     * get partition
     */
    node = mxmlFindElement(tree, tree, "partition", NULL, NULL, MXML_DESCEND);
    if (node == NULL) {
        LOGE("Failed to find \"partition\" element in %s\n", DEVICE_FILE);
        goto error;
    }
    count_str =  mxmlElementGetAttr(node, "count");
    if (count_str == NULL) {
        LOGE("Failed to find \"count\" attribute in %s\n", DEVICE_FILE);
        goto error;
    }
    device_info.partition_count = strtoul(count_str, NULL, 0);

    /*
     * get partition info
     */
    for (node = mxmlFindElement(node, tree, "item", NULL, NULL, MXML_DESCEND);
            node != NULL;
            node = mxmlFindElement(node, tree, "item", NULL, NULL,MXML_DESCEND)) {
        struct partition_info* partition = malloc(sizeof(struct partition_info));
        memset(partition, 0, sizeof(struct partition_info));

        /*
         * get name node
         */
        sub_node =  mxmlFindElement(node, node, "name", NULL, NULL,
                MXML_DESCEND);
        if (sub_node == NULL) {
            LOGE("Failed to find \"name\" element in %s\n", DEVICE_FILE);
            free(partition);
            break;
        }
        const char* name = mxmlGetOpaque(sub_node);
        memcpy(partition->name, name, strlen(name));

        /*
         * get offset node
         */
        sub_node =  mxmlFindElement(node, node, "offset", NULL, NULL,
                MXML_DESCEND);
        if (sub_node == NULL) {
            LOGE("Failed to find \"offset\" element in %s\n", DEVICE_FILE);
            free(partition);
            break;
        }
        const char* offset_str = mxmlGetText(sub_node, 0);
        partition->offset = strtoull(offset_str, NULL, 0);

        /*
         * get size node
         */
        sub_node =  mxmlFindElement(node, node, "size", NULL, NULL,
                MXML_DESCEND);
        if (sub_node == NULL) {
            LOGE("Failed to find \"size\" element in %s\n", DEVICE_FILE);
            free(partition);
            break;
        }
        const char* size_str = mxmlGetText(sub_node, 0);
        partition->size = strtoull(size_str, NULL, 0);

        /*
         * get fs type node
         */
        sub_node =  mxmlFindElement(node, node, "blockname", NULL, NULL,
                MXML_DESCEND);
        if (sub_node == NULL) {
            LOGE("Failed to find \"blockname\" element in %s\n", DEVICE_FILE);
            free(partition);
            break;
        }
        const char* block_name = mxmlGetOpaque(sub_node);
        memcpy(partition->block_name, block_name, strlen(block_name));

        list_add_tail(&partition->head, &device_info.partition_list);
    }

    LOGI("=========================\n");
    LOGI("Dump %s\n", DEVICE_FILE);
    LOGI("device type: %s\n", device_info.type);
    LOGI("device capacity: 0x%x\n", (uint32_t) device_info.capacity);

    struct partition_info* partition;
    list_for_each(pos, &device_info.partition_list) {
        partition = list_entry(pos, struct partition_info, head);
        LOGI("-------------------------\n");
        LOGI("part name: %s\n", partition->name);
        LOGI("part offset: 0x%x\n", (uint32_t) partition->offset);
        LOGI("part size: 0x%x\n", (uint32_t) partition->size);
        LOGI("part mtd name: %s\n", partition->block_name);
    }

    LOGI("=========================\n");

    mxmlDelete(tree);
    return 0;

error:
    if (tree)
        mxmlDelete(tree);

    return -1;
}
