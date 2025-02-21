/*!
 * \file msvc/creat.c
 *
 * \brief A wrapper function for MSVC _creat() that converts permission mode.
 *
 * (C) 2025 by the GRASS Development Team
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * \author Huidae Cho
 *
 * \date 2025
 */

#include <fcntl.h>
#include <sys/stat.h>

int __creat(const char *pathname, int mode)
{
    return _creat(pathname,
                  (mode & 0400 ? _S_IREAD : 0) | (mode & 0200 ? _S_IWRITE : 0));
}
