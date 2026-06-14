/*!
   \file rowio/fileno.c

   \brief RowIO library - file descriptor

   (C) 2001-2009 by the GRASS Development Team

    SPDX-License-Identifier: GPL-2.0-or-later
\author Original author CERL
 */

#include <grass/rowio.h>

/*!
 * \brief Get file descriptor
 *
 * Returns the file descriptor associated with the ROWIO structure.
 *
 * \param R pointer to ROWIO structure
 *
 * \return file descriptor
 */
int Rowio_fileno(const ROWIO *R)
{
    return R->fd;
}
