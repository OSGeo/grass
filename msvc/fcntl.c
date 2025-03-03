/*!
 * \file msvc/fcntl.c
 *
 * \brief Wrapper functions for MSVC _open() and _creat() that convert
 * permission mode.
 *
 * (C) 2025 by the GRASS Development Team
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * \author Huidae Cho
 *
 * \date 2025
 */

#include <stdarg.h>
#include <fcntl.h>
#include <sys/stat.h>

#define O_TMPFILE O_TEMPORARY

int __open(const char *pathname, int flags, ...)
{
    if (flags & (O_CREAT | O_TMPFILE)) {
        va_list ap;
        int mode;

        va_start(ap, flags);
        mode = va_arg(ap, int);
        va_end(ap);

        return _open(pathname, flags,
                     (mode & 0400 ? _S_IREAD : 0) |
                         (mode & 0200 ? _S_IWRITE : 0));
    }

    return _open(pathname, flags);
}

int __creat(const char *pathname, int mode)
{
    return _creat(pathname,
                  (mode & 0400 ? _S_IREAD : 0) | (mode & 0200 ? _S_IWRITE : 0));
}
