/*!
 * \file lib/gis/zero.c
 *
 * \brief GIS Library - Zeroing functions.
 *
 * SPDX-FileCopyrightText: 2001-2009 Other GRASS authors
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * \author Original author CERL
 */

#include <string.h>
#include <grass/gis.h>

/*!
 * \brief Zero out a buffer, <b>buf</b>, of length <b>i</b>.
 *
 * \param[out] buf
 * \param i number of bytes to be zeroed
 */
void G_zero(void *buf, int i)
{
    memset(buf, 0, i);
}
