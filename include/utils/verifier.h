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

#ifndef VERIFIER_H
#define VERIFIER_H

#include "mincrypt/rsa.h"

/* Look in the file for a signature footer, and verify that it
 * matches one of the given keys.  Return one of the constants below.
 */
int verify_file(const char* path, const RSAPublicKey *pKeys, unsigned int numKeys);

RSAPublicKey* load_keys(const char* filename, int* numKeys);

#define VERIFY_SUCCESS        0
#define VERIFY_FAILURE        1

#endif  /* VERIFIER_H */
