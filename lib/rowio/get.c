/*!
  \file rowio/get.c
  
  \brief RowIO library - Get a row
  
  (C) 2001-2009 by the GRASS Development Team
  
  This program is free software under the GNU General Public License
  (>=v2).  Read the file COPYING that comes with GRASS for details.
  
  \author Original author CERL
*/

#include <stdio.h>
#include <grass/rowio.h>

static void *my_select(ROWIO *, int);
static void pageout(ROWIO *, int);


/*!
 * \brief Read a row
 *
 * Rowio_get() returns a buffer which holds the data for row from the
 * file associated with ROWIO structure <i>R</i>. If the row requested
 * is not in memory, the getrow() routine specified in
 * Rowio_setup() is called to read row into memory and a
 * pointer to the memory buffer containing the row is returned. If the
 * data currently in the buffer had been changed by Rowio_put(),
 * the putrow() routine specified in Rowio_setup() is
 * called first to write the changed row to disk. If row is
 * already in memory, no disk read is done. The pointer to the data is
 * simply returned.  
 *
 * \param R pointer to ROWIO structure
 * \param row row number
 *
 * \return NULL on error
 * \return pointer to the buffer containing row
 */
void *Rowio_get(ROWIO * R, int row)
{
    int i;
    int age;
    int cur;

    if (row < 0)
	return NULL;

    if (row == R->cur)
	return R->buf;

    for (i = 0; i < R->nrows; i++)
	if (row == R->rcb[i].row)
	    return my_select(R, i);

    age = 0;
    cur = 0;

    for (i = 0; i < R->nrows; i++)
	if (R->rcb[i].row < 0) {	/* free slot ! */
	    cur = i;
	    break;
	}
	else if (age < R->rcb[i].age) {
	    cur = i;
	    age = R->rcb[i].age;
	}

    pageout(R, cur);

    i = (*R->getrow) (R->fd, R->rcb[cur].buf, R->rcb[cur].row = row, R->len);
    R->rcb[cur].dirty = 0;
    if (!i) {
	R->rcb[cur].row = -1;
	if (cur == R->cur)
	    R->cur = -1;
	return NULL;
    }

    return my_select(R, cur);
}

/*!
  \brief Flush data

  \param R pointer to ROWIO strcuture
*/
void Rowio_flush(ROWIO * R)
{
    int i;

    for (i = 0; i < R->nrows; i++)
	pageout(R, i);
}

static void pageout(ROWIO * R, int cur)
{
    if (R->rcb[cur].row < 0)
	return;
    if (!R->rcb[cur].dirty)
	return;
    (*R->putrow) (R->fd, R->rcb[cur].buf, R->rcb[cur].row, R->len);
    R->rcb[cur].dirty = 0;
}

static void *my_select(ROWIO * R, int n)
{
    int i;

    R->rcb[n].age = 0;
    for (i = 0; i < R->nrows; i++)
	R->rcb[i].age++;
    R->cur = R->rcb[n].row;
    R->buf = R->rcb[n].buf;
    return R->buf;
}
