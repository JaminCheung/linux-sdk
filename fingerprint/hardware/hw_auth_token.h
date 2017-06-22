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

#ifndef HARDWARE_HW_AUTH_TOKEN_H
#define HARDWARE_HW_AUTH_TOKEN_H

#include <types.h>
#include <limits.h>

const uint8_t HW_AUTH_TOKEN_VERSION = 0;

typedef enum {
    HW_AUTH_NONE = 0,
    HW_AUTH_PASSWORD = 1 << 0,
    HW_AUTH_FINGERPRINT = 1 << 1,
    // Additional entries should be powers of 2.
    HW_AUTH_ANY = INT_MAX,
} hw_authenticator_type_t;

/**
 * Data format for an authentication record used to prove successful authentication.
 */
typedef struct __attribute__((__packed__)) {
    uint8_t version;  // Current version is 0
    uint64_t challenge;
    uint64_t user_id;             // secure user ID, not Android user ID
    uint64_t authenticator_id;    // secure authenticator ID
    uint32_t authenticator_type;  // hw_authenticator_type_t, in network order
    uint64_t timestamp;           // in network order
    uint8_t hmac[32];
} hw_auth_token_t;

#endif /* HARDWARE_HW_AUTH_TOKEN_H */
