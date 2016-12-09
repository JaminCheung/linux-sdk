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

#ifndef LOG_H_
#define LOG_H_

#include <errno.h>
#include <stdio.h>
#include <pthread.h>

#ifdef LOCAL_DEBUG
#define DEBUG_TRACE 1
#else
#define DEBUG_TRACE 0
#endif

__attribute__((unused)) static pthread_mutex_t log_init_lock = PTHREAD_MUTEX_INITIALIZER;

#define TAG_BASE "Recovery--->"

#define LOGV(...)                                                              \
    do {                                                                       \
      int save_errno = errno;                                                  \
      pthread_mutex_lock(&log_init_lock);                                      \
      fprintf(stderr, "V/%s%s %d: ", TAG_BASE, LOG_TAG, __LINE__);             \
      errno = save_errno;                                                      \
      fprintf(stderr, __VA_ARGS__);                                            \
      fflush(stderr);                                                          \
      pthread_mutex_unlock(&log_init_lock);                                    \
      errno = save_errno;                                                      \
    } while (0)

#define LOGD(...)                                                              \
    do {                                                                       \
      if (DEBUG_TRACE) {                                                       \
          int save_errno = errno;                                              \
          pthread_mutex_lock(&log_init_lock);                                  \
          fprintf(stderr, "D/%s%s %d: ", TAG_BASE, LOG_TAG, __LINE__);         \
          errno = save_errno;                                                  \
          fprintf(stderr, __VA_ARGS__);                                        \
          fflush(stderr);                                                      \
          pthread_mutex_unlock(&log_init_lock);                                \
          errno = save_errno;                                                  \
      }                                                                        \
    } while (0)

#define LOGI(...)                                                              \
    do {                                                                       \
      int save_errno = errno;                                                  \
      pthread_mutex_lock(&log_init_lock);                                      \
      fprintf(stderr, "I/%s%s %d: ", TAG_BASE, LOG_TAG, __LINE__);             \
      errno = save_errno;                                                      \
      fprintf(stderr, __VA_ARGS__);                                            \
      fflush(stderr);                                                          \
      pthread_mutex_unlock(&log_init_lock);                                    \
      errno = save_errno;                                                      \
    } while (0)

#define LOGW(...)                                                              \
    do {                                                                       \
      int save_errno = errno;                                                  \
      pthread_mutex_lock(&log_init_lock);                                      \
      fprintf(stderr, "W/%s%s %d: ", TAG_BASE, LOG_TAG, __LINE__);             \
      errno = save_errno;                                                      \
      fprintf(stderr, __VA_ARGS__);                                            \
      fflush(stderr);                                                          \
      pthread_mutex_unlock(&log_init_lock);                                    \
      errno = save_errno;                                                      \
    } while (0)

#define LOGE(...)                                                              \
    do {                                                                       \
      int save_errno = errno;                                                  \
      pthread_mutex_lock(&log_init_lock);                                      \
      fprintf(stderr, "E/%s%s %d: ", TAG_BASE, LOG_TAG, __LINE__);             \
      errno = save_errno;                                                      \
      fprintf(stderr, __VA_ARGS__);                                            \
      fflush(stderr);                                                          \
      pthread_mutex_unlock(&log_init_lock);                                    \
      errno = save_errno;                                                      \
    } while (0)
#endif /* LOG_H_ */
