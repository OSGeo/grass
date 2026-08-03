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
 * Copies the DateTime [into/from ???]  src
 *
 *  \param dst
 *  \param src
 *  \return void
 */
void datetime_copy(DateTime *dst, const DateTime *src)
{
    memcpy(dst, src, sizeof(DateTime));
}
