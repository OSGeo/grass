/*
 * SPDX-FileCopyrightText: 1995 Bill Brown <brown@gis.uiuc.edu>
 * SPDX-FileCopyrightText: 1995 Michael Shapiro
 * SPDX-FileCopyrightText: Other GRASS authors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include <string.h>
#include <grass/datetime.h>

/*!
 * \brief
 *
 * Returns:
 * 1 if 'src' is exactly the same as 'dst'
 * 0 if they differ
 *
 *  \param src
 *  \param dst
 *  \return int
 */
int datetime_is_same(const DateTime *src, const DateTime *dst)
{
    /* WARNING: doesn't allow for padding */
    return memcmp(src, dst, sizeof(DateTime)) == 0;
}
