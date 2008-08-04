#include <stdio.h>
#include <grass/rowio.h>

static void *my_select(ROWIO *, int);
static void pageout(ROWIO *, int);


/*!
 * \brief read a row
 *
 * Rowio_get() returns a
 * buffer which holds the data for row <b>n</b> from the file associated with
 * ROWIO structure <b>r.</b> If the row requested is not in memory, the
 * <b>getrow()</b> routine specified in <i>rowio_setup</i> is called to
 * read row <b>n</b> into memory and a pointer to the memory buffer containing
 * the row is returned. If the data currently in the buffer had been changed by
 * <i>rowio_put</i>, the <b>putrow()</b> routine specified in
 * <i>rowio_setup</i> is called first to write the changed row to disk. If row
 * <b>n</b> is already in memory, no disk read is done. The pointer to the data
 * is simply returned.
 * Return codes:
 * NULL <b>n</b> is negative, or
 * <b>getrow()</b> returned 0 (indicating an error condition).
 * !NULL pointer to buffer containing row <b>n.</b>
 *
 *  \param r
 *  \param n
 *  \return char * 
 */

void *rowio_get(ROWIO * R, int row)
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

void rowio_flush(ROWIO * R)
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
