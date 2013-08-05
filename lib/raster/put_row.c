/*!
   \file raster/put_row.c

   \brief Raster library - Put raster row

   (C) 2003-2009 by the GRASS Development Team

   This program is free software under the GNU General Public License
   (>=v2).  Read the file COPYING that comes with GRASS for details.

   \author Original author CERL
 */

/**********************************************************************

 **********************************************************************/

#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include <grass/config.h>
#include <grass/raster.h>
#include <grass/glocale.h>

#include "R.h"

static void put_raster_row(int, const void *, RASTER_MAP_TYPE, int);

/*!
   \brief Writes the next row for cell/fcell/dcell file

   Writes the next row for the cell file opened on 'fd' from 'buf' All
   writes go into NEW files that exactly match the current window.  The
   file must have been opened with Rast_open_new() and be written
   sequentially, ie no skipping rows.

   When the null values are embeded into the data, corresponding cells
   are changed to 0's and the corresponding null value row is written
   into null file.

   A map cannot be copied using Rast_get_row() and
   Rast_put_row(). The former resamples the data of the original
   map into a row buffer that matches the current window. The later
   writes out rows associated with the window.

   Keeps track of the minimum and maximum cell value for use in
   updating the range file upon close of the cell file.  HOWEVER when
   nulls are not embeded, the cells are considered 0's as far as
   updating range is concerned, even if the corresponding cell is null
   in the resulting null file, so programmer should be carefult to set
   all the null values using Rast_set_null_value() or
   G_insert_d_null_values() or G_insert_f_null_values().

   \param fd file descriptor where data is to be written
   \param buf     buffer holding data
   \param data_type raster map type (CELL_TYPE, FCELL_TYPE, DCELL_TYPE)

   \return void
 */
void Rast_put_row(int fd, const void *buf, RASTER_MAP_TYPE data_type)
{
    put_raster_row(fd, buf, data_type, 0);
}

/*!
   \brief Writes the next row for cell file (CELL version)

   See Rast_put_row() for details.

   \param fd file descriptor where data is to be written
   \param buf     buffer holding data

   \return void
 */
void Rast_put_c_row(int fd, const CELL * buf)
{
    Rast_put_row(fd, buf, CELL_TYPE);
}

/*!
   \brief Writes the next row for fcell file (FCELL version)

   See Rast_put_row() for details.

   \param fd file descriptor where data is to be written
   \param buf     buffer holding data

   \return void
 */
void Rast_put_f_row(int fd, const FCELL * buf)
{
    Rast_put_row(fd, buf, FCELL_TYPE);
}

/*!
   \brief Writes the next row for dcell file (DCELL version)

   See Rast_put_row() for details.

   \param fd file descriptor where data is to be written
   \param buf     buffer holding data

   \return void
 */
void Rast_put_d_row(int fd, const DCELL * buf)
{
    Rast_put_row(fd, buf, DCELL_TYPE);
}

static void write_data(int fd, int row, unsigned char *buf, int n)
{
    struct fileinfo *fcb = &R__.fileinfo[fd];
    ssize_t nwrite = fcb->nbytes * n;

    if (write(fcb->data_fd, buf, nwrite) != nwrite)
	G_fatal_error(_("Error writing uncompressed FP data for row %d of <%s>"),
		      row, fcb->name);
}

static void write_data_compressed(int fd, int row, unsigned char *buf, int n)
{
    struct fileinfo *fcb = &R__.fileinfo[fd];
    int nwrite = fcb->nbytes * n;

    if (G_zlib_write(fcb->data_fd, buf, nwrite) < 0)
	G_fatal_error(_("Error writing compressed FP data for row %d of <%s>"),
		      row, fcb->name);
}

static void set_file_pointer(int fd, int row)
{
    struct fileinfo *fcb = &R__.fileinfo[fd];

    fcb->row_ptr[row] = lseek(fcb->data_fd, 0L, SEEK_CUR);
}

static void convert_float(float *work_buf, int size, char *null_buf,
			  const FCELL *rast, int row, int n)
{
    int i;

    for (i = 0; i < n; i++) {
	FCELL f;

	/* substitute embeded null vals by 0's */
	if (Rast_is_f_null_value(&rast[i])) {
	    f = 0.;
	    null_buf[i] = 1;
	}
	else
	    f = rast[i];

	G_xdr_put_float(&work_buf[i], &f);
    }
}

static void convert_double(double *work_buf, int size, char *null_buf,
			   const DCELL *rast, int row, int n)
{
    int i;

    for (i = 0; i < n; i++) {
	DCELL d;

	/* substitute embeded null vals by 0's */
	if (Rast_is_d_null_value(&rast[i])) {
	    d = 0.;
	    null_buf[i] = 1;
	}
	else
	    d = rast[i];

	G_xdr_put_double(&work_buf[i], &d);
    }
}

/* writes data to fcell file for either full or partial rows */
static void put_fp_data(int fd, char *null_buf, const void *rast,
			int row, int n, RASTER_MAP_TYPE data_type)
{
    struct fileinfo *fcb = &R__.fileinfo[fd];
    int compressed = (fcb->open_mode == OPEN_NEW_COMPRESSED);
    int size = fcb->nbytes * fcb->cellhd.cols;
    void *work_buf;

    if (row < 0 || row >= fcb->cellhd.rows)
	return;

    if (n <= 0)
	return;

    work_buf = G__alloca(size + 1);

    if (compressed)
	set_file_pointer(fd, row);

    if (data_type == FCELL_TYPE)
	convert_float(work_buf, size, null_buf, rast, row, n);
    else
	convert_double(work_buf, size, null_buf, rast, row, n);

    if (compressed)
	write_data_compressed(fd, row, work_buf, n);
    else
	write_data(fd, row, work_buf, n);

    G__freea(work_buf);
}

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
	if (Rast_is_c_null_value(&v)) {
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

static void put_data(int fd, char *null_buf, const CELL * cell,
		     int row, int n, int zeros_r_nulls)
{
    struct fileinfo *fcb = &R__.fileinfo[fd];
    int compressed = fcb->cellhd.compressed;
    int len = compressed ? sizeof(CELL) : fcb->nbytes;
    unsigned char *work_buf, *wk;
    ssize_t nwrite;

    if (row < 0 || row >= fcb->cellhd.rows)
	return;

    if (n <= 0)
	return;

    work_buf = G__alloca(fcb->cellhd.cols * sizeof(CELL) + 1);
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
	    ? rle_compress(compressed_buf + 1, work_buf + 1, n, nbytes)
	    : zlib_compress(compressed_buf + 1, work_buf + 1, n, nbytes);

	if (nwrite > 0) {
	    nwrite++;

	    if (write(fcb->data_fd, compressed_buf, nwrite) != nwrite)
		G_fatal_error(_("Error writing compressed data for row %d of <%s>"),
			      row, fcb->name);
	}
	else {
	    nwrite = nbytes * n + 1;
	    if (write(fcb->data_fd, work_buf, nwrite) != nwrite)
		G_fatal_error(_("Error writing compressed data for row %d of <%s>"),
			      row, fcb->name);
	}

	G__freea(compressed_buf);
    }
    else {
	nwrite = fcb->nbytes * n;

	if (write(fcb->data_fd, work_buf, nwrite) != nwrite)
	    G_fatal_error(_("Error writing uncompressed data for row %d of <%s>"),
			  row, fcb->name);
    }

    G__freea(work_buf);
}

static void put_data_gdal(int fd, const void *rast, int row, int n,
			  int zeros_r_nulls, RASTER_MAP_TYPE map_type)
{
#ifdef HAVE_GDAL
    struct fileinfo *fcb = &R__.fileinfo[fd];
    int size = Rast_cell_size(map_type);
    DCELL null_val = fcb->gdal->null_val;
    const void *src;
    void *work_buf, *dst;
    GDALDataType datatype;
    CPLErr err;
    int i;

    if (row < 0 || row >= fcb->cellhd.rows)
	return;

    if (n <= 0)
	return;

    work_buf = G__alloca(n * size);

    switch (map_type) {
    case CELL_TYPE:
	datatype = GDT_Int32;
	break;
    case FCELL_TYPE:
	datatype = GDT_Float32;
	break;
    case DCELL_TYPE:
	datatype = GDT_Float64;
	break;
    }

    src = rast;
    dst = work_buf;

    for (i = 0; i < n; i++) {
	if (Rast_is_null_value(src, map_type) ||
	    (zeros_r_nulls && !*(CELL *) src))
	    Rast_set_d_value(dst, null_val, map_type);
	else
	    memcpy(dst, src, size);
	src = G_incr_void_ptr(src, size);
	dst = G_incr_void_ptr(dst, size);
    }

    err = Rast_gdal_raster_IO(fcb->gdal->band, GF_Write, 0, row, n, 1,
			      work_buf, n, 1, datatype, 0, 0);

    G__freea(work_buf);

    if (err != CE_None)
	G_fatal_error(_("Error writing data via GDAL for row %d of <%s>"),
		      row, fcb->name);
#endif
}

static void put_raster_data(int fd, char *null_buf, const void *rast,
			    int row, int n,
			    int zeros_r_nulls, RASTER_MAP_TYPE map_type)
{
    struct fileinfo *fcb = &R__.fileinfo[fd];

    if (fcb->gdal)
	put_data_gdal(fd, rast, row, n, zeros_r_nulls, map_type);
    else if (map_type == CELL_TYPE)
	put_data(fd, null_buf, rast, row, n, zeros_r_nulls);
    else
	put_fp_data(fd, null_buf, rast, row, n, map_type);
}

static void put_null_data(int fd, const char *flags, int row)
{
    struct fileinfo *fcb = &R__.fileinfo[fd];

    if (fcb->null_fd < 0)
	G_fatal_error(_("No null file for <%s>"), fcb->name);

    Rast__convert_01_flags(flags, fcb->null_bits,
			   fcb->cellhd.cols);

    Rast__write_null_bits(fcb->null_fd, fcb->null_bits, row,
			  fcb->cellhd.cols, fd);
}

static void put_null_value_row(int fd, const char *buf)
{
    struct fileinfo *fcb = &R__.fileinfo[fd];

    if (fcb->gdal)
	G_fatal_error(_("GDAL output doesn't support writing null rows separately"));

    put_null_data(fd, buf, fcb->null_cur_row);

    fcb->null_cur_row++;
}

/*!
   \brief Open null file for write

   \param fd file descriptor of raster cell data file

   \return field descriptor of null data file
 */
int Rast__open_null_write(int fd)
{
    struct fileinfo *fcb = &R__.fileinfo[fd];
    int null_fd;

    if (access(fcb->null_temp_name, 0) != 0)
	G_fatal_error(_("Unable to find a temporary null file <%s>"),
		      fcb->null_temp_name);

    null_fd = open(fcb->null_temp_name, O_WRONLY);
    if (null_fd < 0)
	G_fatal_error(_("Unable to open null file <%s>"),
		      fcb->null_temp_name);

    return null_fd;
}

/*!
   \brief Write null data

   \param null_fd file descriptor of null file where data is to be written
   \param flags ?
   \param row row number
   \param col col number
   \param fd file descriptor of cell data file

   \return void
 */
void Rast__write_null_bits(int null_fd, const unsigned char *flags, int row,
			   int cols, int fd)
{
    off_t offset;
    size_t size;

    size = Rast__null_bitstream_size(cols);
    offset = (off_t) size *row;

    if (lseek(null_fd, offset, SEEK_SET) < 0)
	G_fatal_error(_("Error writing null row %d"), row);

    if (write(null_fd, flags, size) != size)
	G_fatal_error(_("Error writing null row %d"), row);
}

static void convert_and_write_if(int fd, const void *vbuf)
{
    const CELL *buf = vbuf;
    struct fileinfo *fcb = &R__.fileinfo[fd];
    FCELL *p = (FCELL *) fcb->data;
    int i;

    for (i = 0; i < fcb->cellhd.cols; i++)
	if (Rast_is_c_null_value(&buf[i]))
	    Rast_set_f_null_value(&p[i], 1);
	else
	    p[i] = (FCELL) buf[i];

    Rast_put_f_row(fd, p);
}

static void convert_and_write_df(int fd, const void *vbuf)
{
    const DCELL *buf = vbuf;
    struct fileinfo *fcb = &R__.fileinfo[fd];
    FCELL *p = (FCELL *) fcb->data;
    int i;

    for (i = 0; i < fcb->cellhd.cols; i++)
	if (Rast_is_d_null_value(&buf[i]))
	    Rast_set_f_null_value(&p[i], 1);
	else
	    p[i] = (FCELL) buf[i];

    Rast_put_f_row(fd, p);
}

static void convert_and_write_id(int fd, const void *vbuf)
{
    const CELL *buf = vbuf;
    struct fileinfo *fcb = &R__.fileinfo[fd];
    DCELL *p = (DCELL *) fcb->data;
    int i;

    for (i = 0; i < fcb->cellhd.cols; i++)
	if (Rast_is_c_null_value(&buf[i]))
	    Rast_set_d_null_value(&p[i], 1);
	else
	    p[i] = (DCELL) buf[i];

    Rast_put_d_row(fd, p);
}

static void convert_and_write_fd(int fd, const void *vbuf)
{
    const FCELL *buf = vbuf;
    struct fileinfo *fcb = &R__.fileinfo[fd];
    DCELL *p = (DCELL *) fcb->data;
    int i;

    for (i = 0; i < fcb->cellhd.cols; i++)
	if (Rast_is_f_null_value(&buf[i]))
	    Rast_set_d_null_value(&p[i], 1);
	else
	    p[i] = (DCELL) buf[i];

    Rast_put_d_row(fd, p);
}

static void convert_and_write_fi(int fd, const void *vbuf)
{
    const FCELL *buf = vbuf;
    struct fileinfo *fcb = &R__.fileinfo[fd];
    CELL *p = (CELL *) fcb->data;
    int i;

    for (i = 0; i < fcb->cellhd.cols; i++)
	if (Rast_is_f_null_value(&buf[i]))
	    Rast_set_c_null_value(&p[i], 1);
	else
	    p[i] = (CELL) buf[i];

    Rast_put_c_row(fd, p);
}

static void convert_and_write_di(int fd, const void *vbuf)
{
    const DCELL *buf = vbuf;
    struct fileinfo *fcb = &R__.fileinfo[fd];
    CELL *p = (CELL *) fcb->data;
    int i;

    for (i = 0; i < fcb->cellhd.cols; i++)
	if (Rast_is_d_null_value(&buf[i]))
	    Rast_set_c_null_value(&p[i], 1);
	else
	    p[i] = (CELL) buf[i];

    Rast_put_c_row(fd, p);
}

/*!
   \brief converts a buffer of zero's and ones to bitstream.

   Stores this bitstream in memory. (the null rows from memory are
   written into null file after the limit is reached, and the place for
   new null rows to be kept in memory is freed. Should not be used by
   application programs.

   \param fd file descriptor where data is to be written
   \param buf buffer holding null data

   \return void
 */
static void put_raster_row(int fd, const void *buf, RASTER_MAP_TYPE data_type,
			   int zeros_r_nulls)
{
    static void (*convert_and_write_FtypeOtype[3][3])(int, const void *) = {
	{NULL, convert_and_write_if, convert_and_write_id},
	{convert_and_write_fi, NULL, convert_and_write_fd},
	{convert_and_write_di, convert_and_write_df, NULL}
    };
    struct fileinfo *fcb = &R__.fileinfo[fd];
    char *null_buf;

    switch (fcb->open_mode) {
    case OPEN_OLD:
	G_fatal_error(_("put_raster_row: raster map <%s> not open for write - request ignored"),
		      fcb->name);
	break;
    case OPEN_NEW_COMPRESSED:
    case OPEN_NEW_UNCOMPRESSED:
	break;
    default:
	G_fatal_error(_("put_raster_row: unopened file descriptor - request ignored"));
	break;
    }

    if (fcb->map_type != data_type) {
	convert_and_write_FtypeOtype[data_type][fcb->map_type](fd, buf);
	return;
    }

    null_buf = G__alloca(fcb->cellhd.cols);
    G_zero(null_buf, fcb->cellhd.cols);

    put_raster_data(fd, null_buf, buf, fcb->cur_row, fcb->cellhd.cols,
		    zeros_r_nulls, data_type);

    /* only for integer maps */
    if (data_type == CELL_TYPE) {
	if (fcb->want_histogram)
	    Rast_update_cell_stats(buf, fcb->cellhd.cols, &fcb->statf);
	Rast__row_update_range(buf, fcb->cellhd.cols, &fcb->range,
			       zeros_r_nulls);
    }
    else
	Rast_row_update_fp_range(buf, fcb->cellhd.cols, &fcb->fp_range,
				 data_type);

    fcb->cur_row++;

    /* write the null row for the data row */
    if (!fcb->gdal)
	put_null_value_row(fd, null_buf);

    G__freea(null_buf);
}
