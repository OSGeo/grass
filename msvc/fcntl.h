/*!
 * \file msvc/fcntl.h
 *
 * \brief Header file for msvc/open.c and msvc/creat.c
 *
 * (C) 2025 by the GRASS Development Team
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * \author Huidae Cho
 *
 * \date 2025
 */

#ifndef GRASS_MSVC_FCNTL_H
#define GRASS_MSVC_FCNTL_H

#include <../ucrt/fcntl.h>

#ifdef __cplusplus
extern "C" {
#endif

/* these wrapper functions convert UN*X permission mode for MSVC */
int __open(const char *, int, ...);
int __creat(const char *, int);

#ifdef __cplusplus
}
#endif

#include <io.h> // Also for _S_IREAD and _S_IWRITE
#define open      __open
#define creat     __creat

#define O_TMPFILE O_TEMPORARY
#define O_ACCMODE (_O_RDONLY | _O_WRONLY | _O_RDWR)

#if defined(_WIN32)
#ifndef S_IRUSR
#define S_IRUSR _S_IREAD
#endif // S_IRUSR
#ifndef S_IWUSR
#define S_IWUSR _S_IWRITE
#endif // S_IWUSR
#endif

#endif
