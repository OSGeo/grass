/*!
 * \file msvc/creat.c
 *
 * \brief A wrapper function for MSVC _creat() that converts permission mode.
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

#include <fcntl.h>
#include <sys/stat.h>

int __creat(const char *pathname, int mode)
{
    return _creat(pathname,
                  (mode & 0400 ? _S_IREAD : 0) | (mode & 0200 ? _S_IWRITE : 0));
}
