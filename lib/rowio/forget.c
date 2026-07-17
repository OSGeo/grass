/*!
   \file rowio/forget.c

   \brief RowIO library - Release a row

   SPDX-FileCopyrightText: 2001-2009 Other GRASS authors
   SPDX-License-Identifier: GPL-2.0-or-later

   \author Original author CERL
 */

#include <grass/rowio.h>

/*!
   \brief Release row from memory

   \param R pointer to ROWIO structure
   \param row row number
 */
void Rowio_forget(ROWIO *R, int row)
{
    int i;

    if (row < 0)
        return;

    for (i = 0; i < R->nrows; i++)
        if (row == R->rcb[i].row) {
            R->rcb[i].row = -1; /* no longer in memory */
            break;
        }
}
