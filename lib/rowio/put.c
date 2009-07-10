/*!
  \file rowio/put.c
  
  \brief RowIO library - Write a row
  
  (C) 2001-2009 by the GRASS Development Team
  
  This program is free software under the GNU General Public License
  (>=v2).  Read the file COPYING that comes with GRASS for details.
  
  \author Original author CERL
*/

#include <stdio.h>
#include <string.h>
#include <grass/rowio.h>


/*!
 * \brief Write a row
 *
 * Writes the buffer <i>buf</i>, which holds the data for row
 * <i>n</i>, into the ROWIO structure <i>r</i>. If the row requested
 * is currently in memory, the buffer is simply copied into the
 * structure and marked as having been changed. It will be written out
 * later. Otherwise it is written immediately.  Note that when the row
 * is finally written to disk, the putrow() routine specified in
 * Rowio_setup() is called to write row <i>n</i> to the
 * file. Rowio_flush() force pending updates to disk ROWIO *r;
 * Rowio_flush() forces all rows modified by Rowio_put() to be
 * written to the file. This routine must be called before closing the
 * file or releasing the rowio structure if Rowio_put() has been
 * called.
 *
 * \param R pointer to ROWIO structure
 * \param buf pointer to data buffer
 * \param row row number
 *
 * \return
 */

int Rowio_put(ROWIO * R, const void *buf, int row)
{
    int i;

    if (row < 0)
	return 0;

    for (i = 0; i < R->nrows; i++)
	if (row == R->rcb[i].row) {
	    memcpy(R->rcb[i].buf, buf, R->len);
	    R->rcb[i].dirty = 1;
	    return 1;
	}
    return ((*R->putrow) (R->fd, buf, row, R->len));
}
