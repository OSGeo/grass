
/**********************************************************************
 *
 *   G_zeros_r_nulls(zeros_r_nulls)
 *      int zeros_r_nulls	the last argument of put_data()
 *
 *   zeros_r_nulls > 0		zero values of buf to be written into files
 *   				are null values by default.
 *
 *   zeros_r_nulls == 0		zero values are just zero itself.
 *
 *   zeros_r_nulls < 0		do not set. return current setting.
 *   				1: set
 *   				0: not set
 *
 *   Return setting values in all cases.
 *
 *   *** NOTE *** 
 *   Use only to change a default behavior for zero of G_put_map_row
 *
 ********************************************************************** 
 *
 *   G_put_[c/f/d]_raster_row(fd, buf)
 *      int fd           file descriptor of the opened map
 *      [F/D]CELL *buf   buffer holding row info to be written
 *
 *   Writes the next row for the cell file opened on 'fd' from 'buf'
 *   All writes go into NEW files that exactly match the current window.
 *   The file must have been opened with G_open_cell_new()
 *   and be written sequentially, ie no skipping rows
 *
 *   when the null values are embeded into the data, corresponding cells are 
 *   changed to 0's and the corresponding null value row is written into null 
 *   file.
 *
 *   *** NOTE *** 
 *   A map cannot be copied using G_get_raster_row() and G_put_raster_row().
 *   The former resamples the data of the original map into a row buffer
 *   that matches the current window.  The later writes out rows associated
 *   with the window.
 *
 *   returns:    1  if successful
 *              -1  on fail
 *
 *  Keeps track of the minimum and maximum cell value  for use in updating
 *  the range file upon close of the cell file.
 *  HOWEVER when nulls are not embeded, the cells are considered 0's as far
 *  as updating range is concerned, even if the corresponding cell is null
 *  in the resulting null file, so programmer should be carefult to set all 
 *  the null values using G_set_null_value() or G_insert_[d/f_]null_values()
 *
 ********************************************************************** 
 *
 *   G_put_map_row(fd, buf)
 *      int fd           file descriptor of the opened map
 *      CELL *buf        buffer holding row info to be written
 *
 *   Writes the next row for the cell file opened on 'fd' from 'buf'
 *   All writes go into NEW files that exactly match the current window.
 *   The file must have been opened with G_open_cell_new()
 *   and be written sequentially, ie no skipping rows
 *
 *   NULLS are written into null bitmap file for all cells which are zero,
 *   and cells which have null value (these cells are converted to 0's before
 *   writing) 
 *
 *   *** NOTE *** 
 *   A map cannot be copied using G_get_map_row() and G_put_map_row().
 *   The former resamples the data of the original map into a row buffer
 *   that matches the current window.  The later writes out rows associated
 *   with the window.
 *
 *   returns:    1  if successful
 *              -1  on fail
 *
 *  Keeps track of the minimum and maximum cell value  for use in updating
 *  the range file upon close of the cell file.
 *
 ***********************************************************************
 *
 *  G__put_null_value_row(fd, buf)
 *      int fd                  File descriptor where data is to be written
 *      char *buf               Buffer holding null data
 *
 *   converts a buffer of zero's and ones to bitstream and stores this 
 *   bitstream in memory. (the null rows from memory are written into null
 *   file after the limit is reached, and the place for new null rows
 *   to be kept in memory is freed. Should not be used by application
 *   programs.
 *
 *   returns:    0  if successful
 *              -1  on fail
 **********************************************************************/

#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include <grass/config.h>

#include "G.h"
#include <grass/glocale.h>

static int _zeros_r_nulls = 1;

static int put_raster_data(int, char *, const void *, int, int, int,
			   RASTER_MAP_TYPE);
static int put_data(int, char *, const CELL *, int, int, int);
static int check_open(const char *, int);
static void write_error(int, int);
static int same(const unsigned char *, const unsigned char *, int);
static void set_file_pointer(int, int);
static int put_fp_data(int, char *, const void *, int, int, RASTER_MAP_TYPE);
static int put_null_data(int, const char *, int);
static int convert_and_write_if(int, const CELL *);
static int convert_and_write_id(int, const CELL *);
static int convert_and_write_df(int, const DCELL *);
static int convert_and_write_fd(int, const FCELL *);
static int put_raster_row(int fd, const void *buf, RASTER_MAP_TYPE data_type,
			  int zeros_r_nulls);

/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/

int G_zeros_r_nulls(int zeros_r_nulls)
{
    if (zeros_r_nulls >= 0)
	_zeros_r_nulls = zeros_r_nulls > 0;

    return _zeros_r_nulls;
}

int G__put_null_value_row(int fd, const char *buf)
{
    struct fileinfo *fcb = &G__.fileinfo[fd];

    switch (put_null_data(fd, buf, fcb->null_cur_row)) {
    case -1:
	return -1;
    case 0:
	return 1;
    }

    fcb->null_cur_row++;

    return 1;
}

int G_put_map_row(int fd, const CELL * buf)
{
    struct fileinfo *fcb = &G__.fileinfo[fd];

    if (fcb->map_type != CELL_TYPE) {
	G_fatal_error(_("G_put_map_row: %s is not integer! Use G_put_[f/d]_raster_row()!"),
		      fcb->name);
	return -1;
    }

    return put_raster_row(fd, buf, CELL_TYPE, _zeros_r_nulls);
}

int G_put_raster_row(int fd, const void *buf, RASTER_MAP_TYPE data_type)
{
    return put_raster_row(fd, buf, data_type, 0);
}

int G_put_c_raster_row(int fd, const CELL * buf)
{
    return G_put_raster_row(fd, buf, CELL_TYPE);
}

int G_put_f_raster_row(int fd, const FCELL * buf)
{
    return G_put_raster_row(fd, buf, FCELL_TYPE);
}

int G_put_d_raster_row(int fd, const DCELL * buf)
{
    return G_put_raster_row(fd, buf, DCELL_TYPE);
}

/*--------------------------------------------------------------------------*/

static int check_open(const char *me, int fd)
{
    struct fileinfo *fcb = &G__.fileinfo[fd];

    switch (fcb->open_mode) {
    case OPEN_OLD:
	G_warning(_("%s: map [%s] not open for write - request ignored"), me,
		  fcb->name);
	break;
    case OPEN_NEW_COMPRESSED:
    case OPEN_NEW_UNCOMPRESSED:
	return 1;
	break;
    default:
	G_warning(_("%s: unopened file descriptor - request ignored"), me);
	break;
    }

    return 0;
}

static void write_error(int fd, int row)
{
    struct fileinfo *fcb = &G__.fileinfo[fd];

    if (fcb->io_error)
	return;

    G_warning(_("map [%s] - unable to write row %d"), fcb->name, row);

    fcb->io_error = 1;

    return;
}

/*--------------------------------------------------------------------------*/

static int write_data(int fd, int row, unsigned char *buf, int n)
{
    struct fileinfo *fcb = &G__.fileinfo[fd];
    ssize_t nwrite = fcb->nbytes * n;

    if (write(fd, buf, nwrite) != nwrite) {
	write_error(fd, row);
	return -1;
    }

    return 0;
}

static int write_data_compressed(int fd, int row, unsigned char *buf, int n)
{
    struct fileinfo *fcb = &G__.fileinfo[fd];
    int nwrite = fcb->nbytes * n;

    if (G_zlib_write(fd, buf, nwrite) < 0) {
	write_error(fd, row);
	return -1;
    }

    return 0;
}

/*--------------------------------------------------------------------------*/

static void set_file_pointer(int fd, int row)
{
    struct fileinfo *fcb = &G__.fileinfo[fd];

    fcb->row_ptr[row] = lseek(fd, 0L, SEEK_CUR);
}

/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/

static int convert_float(XDR * xdrs, char *null_buf, const FCELL * rast,
			 int row, int n)
{
    int i;

    for (i = 0; i < n; i++) {
	FCELL f;

	/* substitute embeded null vals by 0's */
	if (G_is_f_null_value(&rast[i])) {
	    f = 0.;
	    null_buf[i] = 1;
	}
	else
	    f = rast[i];

	if (!xdr_float(xdrs, &f)) {
	    G_warning(_("xdr_float failed for index %d of row %d"), i, row);
	    return -1;
	}
    }

    return 0;
}

static int convert_double(XDR * xdrs, char *null_buf, const DCELL * rast,
			  int row, int n)
{
    int i;

    for (i = 0; i < n; i++) {
	DCELL d;

	/* substitute embeded null vals by 0's */
	if (G_is_d_null_value(&rast[i])) {
	    d = 0.;
	    null_buf[i] = 1;
	}
	else
	    d = rast[i];

	if (!xdr_double(xdrs, &d)) {
	    G_warning(_("xdr_double failed for index %d of row %d"), i, row);
	    return -1;
	}
    }

    return 0;
}

/*--------------------------------------------------------------------------*/

/* writes data to fcell file for either full or partial rows */

static int put_fp_data(int fd, char *null_buf, const void *rast,
		       int row, int n, RASTER_MAP_TYPE data_type)
{
    struct fileinfo *fcb = &G__.fileinfo[fd];
    int compressed = (fcb->open_mode == OPEN_NEW_COMPRESSED);
    XDR *xdrs = &fcb->xdrstream;
    char *work_buf;

    if (row < 0 || row >= fcb->cellhd.rows)
	return 0;

    if (n <= 0)
	return 0;

    work_buf = G__alloca(G__.window.cols * fcb->nbytes + 1);

    if (compressed)
	set_file_pointer(fd, row);

    xdrmem_create(xdrs, work_buf,
		  (unsigned int) fcb->nbytes * fcb->cellhd.cols, XDR_ENCODE);
    xdr_setpos(xdrs, 0);

    if (data_type == FCELL_TYPE) {
	if (convert_float(xdrs, null_buf, rast, row, n) < 0) {
	    G__freea(work_buf);
	    return -1;
	}
    }
    else {
	if (convert_double(xdrs, null_buf, rast, row, n) < 0) {
	    G__freea(work_buf);
	    return -1;
	}
    }

    xdr_destroy(&fcb->xdrstream);

    if (compressed) {
	if (write_data_compressed(fd, row, work_buf, n) == -1) {
	    G__freea(work_buf);
	    return -1;
	}
    }
    else if (write_data(fd, row, work_buf, n) == -1) {
	G__freea(work_buf);
	return -1;
    }

    G__freea(work_buf);

    return 1;
}

/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/

static void convert_int(unsigned char *wk, char *null_buf, const CELL * rast,
			int n, int len, int zeros_r_nulls)
{
    int i;

    /* transform CELL data into non-machine dependent multi-byte format */

    for (i = 0; i < n; i++) {
	CELL v = rast[i];
	int neg;
	int k;

	/* substitute embeded null vals by 0's */
	if (G_is_c_null_value(&v)) {
	    v = 0;
	    null_buf[i] = 1;
	}
	else if (zeros_r_nulls && !v)
	    null_buf[i] = 1;

	/* negatives */
	if (v < 0) {
	    neg = 1;
	    v = -v;
	}
	else
	    neg = 0;

	/* copy byte by byte */
	for (k = len - 1; k >= 0; k--) {
	    wk[k] = v & 0xff;
	    v >>= 8;
	}

	/* set negative bit in first byte */
	if (neg)
	    wk[0] |= 0x80;

	wk += len;
    }
}

static int count_bytes(const unsigned char *wk, int n, int len)
{
    int i, j;

    for (i = 0; i < len - 1; i++)
	for (j = 0; j < n; j++)
	    if (wk[j * len + i] != 0)
		return len - i;

    return 1;
}

static void trim_bytes(unsigned char *wk, int n, int slen, int trim)
{
    unsigned char *wk2 = wk;
    int i, j;

    for (i = 0; i < n; i++) {
	for (j = 0; j < trim; j++)
	    wk++;
	for (; j < slen; j++)
	    *wk2++ = *wk++;
    }
}

static int same(const unsigned char *x, const unsigned char *y, int n)
{
    return (memcmp(x, y, n) == 0);
}

static int count_run(const unsigned char *src, int n, int nbytes)
{
    const unsigned char *cur = src + nbytes;
    int i;

    for (i = 1; i < n; i++) {
	if (i == 255 || !same(cur, src, nbytes))
	    return i;

	cur += nbytes;
    }

    return n;
}

static int rle_compress(unsigned char *dst, unsigned char *src, int n,
			int nbytes)
{
    int nwrite = 0;
    int total = nbytes * n;

    while (n > 0) {
	int count;

	nwrite += nbytes + 1;
	if (nwrite >= total)
	    return 0;

	count = count_run(src, n, nbytes);

	*dst++ = count;
	memcpy(dst, src, nbytes);
	dst += nbytes;

	src += count * nbytes;
	n -= count;
    }

    return nwrite;
}

static int zlib_compress(unsigned char *dst, unsigned char *src, int n,
			 int nbytes)
{
    int total = nbytes * n;
    int nwrite = G_zlib_compress(src, total, dst, total);

    return (nwrite >= total) ? 0 : nwrite;
}

/*--------------------------------------------------------------------------*/

static int put_data(int fd, char *null_buf, const CELL *cell,
		    int row, int n, int zeros_r_nulls)
{
    struct fileinfo *fcb = &G__.fileinfo[fd];
    int compressed = fcb->cellhd.compressed;
    int len = compressed ? sizeof(CELL) : fcb->nbytes;
    unsigned char *work_buf, *wk;
    ssize_t nwrite;

    if (row < 0 || row >= fcb->cellhd.rows)
	return 0;

    if (n <= 0)
	return 0;

    work_buf = G__alloca(G__.window.cols * sizeof(CELL) + 1);
    wk = work_buf;

    if (compressed)
	set_file_pointer(fd, row);

    if (compressed)
	wk++;

    convert_int(wk, null_buf, cell, n, len, zeros_r_nulls);

    if (compressed) {
	unsigned char *wk = work_buf + 1;
	int nbytes = count_bytes(wk, n, len);
	unsigned char *compressed_buf;
	int total;

	if (fcb->nbytes < nbytes)
	    fcb->nbytes = nbytes;

	/* first trim away zero high bytes */
	if (nbytes < len)
	    trim_bytes(wk, n, len, len - nbytes);

	total = nbytes * n;
	compressed_buf = G__alloca(total + 1);

	compressed_buf[0] = work_buf[0] = nbytes;

	/* then compress the data */
	nwrite = compressed == 1
	    ? rle_compress(compressed_buf + 1, work_buf + 1, n,
			   nbytes)
	    : zlib_compress(compressed_buf + 1, work_buf + 1, n,
			    nbytes);

	if (nwrite > 0) {
	    nwrite++;

	    if (write(fd, compressed_buf, nwrite) != nwrite) {
		write_error(fd, row);
		G__freea(compressed_buf);
		G__freea(work_buf);
		return -1;
	    }
	}
	else {
	    nwrite = nbytes * n + 1;
	    if (write(fd, work_buf, nwrite) != nwrite) {
		write_error(fd, row);
		G__freea(compressed_buf);
		G__freea(work_buf);
		return -1;
	    }
	}

	G__freea(compressed_buf);
    }
    else {
	nwrite = fcb->nbytes * n;

	if (write(fd, work_buf, nwrite) != nwrite) {
	    write_error(fd, row);
	    G__freea(work_buf);
	    return -1;
	}
    }

    G__freea(work_buf);

    return 1;
}

/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/

static int put_raster_data(int fd, char *null_buf, const void *rast,
			   int row, int n,
			   int zeros_r_nulls, RASTER_MAP_TYPE map_type)
{
    return (map_type == CELL_TYPE)
	? put_data(fd, null_buf, rast, row, n, zeros_r_nulls)
	: put_fp_data(fd, null_buf, rast, row, n, map_type);
}

/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/

static int put_null_data(int fd, const char *flags, int row)
{
    struct fileinfo *fcb = &G__.fileinfo[fd];
    int null_fd, i;

    if (fcb->min_null_row + NULL_ROWS_INMEM <= row) {
	/* the row is out of the range of rows stored in memory */
	/* write out all the rows kept in memory, and initialize memory
	   for keeping new NULL_ROWS_INMEM rows */

	if (fcb->min_null_row >= 0) {
	    null_fd = G__open_null_write(fd);
	    if (null_fd < 0)
		return -1;

	    for (i = 0; i < NULL_ROWS_INMEM; i++) {
		/* fcb->cellhd.rows doesn't have to be a miultiple of NULL_ROWS_INMEM */
		if (i + fcb->min_null_row >= fcb->cellhd.rows)
		    break;

		if (G__write_null_bits(null_fd, fcb->NULL_ROWS[i],
				       i + fcb->min_null_row,
				       fcb->cellhd.cols, fd) < 0)
		    return -1;

	    }			/* done writing out memory rows */
	    if (null_fd >= 0)
		close(null_fd);
	}

	/* now initialize memory to store new NULL_ROWS_INMEM rows */
	fcb->min_null_row = fcb->min_null_row + NULL_ROWS_INMEM;
	/* init memory to store next NULL_ROWS_INMEM rows */
    }

    /* remember the null row for i for the future writing */
    G__convert_01_flags(flags, fcb->NULL_ROWS[row - fcb->min_null_row],
			fcb->cellhd.cols);

    return 1;
}

int G__open_null_write(int fd)
{
    struct fileinfo *fcb = &G__.fileinfo[fd];
    int null_fd;

    if (access(fcb->null_temp_name, 0) != 0) {
	G_warning(_("unable to find a temporary null file %s"),
		  fcb->null_temp_name);
	return -1;
    }

    null_fd = open(fcb->null_temp_name, O_WRONLY);
    if (null_fd < 0)
	return -1;

    return null_fd;
}

int G__write_null_bits(int null_fd, const unsigned char *flags, int row,
		       int cols, int fd)
{
    off_t offset;
    size_t size;

    size = G__null_bitstream_size(cols);
    offset = (off_t) size *row;

    if (lseek(null_fd, offset, SEEK_SET) < 0) {
	G_warning(_("error writing null row %d"), row);
	return -1;
    }

    if (write(null_fd, flags, size) != size) {
	G_warning(_("error writing null row %d"), row);
	return -1;
    }

    return 1;
}

/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/

static int convert_and_write_if(int fd, const CELL * buf)
{
    struct fileinfo *fcb = &G__.fileinfo[fd];
    FCELL *p = (FCELL *) fcb->data;
    int i;

    for (i = 0; i < fcb->cellhd.cols; i++)
	if (G_is_c_null_value(&buf[i]))
	    G_set_f_null_value(&p[i], 1);
	else
	    p[i] = (FCELL) buf[i];

    return G_put_f_raster_row(fd, p);
}

static int convert_and_write_df(int fd, const DCELL * buf)
{
    struct fileinfo *fcb = &G__.fileinfo[fd];
    FCELL *p = (FCELL *) fcb->data;
    int i;

    for (i = 0; i < fcb->cellhd.cols; i++)
	if (G_is_d_null_value(&buf[i]))
	    G_set_f_null_value(&p[i], 1);
	else
	    p[i] = (FCELL) buf[i];

    return G_put_f_raster_row(fd, p);
}

static int convert_and_write_id(int fd, const CELL * buf)
{
    struct fileinfo *fcb = &G__.fileinfo[fd];
    DCELL *p = (DCELL *) fcb->data;
    int i;

    for (i = 0; i < fcb->cellhd.cols; i++)
	if (G_is_c_null_value(&buf[i]))
	    G_set_d_null_value(&p[i], 1);
	else
	    p[i] = (DCELL) buf[i];

    return G_put_d_raster_row(fd, p);
}

static int convert_and_write_fd(int fd, const FCELL * buf)
{
    struct fileinfo *fcb = &G__.fileinfo[fd];
    DCELL *p = (DCELL *) fcb->data;
    int i;

    for (i = 0; i < fcb->cellhd.cols; i++)
	if (G_is_f_null_value(&buf[i]))
	    G_set_d_null_value(&p[i], 1);
	else
	    p[i] = (DCELL) buf[i];

    return G_put_d_raster_row(fd, p);
}

static int convert_and_write_fi(int fd, const FCELL * buf)
{
    struct fileinfo *fcb = &G__.fileinfo[fd];
    CELL *p = (CELL *) fcb->data;
    int i;

    for (i = 0; i < fcb->cellhd.cols; i++)
	if (G_is_f_null_value(&buf[i]))
	    G_set_c_null_value(&p[i], 1);
	else
	    p[i] = (CELL) buf[i];

    return G_put_c_raster_row(fd, p);
}

static int convert_and_write_di(int fd, const DCELL * buf)
{
    struct fileinfo *fcb = &G__.fileinfo[fd];
    CELL *p = (CELL *) fcb->data;
    int i;

    for (i = 0; i < fcb->cellhd.cols; i++)
	if (G_is_d_null_value(&buf[i]))
	    G_set_c_null_value(&p[i], 1);
	else
	    p[i] = (CELL) buf[i];

    return G_put_c_raster_row(fd, p);
}

/*--------------------------------------------------------------------------*/

static int put_raster_row(int fd, const void *buf, RASTER_MAP_TYPE data_type,
			  int zeros_r_nulls)
{
    static int (*convert_and_write_FtypeOtype[3][3]) () = {
	{
	NULL, convert_and_write_if, convert_and_write_id}, {
	convert_and_write_fi, NULL, convert_and_write_fd}, {
	convert_and_write_di, convert_and_write_df, NULL}
    };
    struct fileinfo *fcb = &G__.fileinfo[fd];
    char *null_buf;
    int stat;

    if (!check_open("put_raster_row", fd))
	return -1;

    if (fcb->map_type != data_type)
	return convert_and_write_FtypeOtype[data_type][fcb->map_type] (fd,
								       buf);

    null_buf = G__alloca(fcb->cellhd.cols);
    G_zero(null_buf, fcb->cellhd.cols);

    switch (put_raster_data(
		fd, null_buf, buf, fcb->cur_row, fcb->cellhd.cols,
		zeros_r_nulls, data_type)) {
    case -1:
	G__freea(null_buf);
	return -1;
    case 0:
	G__freea(null_buf);
	return 1;
    }

    /* only for integer maps */
    if (data_type == CELL_TYPE) {
	if (fcb->want_histogram)
	    G_update_cell_stats(buf, fcb->cellhd.cols, &fcb->statf);
	G__row_update_range(buf, fcb->cellhd.cols, &fcb->range,
			    zeros_r_nulls);
    }
    else
	G_row_update_fp_range(buf, fcb->cellhd.cols, &fcb->fp_range,
			      data_type);

    fcb->cur_row++;

    /* write the null row for the data row */
    stat = G__put_null_value_row(fd, null_buf);
    G__freea(null_buf);
    return stat;
}
