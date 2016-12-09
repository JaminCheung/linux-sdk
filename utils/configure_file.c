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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <limits.h>
#include <errno.h>
#include <sys/stat.h>
#include <dirent.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

#include <autoconf.h>
#include <utils/log.h>
#include <utils/assert.h>
#include <utils/configure_file.h>

#define LOG_TAG "configure_file"

#if (defined CONFIG_BASE64_ENCODE_CONFIG_FILE) && (CONFIG_BASE64_ENCODE_CONFIG_FILE == true)
static const char encode_table[64] = { 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
        'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V',
        'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j',
        'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x',
        'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/' };
static unsigned int mod_table[] = { 0, 2, 1 };
static char *decode_table;

static void build_decoding_table() {
    unsigned int i;

    decode_table = (char *) malloc(256);
    memset(decode_table, 0, 256);

    for (i = 0; i < 64; i++)
        decode_table[(unsigned char) encode_table[i]] = i;
}

static void base64_cleanup() {
    if (decode_table) {
        free(decode_table);
        decode_table = NULL;
    }
}

__attribute__((unused)) static char *base64_encode(const char *data,
        unsigned int input_length, unsigned int *output_length) {

    unsigned int i = 0;
    unsigned int j = 0;
    unsigned int octet_a = 0;
    unsigned int octet_b = 0;
    unsigned int octet_c = 0;
    unsigned int triple = 0;
    char *encoded_data = NULL;

    *output_length = 4 * ((input_length + 2) / 3);

    encoded_data = (char *) malloc(*output_length);
    if (encoded_data == NULL)
        return NULL;
    memset(encoded_data, 0, *output_length);

    for (i = 0, j = 0; i < input_length;) {
        octet_a = i < input_length ? data[i++] : 0;
        octet_b = i < input_length ? data[i++] : 0;
        octet_c = i < input_length ? data[i++] : 0;

        triple = (octet_a << 0x10) + (octet_b << 0x08) + octet_c;

        encoded_data[j++] = encode_table[(triple >> 3 * 6) & 0x3F];
        encoded_data[j++] = encode_table[(triple >> 2 * 6) & 0x3F];
        encoded_data[j++] = encode_table[(triple >> 1 * 6) & 0x3F];
        encoded_data[j++] = encode_table[(triple >> 0 * 6) & 0x3F];
    }

    for (i = 0; i < mod_table[input_length % 3]; i++)
        encoded_data[*output_length - 1 - i] = '=';

    return encoded_data;
}

static char *base64_decode(const unsigned char *data, unsigned int input_length,
        unsigned int *output_length) {

    unsigned int i = 0;
    unsigned int j = 0;
    unsigned int sextet_a = 0;
    unsigned int sextet_b = 0;
    unsigned int sextet_c = 0;
    unsigned int sextet_d = 0;
    unsigned int triple = 0;

    char *decoded_data = NULL;

    if (decode_table == NULL)
        build_decoding_table();

    if (input_length % 4 != 0)
        return NULL;

    *output_length = input_length / 4 * 3;

    if (data[input_length - 1] == '=')
        (*output_length)--;
    if (data[input_length - 2] == '=')
        (*output_length)--;

    decoded_data = (char*) malloc(*output_length);
    if (decoded_data == NULL)
        return NULL;
    memset(decoded_data, 0, *output_length);

    for (i = 0, j = 0; i < input_length;) {
        sextet_a = data[i] == '=' ? 0 & i++ : decode_table[data[i++]];
        sextet_b = data[i] == '=' ? 0 & i++ : decode_table[data[i++]];
        sextet_c = data[i] == '=' ? 0 & i++ : decode_table[data[i++]];
        sextet_d = data[i] == '=' ? 0 & i++ : decode_table[data[i++]];

        triple = (sextet_a << 3 * 6) + (sextet_b << 2 * 6) + (sextet_c << 1 * 6)
                + (sextet_d << 0 * 6);

        if (j < *output_length)
            decoded_data[j++] = (triple >> 2 * 8) & 0xFF;
        if (j < *output_length)
            decoded_data[j++] = (triple >> 1 * 8) & 0xFF;
        if (j < *output_length)
            decoded_data[j++] = (triple >> 0 * 8) & 0xFF;
    }

    return decoded_data;
}
#endif

static FILE* open_file(const char* file_name) {
    if (!file_name)
        return NULL;

    return fopen(file_name, "r");
}

static char* get_line(char *s, int size, FILE* stream, int* line, char** _pos) {
    char *pos = NULL;
    char *end = NULL;
    char *sstart = NULL;

    memset(s, 0, size);

#if (defined CONFIG_BASE64_ENCODE_CONFIG_FILE) && (CONFIG_BASE64_ENCODE_CONFIG_FILE == true)
    unsigned int base64_out_size = 0;
    char* buf = (char*) malloc(size);
    memset(buf, 0, size);

    char* base64_buf = NULL;

    while (fgets(buf, size, stream)) {
        base64_buf = base64_decode((unsigned char*) buf, size,
                &base64_out_size);
        memset(buf, 0, size);
        assert_die_if(base64_buf == NULL, "Base64 decode error");
        memcpy(s, base64_buf, base64_out_size);
        free(base64_buf);
#else
    while (fgets(s, size, stream)) {
#endif
        (*line)++;
        pos = s;

        /* Skip white space from the beginning of line. */
        while (*pos == ' ' || *pos == '\t' || *pos == '\r')
            pos++;

        /* Skip comment lines and empty lines */
        if (*pos == '#' || *pos == '\n' || *pos == '\0')
            continue;

        /*
         * Remove # comments unless they are within a double quoted
         * string.
         */
        sstart = strchr(pos, '"');
        if (sstart)
            sstart = strrchr(sstart + 1, '"');
        if (!sstart)
            sstart = pos;
        end = strchr(sstart, '#');
        if (end)
            *end-- = '\0';
        else
            end = pos + strlen(pos) - 1;

        /* Remove trailing white space. */
        while (end > pos
                && (*end == '\n' || *end == ' ' || *end == '\t' || *end == '\r'))
            *end-- = '\0';

        if (*pos == '\0')
            continue;

        if (_pos)
            *_pos = pos;

//        LOGD("Line %d: %s", *line, *_pos);

#if (defined CONFIG_BASE64_ENCODE_CONFIG_FILE) && (CONFIG_BASE64_ENCODE_CONFIG_FILE == true)
        if (buf)
            free(buf);
        base64_cleanup();
#endif
        return pos;
    }

    if (_pos)
        *_pos = NULL;

#if (defined CONFIG_BASE64_ENCODE_CONFIG_FILE) && (CONFIG_BASE64_ENCODE_CONFIG_FILE == true)
    if (buf)
        free(buf);
    base64_cleanup();
#endif

    return NULL;
}

const char* get_value(const char* line) {
    char* sstart = strchr(line, '=');
    if (sstart)
        return sstart[1] == '\0' ? NULL : sstart + 1;
    else
        return NULL;
}

static struct bootloader_config* read_bootloader_config(FILE* stream, int* line) {
    struct bootloader_config* config;
    char* pos;
    char buf[512] = { 0 };
    int end = 0;
    int errors = 0;

    char* name = NULL;
    char* file_path = NULL;
    char* md5 = NULL;

    while (get_line(buf, sizeof(buf), stream, line, &pos)) {
        if (!strcmp(pos, "}")) {
            end = 1;
            break;
        } else if (!strncmp(pos, "name=", 5)) {
            name = strdup(get_value(pos));
            if (!name) {
                LOGE("Failed to parase line: %d: name=?", *line);
                errors++;
                break;
            }
        } else if (!strncmp(pos, "file_path=", 10)) {
            file_path = strdup(get_value(pos));
            if (!file_path) {
                LOGE("Failed to parase line: %d: file_path=?", *line);
                errors++;
                break;
            }
        } else if (!strncmp(pos, "md5=", 4)) {
            md5 = strdup(get_value(pos));
            if (!file_path) {
                LOGE("Failed to parase line: %d: md5=?", *line);
                errors++;
                break;
            }
        } else {
            LOGE("Failed to parse Line: %d: Unrecognized line: %s.", *line,
                    pos);
            errors++;
            break;
        }
    }

    if (!end) {
        LOGE("Failed to parse Line: %d: lost \"}\".", *line);
        goto error;
    }

    if (errors || !name || !file_path || !md5)
        goto error;

    config = (struct bootloader_config*) malloc(
            sizeof(struct bootloader_config));
    config->name.key = strdup("BOOTLOADER PART");
    config->name.value = name;
    config->file_path.key = strdup("BOOTLOADER IMAGE");
    config->file_path.value = file_path;
    config->md5.key = strdup("BOOTLOADER MD5");
    config->md5.value = md5;
    return config;

error:
    if (name)
        free(name);
    if (file_path)
        free(file_path);
    if (md5)
        free(md5);

    return NULL;
}

static struct kernel_config* read_kernel_config(FILE* stream, int* line) {
    struct kernel_config* config;
    char* pos;
    char buf[512] = { 0 };
    int end = 0;
    int errors = 0;

    char* name = NULL;
    char* file_path = NULL;
    char* md5 = NULL;

    while (get_line(buf, sizeof(buf), stream, line, &pos)) {
        if (!strcmp(pos, "}")) {
            end = 1;
            break;
        } else if (!strncmp(pos, "name=", 5)) {
            name = strdup(get_value(pos));
            if (!name) {
                LOGE("Failed to parase line: %d: name=?", *line);
                errors++;
                break;
            }
        } else if (!strncmp(pos, "file_path=", 10)) {
            file_path = strdup(get_value(pos));
            if (!file_path) {
                LOGE("Failed to parase line: %d: file_path=?", *line);
                errors++;
                break;
            }
        } else if (!strncmp(pos, "md5=", 4)) {
            md5 = strdup(get_value(pos));
            if (!file_path) {
                LOGE("Failed to parase line: %d: md5=?", *line);
                errors++;
                break;
            }
        } else {
            LOGE("Failed to parse Line: %d: Unrecognized line: %s.", *line,
                    pos);
            errors++;
            break;
        }
    }

    if (!end) {
        LOGE("Failed to parse Line: %d: lost \"}\".", *line);
        goto error;
    }

    if (errors || !name || !file_path || !md5)
        goto error;

    config = (struct kernel_config*) malloc(sizeof(struct kernel_config));
    config->name.key = strdup("KERNEL PART");
    config->name.value = name;
    config->file_path.key = strdup("KERNEL IMAGE");
    config->file_path.value = file_path;
    config->md5.key = strdup("KERNEL MD5");
    config->md5.value = md5;

    return config;

error:
    if (name)
        free(name);
    if (file_path)
        free(file_path);
    if (md5)
        free(md5);

    return NULL;
}

static struct splash_config* read_splash_config(FILE* stream, int* line) {
    struct splash_config* config;
    char* pos;
    char buf[512] = { 0 };
    int end = 0;
    int errors = 0;

    char* name = NULL;
    char* file_path = NULL;
    char* md5 = NULL;

    while (get_line(buf, sizeof(buf), stream, line, &pos)) {
        if (!strcmp(pos, "}")) {
            end = 1;
            break;
        } else if (!strncmp(pos, "name=", 5)) {
            name = strdup(get_value(pos));
            if (!name) {
                LOGE("Failed to parase line: %d: name=?", *line);
                errors++;
                break;
            }
        } else if (!strncmp(pos, "file_path=", 10)) {
            file_path = strdup(get_value(pos));
            if (!file_path) {
                LOGE("Failed to parase line: %d: file_path=?", *line);
                errors++;
                break;
            }
        } else if (!strncmp(pos, "md5=", 4)) {
            md5 = strdup(get_value(pos));
            if (!file_path) {
                LOGE("Failed to parase line: %d: md5=?", *line);
                errors++;
                break;
            }
        } else {
            LOGE("Failed to parse Line: %d: Unrecognized line: %s.", *line,
                    pos);
            errors++;
            break;
        }
    }

    if (!end) {
        LOGE("Failed to parse Line: %d: lost \"}\".", *line);
        goto error;
    }

    if (errors || !name || !file_path || !md5)
        goto error;

    config = (struct splash_config*) malloc(sizeof(struct splash_config));
    config->name.key = strdup("SPLASH PART");
    config->name.value = name;
    config->file_path.key = strdup("SPLASH IMAGE");
    config->file_path.value = file_path;
    config->md5.key = strdup("SPLASH MD5");
    config->md5.value = md5;

    return config;

error:
    if (name)
        free(name);
    if (file_path)
        free(file_path);
    if (md5)
        free(md5);

    return NULL;
}

static struct rootfs_config* read_rootfs_config(FILE* stream, int* line) {
    struct rootfs_config* config;
    char* pos;
    char buf[512] = { 0 };
    int end = 0;
    int errors = 0;

    char* name = NULL;
    char* full_upgrade = NULL;
    char* file_path = NULL;
    char* md5 = NULL;

    while (get_line(buf, sizeof(buf), stream, line, &pos)) {
        if (!strcmp(pos, "}")) {
            end = 1;
            break;
        } else if (!strncmp(pos, "name=", 5)) {
            name = strdup(get_value(pos));
            if (!name) {
                LOGE("Failed to parase line: %d: name=?", *line);
                errors++;
                break;
            }
        } else if (!strncmp(pos, "full_upgrade=", 13)) {
            full_upgrade = strdup(get_value(pos));
            if (!full_upgrade) {
                LOGE("Failed to parase line: %d: full_upgrade=?", *line);
                errors++;
                break;
            }
        } else if (!strncmp(pos, "file_path=", 10)) {
            file_path = strdup(get_value(pos));
            if (!file_path) {
                LOGE("Failed to parase line: %d: file_path=?", *line);
                errors++;
                break;
            }
        } else if (!strncmp(pos, "md5=", 4)) {
            md5 = strdup(get_value(pos));
            if (!file_path) {
                LOGE("Failed to parase line: %d: md5=?", *line);
                errors++;
                break;
            }
        } else {
            LOGE("Failed to parse Line: %d: Unrecognized line: %s.", *line,
                    pos);
            errors++;
            break;
        }
    }

    if (!end) {
        LOGE("Failed to parse Line: %d: lost \"}\".", *line);
        goto error;
    }

    if (errors || !name || !full_upgrade || !file_path || !md5)
        goto error;

    if ((strcmp(full_upgrade, "yes") && strcmp(full_upgrade, "no"))) {
        LOGE("Failed to read full_upgrade value: %s", full_upgrade);
        goto error;
    }

    config = (struct rootfs_config*) malloc(sizeof(struct rootfs_config));
    config->name.key = strdup("ROOTFS PART");
    config->name.value = name;
    config->full_upgrade.key = strdup("ROOTFS FULL UPFRADE");
    config->full_upgrade.value = full_upgrade;
    config->file_path.key = strdup("ROOTFS IMAGE");
    config->file_path.value = file_path;
    config->md5.key = strdup("ROOTFS MD5");
    config->md5.value = md5;

    return config;

error:
    if (name)
        free(name);
    if (full_upgrade)
        free(full_upgrade);
    if (file_path)
        free(file_path);
    if (md5)
        free(md5);

    return NULL;
}

static struct userfs_config* read_userfs_config(FILE* stream, int* line) {
    struct userfs_config* config;
    char* pos;
    char buf[512] = { 0 };
    int end = 0;
    int errors = 0;

    char* name = NULL;
    char* full_upgrade = NULL;
    char* file_path = NULL;
    char* md5 = NULL;

    while (get_line(buf, sizeof(buf), stream, line, &pos)) {
        if (!strcmp(pos, "}")) {
            end = 1;
            break;
        } else if (!strncmp(pos, "name=", 5)) {
            name = strdup(get_value(pos));
            if (!name) {
                LOGE("Failed to parase line: %d: name=?", *line);
                errors++;
                break;
            }
        } else if (!strncmp(pos, "full_upgrade=", 13)) {
            full_upgrade = strdup(get_value(pos));
            if (!full_upgrade) {
                LOGE("Failed to parase line: %d: full_upgrade=?", *line);
                errors++;
                break;
            }
        } else if (!strncmp(pos, "file_path=", 10)) {
            file_path = strdup(get_value(pos));
            if (!file_path) {
                LOGE("Failed to parase line: %d: file_path=?", *line);
                errors++;
                break;
            }
        } else if (!strncmp(pos, "md5=", 4)) {
            md5 = strdup(get_value(pos));
            if (!file_path) {
                LOGE("Failed to parase line: %d: md5=?", *line);
                errors++;
                break;
            }
        } else {
            LOGE("Failed to parse Line: %d: Unrecognized line: %s.", *line,
                    pos);
            errors++;
            break;
        }
    }

    if (!end) {
        LOGE("Failed to parse Line: %d: lost \"}\".", *line);
        goto error;
    }

    if (errors || !name || !full_upgrade || !file_path || !md5)
        goto error;

    if ((strcmp(full_upgrade, "yes") && strcmp(full_upgrade, "no"))) {
        LOGE("Failed to read full_upgrade value: %s", full_upgrade);
        goto error;
    }

    config = (struct userfs_config*) malloc(sizeof(struct userfs_config));
    config->name.key = strdup("USERFS PART");
    config->name.value = name;
    config->full_upgrade.key = strdup("USERFS FULL UPGRADE");
    config->full_upgrade.value = full_upgrade;
    config->file_path.key = strdup("USERFS IMAGE");
    config->file_path.value = file_path;
    config->md5.key = strdup("USERFS MD5");
    config->md5.value = md5;

    return config;

error:
    if (name)
        free(name);
    if (full_upgrade)
        free(full_upgrade);
    if (file_path)
        free(file_path);
    if (md5)
        free(md5);

    return NULL;
}

static int parse_configure(FILE* stream, struct config* config) {
    char buf[512] = { 0 };
    char *pos = NULL;
    int line = 0;
    int errors = 0;

    while (get_line(buf, sizeof(buf), stream, &line, &pos)) {
        if (!strcmp(pos, "bootloader={")) {
            config->bootloader = read_bootloader_config(stream, &line);
            if (!config->bootloader) {
                LOGE("Failed to read bootloader configure.");
                errors++;
                break;
            }
        } else if (!strcmp(pos, "kernel={")) {
            config->kernel = read_kernel_config(stream, &line);
            if (!config->kernel) {
                LOGE("Failed to read kernel configure.");
                errors++;
                break;
            }
        } else if (!strcmp(pos, "splash={")) {
            config->splash = read_splash_config(stream, &line);
            if (!config->kernel) {
                LOGE("Failed to read splash configure.");
                errors++;
                break;
            }
        } else if (!strcmp(pos, "rootfs={")) {
            config->rootfs = read_rootfs_config(stream, &line);
            if (!config->rootfs) {
                LOGE("Failed to read rootfs configure.");
                errors++;
                break;
            }
        } else if (!strcmp(pos, "userfs={")) {
            config->userfs = read_userfs_config(stream, &line);
            if (!config->userfs) {
                LOGE("Failed to read userfs configure.");
                errors++;
                break;
            }
        } else {
            LOGE("Failed to parse Line: %d: Unrecognized line: %s.", line, pos);
            errors++;
            break;
        }
    }

    if (errors)
        return -1;

    return 0;
}

static void install_config(struct config* config,
        struct configure_data* configure_data) {

    memset(configure_data, 0, sizeof(struct configure_data));

    if (config->bootloader) {
        configure_data->bootloader_upgrade = true;
        strcpy(configure_data->bootloader_name, config->bootloader->name.value);
        strcpy(configure_data->bootloader_path, config->bootloader->file_path.value);
        strcpy(configure_data->bootloader_md5, config->bootloader->md5.value);
    }

    if (config->kernel) {
        configure_data->kernel_upgrade = true;
        strcpy(configure_data->kernel_name, config->kernel->name.value);
        strcpy(configure_data->kernel_path, config->kernel->file_path.value);
        strcpy(configure_data->kernel_md5, config->kernel->md5.value);
    }

    if (config->splash) {
        configure_data->splash_upgrade = true;
        strcpy(configure_data->splash_name, config->splash->name.value);
        strcpy(configure_data->splash_path, config->splash->file_path.value);
        strcpy(configure_data->splash_md5, config->splash->md5.value);
    }

    if (config->rootfs) {
        configure_data->rootfs_upgrade = true;
        strcpy(configure_data->rootfs_name, config->rootfs->name.value);
        if (!strcmp(config->rootfs->full_upgrade.value, "yes"))
            configure_data->rootfs_full_upgrade = true;
        else
            configure_data->rootfs_full_upgrade = false;
        strcpy(configure_data->rootfs_path, config->rootfs->file_path.value);
        strcpy(configure_data->rootfs_md5, config->rootfs->md5.value);
    }

    if (config->userfs) {
        configure_data->userfs_upgrade = true;
        strcpy(configure_data->userfs_name, config->userfs->name.value);
        if (!strcmp(config->userfs->full_upgrade.value, "yes"))
            configure_data->userfs_full_upgrade = true;
        else
            configure_data->userfs_full_upgrade = false;
        strcpy(configure_data->userfs_path, config->userfs->file_path.value);
        strcpy(configure_data->userfs_md5, config->userfs->md5.value);
    }
}

static void clean_config(struct config* config) {
    if (config == NULL)
        return;

    if (config->bootloader) {
        if (config->bootloader->name.key) {
            free(config->bootloader->name.key);
            config->bootloader->name.key = NULL;
        }

        if (config->bootloader->name.value) {
            free(config->bootloader->name.value);
            config->bootloader->name.value = NULL;
        }

        if (config->bootloader->file_path.key) {
            free(config->bootloader->file_path.key);
            config->bootloader->file_path.key = NULL;
        }

        if (config->bootloader->file_path.value) {
            free(config->bootloader->file_path.value);
            config->bootloader->file_path.value = NULL;
        }

        if (config->bootloader->md5.key) {
            free(config->bootloader->md5.key);
            config->bootloader->md5.key = NULL;
        }

        if (config->bootloader->md5.value) {
            free(config->bootloader->md5.value);
            config->bootloader->md5.value = NULL;
        }
    }

    if (config->kernel) {
        if (config->kernel->name.key) {
            free(config->kernel->name.key);
            config->kernel->name.key = NULL;
        }

        if (config->kernel->name.value) {
            free(config->kernel->name.value);
            config->kernel->name.value = NULL;
        }

        if (config->kernel->file_path.key) {
            free(config->kernel->file_path.key);
            config->kernel->file_path.key = NULL;
        }

        if (config->kernel->file_path.value) {
            free(config->kernel->file_path.value);
            config->kernel->file_path.value = NULL;
        }

        if (config->kernel->md5.key) {
            free(config->kernel->md5.key);
            config->kernel->md5.key = NULL;
        }
    }

    if (config->splash) {
        if (config->splash->name.key) {
            free(config->splash->name.key);
            config->splash->name.key = NULL;
        }

        if (config->splash->name.value) {
            free(config->splash->name.value);
            config->splash->name.value = NULL;
        }

        if (config->splash->file_path.key) {
            free(config->splash->file_path.key);
            config->splash->file_path.key = NULL;
        }

        if (config->splash->file_path.value) {
            free(config->splash->file_path.value);
            config->splash->file_path.value = NULL;
        }

        if (config->splash->md5.key) {
            free(config->splash->md5.key);
            config->splash->md5.key = NULL;
        }

        if (config->splash->md5.value) {
            free(config->splash->md5.value);
            config->splash->md5.value = NULL;
        }
    }

    if (config->rootfs) {
        if (config->rootfs->name.key) {
            free(config->rootfs->name.key);
            config->rootfs->name.key = NULL;
        }

        if (config->rootfs->name.value) {
            free(config->rootfs->name.value);
            config->rootfs->name.value = NULL;
        }

        if (config->rootfs->full_upgrade.key) {
            free(config->rootfs->full_upgrade.key);
            config->rootfs->full_upgrade.key =  NULL;
        }

        if (config->rootfs->full_upgrade.value) {
            free(config->rootfs->full_upgrade.value);
            config->rootfs->full_upgrade.value = NULL;
        }

        if (config->rootfs->file_path.key) {
            free(config->rootfs->file_path.key);
            config->rootfs->file_path.key = NULL;
        }

        if (config->rootfs->file_path.value) {
            free(config->rootfs->file_path.value);
            config->rootfs->file_path.value = NULL;
        }

        if (config->rootfs->md5.key) {
            free(config->rootfs->md5.key);
            config->rootfs->md5.key = NULL;
        }

        if (config->rootfs->md5.value) {
            free(config->rootfs->md5.value);
            config->rootfs->md5.value = NULL;
        }
    }

    if (config->userfs) {
        if (config->userfs->name.key) {
            free(config->userfs->name.key);
            config->userfs->name.key = NULL;
        }

        if (config->userfs->name.value) {
            free(config->userfs->name.value);
            config->userfs->name.value = NULL;
        }

        if (config->userfs->full_upgrade.key) {
            free(config->userfs->full_upgrade.key);
            config->userfs->full_upgrade.key = NULL;
        }

        if (config->userfs->full_upgrade.value) {
            free(config->userfs->full_upgrade.value);
            config->userfs->full_upgrade.value = NULL;
        }

        if (config->userfs->file_path.key) {
            free(config->userfs->file_path.key);
            config->userfs->file_path.key = NULL;
        }

        if (config->userfs->file_path.value) {
            free(config->userfs->file_path.value);
            config->userfs->file_path.value = NULL;
        }

        if (config->userfs->md5.key) {
            free(config->userfs->md5.key);
            config->userfs->md5.key = NULL;
        }

        if (config->userfs->md5.value) {
            free(config->userfs->md5.value);
            config->userfs->md5.value = NULL;
        }
    }

    if (config) {
        free(config);
        config = NULL;
    }
}

static void dump_config(struct config* config) {
    LOGD("===================================");
    LOGD("Dump configure file.");

    if (config->bootloader) {
        LOGD("%s: %s", config->bootloader->name.key,
                config->bootloader->name.value);
        LOGD("%s: %s", config->bootloader->file_path.key,
                config->bootloader->file_path.value);
        LOGD("%s: %s", config->bootloader->md5.key, config->bootloader->md5.value);
    }

    if (config->kernel) {
        LOGD("%s: %s", config->kernel->name.key, config->kernel->name.value);
        LOGD("%s: %s", config->kernel->file_path.key,
                config->kernel->file_path.value);
        LOGD("%s: %s", config->kernel->md5.key, config->kernel->md5.value);
    }

    if (config->splash) {
        LOGD("%s: %s", config->splash->name.key, config->splash->name.value);
        LOGD("%s: %s", config->splash->file_path.key,
                config->splash->file_path.value);
        LOGD("%s: %s", config->splash->md5.key, config->splash->md5.value);
    }

    if (config->rootfs) {
        LOGD("%s: %s", config->rootfs->name.key, config->rootfs->name.value);
        LOGD("%s: %s", config->rootfs->full_upgrade.key,
                config->rootfs->full_upgrade.value);
        LOGD("%s: %s", config->rootfs->file_path.key,
                config->rootfs->file_path.value);
        LOGD("%s: %s", config->rootfs->md5.key, config->rootfs->md5.value);
    }

    if (config->userfs) {
        LOGD("%s: %s", config->userfs->name.key, config->userfs->name.value);
        LOGD("%s: %s", config->userfs->full_upgrade.key,
                config->userfs->full_upgrade.value);
        LOGD("%s: %s", config->userfs->file_path.key,
                config->userfs->file_path.value);
        LOGD("%s: %s", config->userfs->md5.key, config->userfs->md5.value);
    }
    LOGD("===================================");
}

static struct configure_data* load_configure_file(struct configure_file* this,
        const char* file_name) {
    FILE* stream = NULL;
    int retval = 0;
    struct config *config = NULL;

    config = (struct config *) malloc(sizeof(struct config));
    memset(config, 0, sizeof(struct config));

    if (!(stream = open_file(file_name))) {
        LOGE("Failed to open file: %s :%s", file_name, strerror(errno));
        goto error;
    }

    retval = parse_configure(stream, config);
    if (retval < 0) {
        LOGE("Failed to parse configure file: %s", file_name);
        goto error;
    }

    dump_config(config);

    install_config(config, this->data);

    clean_config(config);

    if (stream)
        fclose(stream);

    return this->data;

error:
    clean_config(config);

    if (stream)
        fclose(stream);

    return NULL;
}

static void dump(struct configure_file* this) {
    LOGI("===================================");
    LOGI("Dump configure data.");

    if (this->data->bootloader_name[0] != '\0') {
        LOGI("BOOTLOADER PART: %s", this->data->bootloader_name);
        LOGI("BOOTLOADER IMAGE: %s", this->data->bootloader_path);
        LOGI("BOOTLOADER MD5: %s", this->data->bootloader_md5);
    }

    if (this->data->kernel_name[0] != '\0') {
        LOGI("KERNEL PART: %s", this->data->kernel_name);
        LOGI("KERNEL IMAGE: %s", this->data->kernel_path);
        LOGI("KERNEL MD5: %s", this->data->kernel_md5);
    }

    if (this->data->splash_name[0] != '\0') {
        LOGI("SPLASH PART: %s", this->data->splash_name);
        LOGI("SPLASH IMAGE: %s", this->data->splash_path);
        LOGI("SPLASH MD5: %s", this->data->splash_md5);
    }

    if (this->data->rootfs_name[0] != '\0') {
        LOGI("ROOTFS PART: %s", this->data->rootfs_name);
        LOGI("ROOTFS FULL UPFRADE: %s",
                this->data->rootfs_full_upgrade ? "true" : "false");
        LOGI("ROOTFS IMAGE: %s", this->data->rootfs_path);
        LOGI("ROOTFS MD5: %s", this->data->rootfs_md5);
    }

    if (this->data->userfs_name[0] != '\0') {
        LOGI("USERFS PART: %s", this->data->userfs_name);
        LOGI("userfs full upgrade: %s",
                this->data->userfs_full_upgrade ? "true" : "false");
        LOGI("USERFS IMAGE: %s", this->data->userfs_path);
        LOGI("USERFS MD5: %s", this->data->userfs_md5);
    }
    LOGI("===================================");
}

void construct_configure_file(struct configure_file* this) {
    this->load_configure_file = load_configure_file;
    this->dump = dump;
    this->data = (struct configure_data*) malloc(sizeof(struct configure_data));
    memset(this->data, 0, sizeof(struct configure_data));
}

void destruct_configure_file(struct configure_file* this) {
    if (this->data) {
        free(this->data);
        this->data = NULL;
    }
}

