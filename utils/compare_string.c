/*
 *  Copyright (C) 2016, Zhang YanMing <jamincheung@126.com>
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

#include <string.h>

/*
 * Return NULL if string is not prefixed with key. Return pointer to the
 * first character in string after the prefix key. If key is an empty string,
 * return pointer to the beginning of string.
 */
char* is_prefixed_with(const char* string, const char* key) {
    while (*key != '\0') {
        if (*key != *string)
            return NULL;
        key++;
        string++;
    }
    return (char*) string;
}

/*
 * Return NULL if string is not suffixed with key. Return pointer to the
 * beginning of prefix key in string. If key is an empty string return pointer
 * to the end of string.
 */
char* is_suffixed_with(const char* string, const char* key) {
    int key_len = strlen(key);
    int len_diff = strlen(string) - key_len;

    if (len_diff >= 0) {
        string += len_diff;
        if (strcmp(string, key) == 0) {
            return (char*)string;
        }
    }

    return NULL;
}

/* returns the array index of the string */
/* (index of first match is returned, or -1) */
int index_in_str_array(const char *const string_array[], const char *key)
{
    int i;

    for (i = 0; string_array[i] != 0; i++) {
        if (strcmp(string_array[i], key) == 0) {
            return i;
        }
    }
    return -1;
}

int index_in_strings(const char *strings, const char *key)
{
    int idx = 0;

    while (*strings) {
        if (strcmp(strings, key) == 0) {
            return idx;
        }
        strings += strlen(strings) + 1; /* skip NUL */
        idx++;
    }
    return -1;
}

int index_in_substrings(const char *strings, const char *key)
{
    int matched_idx = -1;
    const int len = strlen(key);

    if (len) {
        int idx = 0;
        while (*strings) {
            if (strncmp(strings, key, len) == 0) {
                if (strings[len] == '\0')
                    return idx; /* exact match */
                if (matched_idx >= 0)
                    return -1; /* ambiguous match */
                matched_idx = idx;
            }
            strings += strlen(strings) + 1; /* skip NUL */
            idx++;
        }
    }
    return matched_idx;
}

const char* nth_string(const char *strings, int n)
{
    while (n) {
        n--;
        strings += strlen(strings) + 1;
    }
    return strings;
}
