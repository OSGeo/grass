#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>

struct whereandwhat
{
    off_t offset;
    CELL *p;
};

int recurse_cell(CELL flag, int i, int j, int nl, int ns,
		 struct whereandwhat bas[], struct whereandwhat dir[])
{
    CELL edge;
    int rc = 0;

    if (j == 0 && j >= ns - 1)
	return rc;

    if (bas[i].p[j] != flag) {
	rc = 1;
	bas[i].p[j] = flag;
    }

    if (i > 0) {
	edge = dir[i - 1].p[j - 1];
	if (bas[i - 1].p[j - 1] == -1 && !Rast_is_c_null_value(&edge) &&
	    edge == 4)
	    rc += recurse_cell(flag, i - 1, j - 1, nl, ns, bas, dir);
	edge = dir[i - 1].p[j];
	if (bas[i - 1].p[j] == -1 && !Rast_is_c_null_value(&edge) && edge == 8)
	    rc += recurse_cell(flag, i - 1, j, nl, ns, bas, dir);
	edge = dir[i - 1].p[j + 1];
	if (bas[i - 1].p[j + 1] == -1 && !Rast_is_c_null_value(&edge) &&
	    edge == 16)
	    rc += recurse_cell(flag, i - 1, j + 1, nl, ns, bas, dir);

    }

    edge = dir[i].p[j - 1];
    if (bas[i].p[j - 1] == -1 && !Rast_is_c_null_value(&edge) && edge == 2)
	rc += recurse_cell(flag, i, j - 1, nl, ns, bas, dir);

    edge = dir[i].p[j + 1];
    if (bas[i].p[j + 1] == -1 && !Rast_is_c_null_value(&edge) && edge == 32)
	rc += recurse_cell(flag, i, j + 1, nl, ns, bas, dir);

    if (i < nl - 1) {
	edge = dir[i + 1].p[j - 1];
	if (bas[i + 1].p[j - 1] == -1 && !Rast_is_c_null_value(&edge) &&
	    edge == 1)
	    rc += recurse_cell(flag, i + 1, j - 1, nl, ns, bas, dir);
	edge = dir[i + 1].p[j];
	if (bas[i + 1].p[j] == -1 && !Rast_is_c_null_value(&edge) && edge == 128)
	    rc += recurse_cell(flag, i + 1, j, nl, ns, bas, dir);
	edge = dir[i + 1].p[j + 1];
	if (bas[i + 1].p[j + 1] == -1 && !Rast_is_c_null_value(&edge) &&
	    edge == 64)
	    rc += recurse_cell(flag, i + 1, j + 1, nl, ns, bas, dir);
    }
    return rc;
}

void wtrshed(int fm, int fd, int nl, int ns, int mxbuf)
{
    int pass, repeat, flag, i, j, half, bufsz;
    int sline, nline, rdline;

    struct whereandwhat hold;
    struct whereandwhat *dir;
    struct whereandwhat *bas;

    dir = G_malloc(mxbuf * sizeof(struct whereandwhat));
    bas = G_malloc(mxbuf * sizeof(struct whereandwhat));

    bufsz = ns * sizeof(CELL);

    /* adjust maxbuf to an even number */
    half = mxbuf / 2;
    mxbuf = 2 * half;

    /* allocate buffers for drainage directions and basin areas */
    for (i = 0; i < mxbuf; i += 1)
	bas[i].p = (CELL *) G_calloc(ns, sizeof(CELL));
    for (i = 0; i < mxbuf; i += 1)
	dir[i].p = (CELL *) G_calloc(ns, sizeof(CELL));

    pass = 0;

    /* complete a downward pass */
    do {
	G_verbose_message(_("Watershed pass %d"), ++pass);
	repeat = 0;

	/* fill the buffer */
	nline = mxbuf;
	sline = 0;
	rdline = 1;
	for (i = 0; i < mxbuf; i++) {
	    bas[i].offset = dir[i].offset = (off_t) rdline *bufsz;

	    lseek(fm, bas[i].offset, SEEK_SET);
	    read(fm, bas[i].p, bufsz);

	    lseek(fd, dir[i].offset, SEEK_SET);
	    read(fd, dir[i].p, bufsz);

	    rdline++;
	}

	/* repeat for all subsequent rows except the first and last */
	for (i = 1; i < nl - 1; i += 1) {
	    /* analyse one line */
	    for (j = 1; j < ns - 1; j += 1) {
		flag = bas[sline].p[j];
		if (flag > 0)
		    if (recurse_cell(flag, sline, j, nline, ns, bas, dir) > 0)
			repeat = 1;
	    }

	    /* write one line */
	    lseek(fm, bas[sline].offset, SEEK_SET);
	    write(fm, bas[sline].p, bufsz);

	    /* If the bottom end of the buffers reach the bottom of the file, 
	     * rotate the buffers and read new lines */
	    if (rdline < nl - 1) {
		hold = bas[0];
		for (j = 1; j < mxbuf; j += 1)
		    bas[j - 1] = bas[j];
		bas[mxbuf - 1] = hold;

		hold = dir[0];
		for (j = 1; j < mxbuf; j += 1)
		    dir[j - 1] = dir[j];
		dir[mxbuf - 1] = hold;

		bas[mxbuf - 1].offset = dir[mxbuf - 1].offset =
		    (off_t) rdline *bufsz;

		lseek(fm, bas[mxbuf - 1].offset, SEEK_SET);
		read(fm, bas[mxbuf - 1].p, bufsz);

		lseek(fd, dir[mxbuf - 1].offset, SEEK_SET);
		read(fd, dir[mxbuf - 1].p, bufsz);

		rdline++;
	    }
	    /* After the buffer reaches the bottom of the file, stop reading file,
	     * just advance the pointers */
	    else {
		nline -= 1;
		sline += 1;
	    }

	}

	/* fill the buffer */
	nline = mxbuf;
	rdline = nl - 2;
	for (i = mxbuf - 1; i >= 0; i -= 1) {
	    bas[i].offset = dir[i].offset = (off_t) rdline *bufsz;

	    lseek(fm, bas[i].offset, SEEK_SET);
	    read(fm, bas[i].p, bufsz);

	    lseek(fd, dir[i].offset, SEEK_SET);
	    read(fd, dir[i].p, bufsz);

	    rdline--;
	}

	/* repeat */
	for (i = nl - 2; i >= 1; i -= 1) {
	    /* analyse one line */
	    for (j = 1; j < ns - 1; j += 1) {
		flag = bas[nline - 1].p[j];
		if (flag > 0)
		    if (recurse_cell(flag, nline - 1, j, nline, ns, bas, dir)
			> 0)
			repeat = 1;
	    }

	    /* write one line */
	    lseek(fm, bas[nline - 1].offset, SEEK_SET);
	    write(fm, bas[nline - 1].p, bufsz);

	    /* Until the top of the buffers reach the top of the file, 
	     * rotate the buffers and read new lines */
	    if (rdline >= 1) {
		hold = bas[nline - 1];
		for (j = nline - 1; j > 0; j -= 1)
		    bas[j] = bas[j - 1];
		bas[0] = hold;

		hold = dir[nline - 1];
		for (j = nline - 1; j > 0; j -= 1)
		    dir[j] = dir[j - 1];
		dir[0] = hold;

		bas[0].offset = dir[0].offset = (off_t) rdline *bufsz;

		lseek(fm, bas[0].offset, SEEK_SET);
		read(fm, bas[0].p, bufsz);

		lseek(fd, dir[0].offset, SEEK_SET);
		read(fd, dir[0].p, bufsz);

		rdline--;
	    }
	    /* after the buffer reaches the top of the file, just decrease the
	     * line count */
	    else
		nline -= 1;

	}

    } while (repeat);

    /* allocate buffers for drainage directions and basin areas */
    for (i = 0; i < mxbuf; i += 1)
	G_free(bas[i].p);
    for (i = 0; i < mxbuf; i += 1)
	G_free(dir[i].p);

    G_free(dir);
    G_free(bas);
}
