/* LIBDGL -- a Directed Graph Library implementation
 * SPDX-FileCopyrightText: 2002 Roberto Micarelli
 * SPDX-FileCopyrightText: Other GRASS authors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

/*
 * best view tabstop=4
 */

#ifndef _DGL_TYPE_H_
#define _DGL_TYPE_H_ 1

/*
 * local endianness
 */
#ifdef WORDS_BIGENDIAN
#define G_XDR 1
#else
#define G_NDR 1
#endif

typedef unsigned char dglByte_t;
typedef long dglInt32_t;
typedef long long dglInt64_t;

#endif
