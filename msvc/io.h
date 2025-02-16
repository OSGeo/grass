/*!
 * \file msvc/io.h
 *
 * \brief Header file for msvc/open.c and msvc/creat.c
 *
 * (C) 2025 by the GRASS Development Team
 *
 * This program is free software under the GNU General Public License (>=v2).
 * Read the file COPYING that comes with GRASS for details.
 *
 * \author Huidae Cho
 *
 * \date 2025
 */

#ifndef GRASS_MSVC_IO_H
#define GRASS_MSVC_IO_H

#include <../ucrt/io.h>

#ifdef __cplusplus
extern "C" {
#endif

/* these wrapper functions convert UN*X permission mode for MSVC */
int __open(const char *, int, ...);
int __creat(const char *, int);

#ifdef __cplusplus
}
#endif

#define open      __open
#define creat     __creat

#define O_TMPFILE O_TEMPORARY

#endif
