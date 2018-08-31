/*!
  \file rowio/setup.c
  
  \brief RowIO library - Setup
  
  (C) 2001-2009 by the GRASS Development Team
  
  This program is free software under the GNU General Public License
  (>=v2).  Read the file COPYING that comes with GRASS for details.
  
  \author Original author CERL
*/

#include <stdio.h>
#include <stdlib.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include <grass/rowio.h>

/*!
 * \brief Configure rowio structure
 *
 * Rowio_setup() initializes the ROWIO structure <i>r</i> and
 * allocates the required memory buffers. The file descriptor
 * <i>fd</i> must be open for reading. The number of rows to be held
 * in memory is <i>nrows</i>. The length in bytes of each row is
 * <i>len</i>. The routine which will be called to read data from the
 * file is getrow() and must be provided by the programmer. If the
 * application requires that the rows be written back into the file if
 * changed, the file descriptor <i>fd</i> must be open for write as
 * well, and the programmer must provide a putrow() routine to write
 * the data into the file. If no writing of the file is to occur,
 * specify NULL for putrow().
 *
 * \param R pointer to ROWIO structure
 * \param fd file descriptor
 * \param nrows number of rows
 * \param getrow get row function
 *
 * \return 1 on success
 * \return -1 no memory
 */

int Rowio_setup(ROWIO * R,
		int fd, int nrows, int len,
		int (*getrow) (int, void *, int, int),
		int (*putrow) (int, const void *, int, int))
{
    int i;

    R->getrow = getrow;
    R->putrow = putrow;
    R->nrows = nrows;
    R->len = len;
    R->cur = -1;
    R->buf = NULL;
    R->fd = fd;

    R->rcb = (struct ROWIO_RCB *) G_malloc(nrows * sizeof(struct ROWIO_RCB));
    if (R->rcb == NULL) {
	G_warning(_("Out of memory"));
	return -1;
    }
    for (i = 0; i < nrows; i++) {
	R->rcb[i].buf = G_malloc(len);
	if (R->rcb[i].buf == NULL) {
	    G_warning(_("Out of memory"));
	    return -1;
	}
	R->rcb[i].row = -1;	/* mark not used */
    }
    return 1;
}
