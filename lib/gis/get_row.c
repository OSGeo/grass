/*!
   \file get_row.c

   \brief GIS library - get raster row

   (C) 2003-2008 by the GRASS Development Team

   This program is free software under the 
   GNU General Public License (>=v2). 
   Read the file COPYING that comes with GRASS
   for details.

   \author Original author CERL
 */

#include <string.h>
#include <unistd.h>
#include <sys/types.h>

#include <rpc/types.h>		/* need this for sgi */
#include <rpc/xdr.h>

#include <grass/config.h>
#include <grass/glocale.h>

#include "G.h"

/*--------------------------------------------------------------------------*/

#define NULL_FILE   "null"

/*--------------------------------------------------------------------------*/

static int embed_nulls(int, void *, int, RASTER_MAP_TYPE, int, int);

/*--------------------------------------------------------------------------*/

static int compute_window_row(int fd, int row, int *cellRow)
{
    struct fileinfo *fcb = &G__.fileinfo[fd];
    double f;
    int r;

    /* check for row in window */
    if (row < 0 || row >= G__.window.rows) {
	G_warning(_("Reading raster map <%s@%s> request for row %d is outside region"),
		  fcb->name, fcb->mapset, row);

	return -1;
    }

    /* convert window row to cell file row */
    f = row * fcb->C1 + fcb->C2;
    r = (int)f;
    if (f < r)			/* adjust for rounding up of negatives */
	r--;

    if (r < 0 || r >= fcb->cellhd.rows)
	return 0;

    *cellRow = r;

    return 1;
}

/*--------------------------------------------------------------------------*/

static void do_reclass_int(int fd, void *cell, int null_is_zero)
{
    struct fileinfo *fcb = &G__.fileinfo[fd];
    CELL *c = cell;
    CELL *reclass_table = fcb->reclass.table;
    CELL min = fcb->reclass.min;
    CELL max = fcb->reclass.max;
    int i;

    for (i = 0; i < G__.window.cols; i++) {
	if (G_is_c_null_value(&c[i])) {
	    if (null_is_zero)
		c[i] = 0;
	    continue;
	}

	if (c[i] < min || c[i] > max) {
	    if (null_is_zero)
		c[i] = 0;
	    else
		G_set_c_null_value(&c[i], 1);
	    continue;
	}

	c[i] = reclass_table[c[i] - min];

	if (null_is_zero && G_is_c_null_value(&c[i]))
	    c[i] = 0;
    }
}

/*--------------------------------------------------------------------------*/

static int read_data_fp_compressed(int fd, int row, unsigned char *data_buf,
				   int *nbytes)
{
    struct fileinfo *fcb = &G__.fileinfo[fd];
    off_t t1 = fcb->row_ptr[row];
    off_t t2 = fcb->row_ptr[row + 1];
    size_t readamount = t2 - t1;
    size_t bufsize = fcb->cellhd.cols * fcb->nbytes;

    if (lseek(fd, t1, SEEK_SET) < 0)
	return -1;

    *nbytes = fcb->nbytes;

    if ((size_t) G_zlib_read(fd, readamount, data_buf, bufsize) != bufsize)
	return -1;

    return 0;
}

/*--------------------------------------------------------------------------*/

static void rle_decompress(unsigned char *dst, const unsigned char *src,
			   int nbytes, int size)
{
    int pairs = size / (nbytes + 1);
    int i;

    for (i = 0; i < pairs; i++) {
	int repeat = *src++;
	int j;

	for (j = 0; j < repeat; j++) {
	    memcpy(dst, src, nbytes);
	    dst += nbytes;
	}

	src += nbytes;
    }
}

static int read_data_compressed(int fd, int row, unsigned char *data_buf,
				int *nbytes)
{
    struct fileinfo *fcb = &G__.fileinfo[fd];
    off_t t1 = fcb->row_ptr[row];
    off_t t2 = fcb->row_ptr[row + 1];
    ssize_t readamount = t2 - t1;
    unsigned char *cmp;
    int n;

    if (lseek(fd, t1, SEEK_SET) < 0)
	return -1;

    cmp = G__alloca(readamount);

    if (read(fd, cmp, readamount) != readamount) {
	G__freea(cmp);
	return -1;
    }

    /* Now decompress the row */
    if (fcb->cellhd.compressed > 0) {
	/* one byte is nbyte count */
	n = *nbytes = *cmp++;
	readamount--;
    }
    else
	/* pre 3.0 compression */
	n = *nbytes = fcb->nbytes;

    if (fcb->cellhd.compressed < 0 || readamount < n * fcb->cellhd.cols) {
	if (fcb->cellhd.compressed == 2)
	    G_zlib_expand(cmp, readamount, data_buf, n * fcb->cellhd.cols);
	else
	    rle_decompress(data_buf, cmp, n, readamount);
    }
    else
	memcpy(data_buf, cmp, readamount);

    G__freea(cmp);

    return 0;
}

/*--------------------------------------------------------------------------*/

static int read_data_uncompressed(int fd, int row, unsigned char *data_buf,
				  int *nbytes)
{
    struct fileinfo *fcb = &G__.fileinfo[fd];
    ssize_t bufsize = fcb->cellhd.cols * fcb->nbytes;

    *nbytes = fcb->nbytes;

    if (lseek(fd, (off_t) row * bufsize, SEEK_SET) == -1)
	return -1;

    if (read(fd, data_buf, bufsize) != bufsize)
	return -1;

    return 0;
}

/*--------------------------------------------------------------------------*/

#ifdef HAVE_GDAL
static int read_data_gdal(int fd, int row, unsigned char *data_buf, int *nbytes)
{
    struct fileinfo *fcb = &G__.fileinfo[fd];
    unsigned char *buf;
    CPLErr err;

    *nbytes = fcb->nbytes;

    if (fcb->gdal->vflip)
	row = fcb->cellhd.rows - 1 - row;

    buf = fcb->gdal->hflip
	? G__alloca(fcb->cellhd.cols * fcb->cur_nbytes)
	: data_buf;

    err = G_gdal_raster_IO(
	fcb->gdal->band, GF_Read, 0, row, fcb->cellhd.cols, 1, buf,
	fcb->cellhd.cols, 1, fcb->gdal->type, 0, 0);

    if (fcb->gdal->hflip) {
	int i;

	for (i = 0; i < fcb->cellhd.cols; i++)
	    memcpy(data_buf + i * fcb->cur_nbytes,
		   buf + (fcb->cellhd.cols - 1 - i) * fcb->cur_nbytes,
		   fcb->cur_nbytes);
	G__freea(buf);
    }

    return err == CE_None ? 0 : -1;
}
#endif

/*--------------------------------------------------------------------------*/

/* Actually read a row of data in */

static int read_data(int fd, int row, unsigned char *data_buf, int *nbytes)
{
    struct fileinfo *fcb = &G__.fileinfo[fd];

#ifdef HAVE_GDAL
    if (fcb->gdal)
	return read_data_gdal(fd, row, data_buf, nbytes);
#endif

    if (!fcb->cellhd.compressed)
	return read_data_uncompressed(fd, row, data_buf, nbytes);

    /* map is in compressed form */

    if (fcb->map_type == CELL_TYPE)
	return read_data_compressed(fd, row, data_buf, nbytes);
    else
	return read_data_fp_compressed(fd, row, data_buf, nbytes);
}

/*--------------------------------------------------------------------------*/

/* copy cell file data to user buffer translated by window column mapping */

static void cell_values_int(int fd, const unsigned char *data,
			    const COLUMN_MAPPING * cmap, int nbytes,
			    void *cell, int n)
{
    CELL *c = cell;
    COLUMN_MAPPING cmapold = 0;
    int big = (size_t) nbytes >= sizeof(CELL);
    int i;

    for (i = 0; i < n; i++) {
	const unsigned char *d;
	int neg;
	CELL v;
	int j;

	if (!cmap[i]) {
	    c[i] = 0;
	    continue;
	}

	if (cmap[i] == cmapold) {
	    c[i] = c[i - 1];
	    continue;
	}

	d = data + (cmap[i] - 1) * nbytes;

	if (big && (*d & 0x80)) {
	    neg = 1;
	    v = *d++ & 0x7f;
	}
	else {
	    neg = 0;
	    v = *d++;
	}

	for (j = 1; j < nbytes; j++)
	    v = (v << 8) + *d++;

	c[i] = neg ? -v : v;

	cmapold = cmap[i];
    }
}

/*--------------------------------------------------------------------------*/

static void cell_values_float(int fd, const unsigned char *data,
			      const COLUMN_MAPPING * cmap, int nbytes,
			      void *cell, int n)
{
    struct fileinfo *fcb = &G__.fileinfo[fd];
    FCELL *c = cell;
    COLUMN_MAPPING cmapold = 0;
    XDR *xdrs = &fcb->xdrstream;
    int i;

    /* xdr stream is initialized to read from */
    /* fcb->data in 'opencell.c' */
    xdr_setpos(xdrs, 0);

    for (i = 0; i < n; i++) {
	if (!cmap[i]) {
	    c[i] = 0;
	    continue;
	}

	if (cmap[i] == cmapold) {
	    c[i] = c[i - 1];
	    continue;
	}

	if (cmap[i] < cmapold) {
	    xdr_setpos(xdrs, 0);
	    cmapold = 0;
	}

	while (cmapold++ != cmap[i])	/* skip */
	    if (!xdr_float(xdrs, &c[i]))
		G_fatal_error(_("cell_values_float: xdr_float failed for index %d"),
			      i);

	cmapold--;
    }
}

/*--------------------------------------------------------------------------*/

static void cell_values_double(int fd, const unsigned char *data,
			       const COLUMN_MAPPING * cmap, int nbytes,
			       void *cell, int n)
{
    struct fileinfo *fcb = &G__.fileinfo[fd];
    DCELL *c = cell;
    COLUMN_MAPPING cmapold = 0;
    XDR *xdrs = &fcb->xdrstream;
    int i;

    /* xdr stream is initialized to read from */
    /* fcb->data in 'opencell.c' */
    xdr_setpos(xdrs, 0);

    for (i = 0; i < n; i++) {
	if (!cmap[i]) {
	    c[i] = 0;
	    continue;
	}

	if (cmap[i] == cmapold) {
	    c[i] = c[i - 1];
	    continue;
	}

	if (cmap[i] < cmapold) {
	    xdr_setpos(xdrs, 0);
	    cmapold = 0;
	}

	while (cmapold++ != cmap[i])	/* skip */
	    if (!xdr_double(xdrs, &c[i]))
		G_fatal_error(_("cell_values_double: xdr_double failed for index %d"),
			      i);

	cmapold--;
    }
}

/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/

#ifdef HAVE_GDAL

/*--------------------------------------------------------------------------*/

static void gdal_values_int(int fd, const unsigned char *data,
			    const COLUMN_MAPPING *cmap, int nbytes,
			    CELL *cell, int n)
{
    struct fileinfo *fcb = &G__.fileinfo[fd];
    const unsigned char *d;
    COLUMN_MAPPING cmapold = 0;
    int i;

    for (i = 0; i < n; i++) {
	if (!cmap[i]) {
	    cell[i] = 0;
	    continue;
	}

	if (cmap[i] == cmapold) {
	    cell[i] = cell[i-1];
	    continue;
	}

	d = data + (cmap[i] - 1) * nbytes;

	switch (fcb->gdal->type) {
	case GDT_Byte:	    cell[i] = *(GByte   *)d;	break;
	case GDT_Int16:	    cell[i] = *(GInt16  *)d;	break;
	case GDT_UInt16:    cell[i] = *(GUInt16 *)d;	break;
	case GDT_Int32:	    cell[i] = *(GInt32  *)d;	break;
	case GDT_UInt32:    cell[i] = *(GUInt32 *)d;	break;
	default:
	    /* shouldn't happen */
	    G_set_c_null_value(&cell[i], 1);
	    break;
	}

	cmapold = cmap[i];
    }
}

/*--------------------------------------------------------------------------*/

static void gdal_values_float(int fd, const float *data,
			      const COLUMN_MAPPING *cmap, int nbytes,
			      FCELL *cell, int n)
{
    COLUMN_MAPPING cmapold = 0;
    int i;

    for (i = 0; i < n; i++) {
	if (!cmap[i]) {
	    cell[i] = 0;
	    continue;
	}

	if (cmap[i] == cmapold) {
	    cell[i] = cell[i-1];
	    continue;
	}

	cell[i] = data[cmap[i] - 1];

	cmapold = cmap[i];
    }
}

/*--------------------------------------------------------------------------*/

static void gdal_values_double(int fd, const double *data,
			       const COLUMN_MAPPING *cmap, int nbytes,
			       DCELL *cell, int n)
{
    COLUMN_MAPPING cmapold = 0;
    int i;

    for (i = 0; i < n; i++) {
	if (!cmap[i]) {
	    cell[i] = 0;
	    continue;
	}

	if (cmap[i] == cmapold) {
	    cell[i] = cell[i-1];
	    continue;
	}

	cell[i] = data[cmap[i] - 1];

	cmapold = cmap[i];
    }
}

/*--------------------------------------------------------------------------*/

#endif

/*--------------------------------------------------------------------------*/

/* transfer_to_cell_XY takes bytes from fcb->data, converts these bytes with
   the appropriate procedure (e.g. XDR or byte reordering) into type X 
   values which are put into array work_buf.  
   finally the values in work_buf are converted into 
   type Y and put into 'cell'.
   if type X == type Y the intermediate step of storing the values in 
   work_buf might be ommited. check the appropriate function for XY to
   determine the procedure of conversion. 
 */

/*--------------------------------------------------------------------------*/

static void transfer_to_cell_XX(int fd, void *cell)
{
    static void (*cell_values_type[3]) () = {
    cell_values_int, cell_values_float, cell_values_double};
#ifdef HAVE_GDAL
    static void (*gdal_values_type[3]) () = {
    gdal_values_int, gdal_values_float, gdal_values_double};
#endif
    struct fileinfo *fcb = &G__.fileinfo[fd];

#ifdef HAVE_GDAL
    if (fcb->gdal)
    (gdal_values_type[fcb->map_type]) (fd, fcb->data, fcb->col_map,
				       fcb->cur_nbytes, cell,
				       G__.window.cols);
    else
#endif
    (cell_values_type[fcb->map_type]) (fd, fcb->data, fcb->col_map,
				       fcb->cur_nbytes, cell,
				       G__.window.cols);
}

/*--------------------------------------------------------------------------*/

static void transfer_to_cell_fi(int fd, void *cell)
{
    struct fileinfo *fcb = &G__.fileinfo[fd];
    FCELL *work_buf = G__alloca(G__.window.cols * sizeof(FCELL));
    int i;

    transfer_to_cell_XX(fd, work_buf);

    for (i = 0; i < G__.window.cols; i++)
	((CELL *) cell)[i] = (fcb->col_map[i] == 0)
	    ? 0
	    : G_quant_get_cell_value(&fcb->quant, work_buf[i]);

    G__freea(work_buf);
}

static void transfer_to_cell_di(int fd, void *cell)
{
    struct fileinfo *fcb = &G__.fileinfo[fd];
    DCELL *work_buf = G__alloca(G__.window.cols * sizeof(DCELL));
    int i;

    transfer_to_cell_XX(fd, work_buf);

    for (i = 0; i < G__.window.cols; i++)
	((CELL *) cell)[i] = (fcb->col_map[i] == 0)
	    ? 0
	    : G_quant_get_cell_value(&fcb->quant, work_buf[i]);

    G__freea(work_buf);
}

/*--------------------------------------------------------------------------*/

static void transfer_to_cell_if(int fd, void *cell)
{
    CELL *work_buf = G__alloca(G__.window.cols * sizeof(CELL));
    int i;

    transfer_to_cell_XX(fd, work_buf);

    for (i = 0; i < G__.window.cols; i++)
	((FCELL *) cell)[i] = work_buf[i];

    G__freea(work_buf);
}

static void transfer_to_cell_df(int fd, void *cell)
{
    DCELL *work_buf = G__alloca(G__.window.cols * sizeof(DCELL));
    int i;

    transfer_to_cell_XX(fd, work_buf);

    for (i = 0; i < G__.window.cols; i++)
	((FCELL *) cell)[i] = work_buf[i];

    G__freea(work_buf);
}

/*--------------------------------------------------------------------------*/

static void transfer_to_cell_id(int fd, void *cell)
{
    CELL *work_buf = G__alloca(G__.window.cols * sizeof(CELL));
    int i;

    transfer_to_cell_XX(fd, work_buf);

    for (i = 0; i < G__.window.cols; i++)
	((DCELL *) cell)[i] = work_buf[i];

    G__freea(work_buf);
}

static void transfer_to_cell_fd(int fd, void *cell)
{
    FCELL *work_buf = G__alloca(G__.window.cols * sizeof(FCELL));
    int i;

    transfer_to_cell_XX(fd, work_buf);

    for (i = 0; i < G__.window.cols; i++)
	((DCELL *) cell)[i] = work_buf[i];

    G__freea(work_buf);
}

/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/*
 *   works for all map types and doesn't consider
 *   null row corresponding to the requested row 
 */
static int get_map_row_nomask(int fd, void *rast, int row,
			      RASTER_MAP_TYPE data_type)
{
    static void (*transfer_to_cell_FtypeOtype[3][3])() = {
	{transfer_to_cell_XX, transfer_to_cell_if, transfer_to_cell_id},
	{transfer_to_cell_fi, transfer_to_cell_XX, transfer_to_cell_fd},
	{transfer_to_cell_di, transfer_to_cell_df, transfer_to_cell_XX}
    };
    struct fileinfo *fcb = &G__.fileinfo[fd];
    int r;
    int rowStatus;

    rowStatus = compute_window_row(fd, row, &r);

    if (rowStatus <= 0) {
	fcb->cur_row = -1;
	G_zero_raster_buf(rast, data_type);
	return rowStatus;
    }

    /* read cell file row if not in memory */
    if (r != fcb->cur_row) {
	fcb->cur_row = r;

	if (read_data(fd, fcb->cur_row, fcb->data, &fcb->cur_nbytes) < 0) {
	    G_zero_raster_buf(rast, data_type);

	    if (!fcb->io_error) {
		if (fcb->cellhd.compressed)
		    G_warning(_("Error reading compressed map <%s@%s>, row %d"),
			      fcb->name, fcb->mapset, r);
		else
		    G_warning(_("Error reading map <%s@%s>, row %d"),
			      fcb->name, fcb->mapset, r);

		fcb->io_error = 1;
	    }
	    return -1;
	}
    }

    (transfer_to_cell_FtypeOtype[fcb->map_type][data_type]) (fd, rast);

    return 1;
}

/*--------------------------------------------------------------------------*/

static int get_map_row_no_reclass(int fd, void *rast, int row,
				  RASTER_MAP_TYPE data_type, int null_is_zero,
				  int with_mask)
{
    int stat;

    stat = get_map_row_nomask(fd, rast, row, data_type);
    if (stat < 0)
	return stat;

    stat = embed_nulls(fd, rast, row, data_type, null_is_zero, with_mask);
    if (stat < 0)
	return stat;

    return 1;
}

/*--------------------------------------------------------------------------*/

static int get_map_row(int fd, void *rast, int row, RASTER_MAP_TYPE data_type,
		       int null_is_zero, int with_mask)
{
    struct fileinfo *fcb = &G__.fileinfo[fd];
    int size = G_raster_size(data_type);
    CELL *temp_buf = NULL;
    void *buf;
    int type;
    int stat;
    int i;

    if (fcb->reclass_flag && data_type != CELL_TYPE) {
	temp_buf = G__alloca(G__.window.cols * sizeof(CELL));
	buf = temp_buf;
	type = CELL_TYPE;
    }
    else {
	buf = rast;
	type = data_type;
    }

    stat = get_map_row_no_reclass(fd, buf, row, type, null_is_zero, with_mask);
    if (stat < 0) {
	if (temp_buf)
	    G__freea(temp_buf);
	return stat;
    }

    if (!fcb->reclass_flag)
	return 1;

    /* if the map is reclass table, get and
       reclass CELL row and copy results to needed type  */

    do_reclass_int(fd, buf, null_is_zero);

    if (data_type == CELL_TYPE)
	return 1;

    for (i = 0; i < G__.window.cols; i++) {
	G_set_raster_value_c(rast, temp_buf[i], data_type);
	rast = G_incr_void_ptr(rast, size);
    }

    G__freea(temp_buf);

    return 1;
}

/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/

/*!
 * \brief Read raster row without masking (this routine is deprecated)
 *
 * This routine reads the specified <em>row</em> from the raster map
 * open on file descriptor <em>fd</em> into the <em>buf</em> buffer
 * like G_get_map_row() does. The difference is that masking is
 * suppressed. If the user has a mask set, G_get_map_row() will apply
 * the mask but G_get_map_row_nomask() will ignore it. This routine
 * prints a diagnostic message and returns -1 if there is an error
 * reading the raster map. Otherwise a nonnegative value is returned.
 *
 * <b>Note.</b> Ignoring the mask is not generally acceptable. Users
 * expect the mask to be applied. However, in some cases ignoring the
 * mask is justified. For example, the GRASS modules
 * <i>r.describe</i>, which reads the raster map directly to report
 * all data values in a raster map, and <i>r.slope.aspect</i>, which
 * produces slope and aspect from elevation, ignore both the mask and
 * the region. However, the number of GRASS modules which do this
 * should be minimal. See Mask for more information about the mask.
 *
 * <b>This routine is deprecated! Use G_get_raster_row_nomask()
 * instead.</b>
 *
 * \param fd file descriptor for the opened raster map
 * \param buf buffer for the row to be placed into
 * \param row data row desired
 *
 * \return 1 on success
 * \return 0 row requested not within window
 * \return -1 on error
 */

int G_get_map_row_nomask(int fd, CELL * buf, int row)
{
    return get_map_row(fd, buf, row, CELL_TYPE, 1, 0);
}

/*!
 * \brief Read raster row without masking
 *
 *  Same as G_get_raster_row() except no masking occurs.
 *
 * \param fd file descriptor for the opened raster map
 * \param buf buffer for the row to be placed into
 * \param row data row desired
 * \param data_type data type
 *
 * \return 1 on success
 * \return 0 row requested not within window
 * \return -1 on error
 */

int G_get_raster_row_nomask(int fd, void *buf, int row,
			    RASTER_MAP_TYPE data_type)
{
    return get_map_row(fd, buf, row, data_type, 0, 0);
}

/*!
 * \brief Read raster row without masking (CELL type)
 *
 *  Same as G_get_c_raster_row() except no masking occurs.
 *
 * \param fd file descriptor for the opened raster map
 * \param buf buffer for the row to be placed into
 * \param row data row desired
 * \param data_type data type
 *
 * \return 1 on success
 * \return 0 row requested not within window
 * \return -1 on error
 */

int G_get_c_raster_row_nomask(int fd, CELL * buf, int row)
{
    return G_get_raster_row_nomask(fd, buf, row, CELL_TYPE);
}

/*!
 * \brief Read raster row without masking (FCELL type)
 *
 *  Same as G_get_f_raster_row() except no masking occurs.
 *
 * \param fd file descriptor for the opened raster map
 * \param buf buffer for the row to be placed into
 * \param row data row desired
 * \param data_type data type
 *
 * \return 1 on success
 * \return 0 row requested not within window
 * \return -1 on error
 */

int G_get_f_raster_row_nomask(int fd, FCELL * buf, int row)
{
    return G_get_raster_row_nomask(fd, buf, row, FCELL_TYPE);
}

/*!
 * \brief Read raster row without masking (DCELL type)
 *
 *  Same as G_get_d_raster_row() except no masking occurs.
 *
 * \param fd file descriptor for the opened raster map
 * \param buf buffer for the row to be placed into
 * \param row data row desired
 * \param data_type data type
 *
 * \return 1 on success
 * \return 0 row requested not within window
 * \return -1 on error
 */

int G_get_d_raster_row_nomask(int fd, DCELL * buf, int row)
{
    return G_get_raster_row_nomask(fd, buf, row, DCELL_TYPE);
}

/*--------------------------------------------------------------------------*/

/*!
 * \brief Get raster row (this routine is deprecated!)
 *
 * If the map is floating-point, quantize the floating-point values to
 * integer using the quantization rules established for the map when
 * the map was opened for reading (this quantization is read from
 * cell_misc/name/f_quant file, but can be reset after opening raster
 * map by G_set_quant_rules()). NULL values are converted to zeros.
 *
 * <b>This routine is deprecated! Use G_get_raster_row() instead.</b>
 *
 * \param fd file descriptor for the opened raster map
 * \param buf buffer for the row to be placed into
 * \param row data row desired
 *
 * \return 1 on success
 * \return 0 row requested not within window
 * \return -1 on error
 */

int G_get_map_row(int fd, CELL * buf, int row)
{
    return get_map_row(fd, buf, row, CELL_TYPE, 1, 1);
}

/*!
 * \brief Get raster row
 *
 * If <em>data_type</em> is
 *  - CELL_TYPE, calls G_get_c_raster_row()
 *  - FCELL_TYPE, calls G_get_f_raster_row()
 *  - DCELL_TYPE, calls G_get_d_raster_row()
 *
 *   Reads appropriate information into the buffer <em>buf</em> associated 
 *   with the requested row <em>row</em>. <em>buf</em> is associated with the
 *   current window.
 *
 *   Note, that the type of the data in <em>buf</em> (say X) is independent of 
 *   the type of the data in the file described by <em>fd</em> (say Y).
 *
 *    - Step 1:  Read appropriate raw map data into a intermediate buffer.
 *    - Step 2:  Convert the data into a CPU readable format, and subsequently
 *            resample the data. the data is stored in a second intermediate 
 *            buffer (the type of the data in this buffer is Y).
 *    - Step 3:  Convert this type Y data into type X data and store it in
 *            buffer "buf". Conversion is performed in functions 
 *            "transfer_to_cell_XY". (For details of the conversion between
 *            two particular types check the functions).
 *    - Step 4:  read or simmulate null value row and zero out cells corresponding 
 *            to null value cells. The masked out cells are set to null when the
 *            mask exists. (the MASK is taken care of by null values
 *            (if the null file doesn't exist for this map, then the null row
 *            is simulated by assuming that all zero are nulls *** in case
 *            of G_get_raster_row() and assuming that all data is valid 
 *            in case of G_get_f/d_raster_row(). In case of deprecated function
 *            G_get_map_row() all nulls are converted to zeros (so there are
 *            no embedded nulls at all). Also all masked out cells become zeros.
 *
 * \param fd file descriptor for the opened raster map
 * \param buf buffer for the row to be placed into
 * \param row data row desired
 * \param data_type data type
 *
 * \return 1 on success
 * \return 0 row requested not within window
 * \return -1 on error
 */

int G_get_raster_row(int fd, void *buf, int row, RASTER_MAP_TYPE data_type)
{
    return get_map_row(fd, buf, row, data_type, 0, 1);
}

/*!
 * \brief Get raster row (CELL type)
 *
 * Reads a row of raster data and leaves the NULL values intact. (As
 * opposed to the deprecated function G_get_map_row() which
 * converts NULL values to zero.) 
 *
 * <b>NOTE.</b> When the raster map is old and null file doesn't
 * exist, it is assumed that all 0-cells are no-data. When map is
 * floating point, uses quant rules set explicitly by
 * G_set_quant_rules() or stored in map's quant file to convert floats
 * to integers.
 *
 * \param fd file descriptor for the opened raster map
 * \param buf buffer for the row to be placed into
 * \param row data row desired
 *
 * \return 1 on success
 * \return 0 row requested not within window
 * \return -1 on error
 */

int G_get_c_raster_row(int fd, CELL * buf, int row)
{
    return G_get_raster_row(fd, buf, row, CELL_TYPE);
}

/*!
 * \brief Get raster row (FCELL type)
 *
 * Read a row from the raster map open on <em>fd</em> into the
 * <tt>float</tt> array <em>fcell</em> performing type conversions as
 * necessary based on the actual storage type of the map. Masking,
 * resampling into the current region.  NULL-values are always
 * embedded in <tt>fcell</tt> (<em>never converted to a value</em>).
 *
 * \param fd file descriptor for the opened raster map
 * \param buf buffer for the row to be placed into
 * \param row data row desired
 *
 * \return 1 on success
 * \return 0 row requested not within window
 * \return -1 on error
 */

int G_get_f_raster_row(int fd, FCELL * buf, int row)
{
    return G_get_raster_row(fd, buf, row, FCELL_TYPE);
}

/*!
 * \brief Get raster row (DCELL type)
 *
 * Same as G_get_f_raster_row() except that the array <em>dcell</em>
 * is <tt>double</tt>.
 *
 * \param fd file descriptor for the opened raster map
 * \param buf buffer for the row to be placed into
 * \param row data row desired
 *
 * \return 1 on success
 * \return 0 row requested not within window
 * \return -1 on error
 */

int G_get_d_raster_row(int fd, DCELL * buf, int row)
{
    return G_get_raster_row(fd, buf, row, DCELL_TYPE);
}

/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/

static int open_null_read(int fd)
{
    struct fileinfo *fcb = &G__.fileinfo[fd];
    const char *name, *mapset, *dummy;
    int null_fd;

    if (fcb->null_file_exists == 0)
	return -1;

    if (fcb->reclass_flag) {
	name = fcb->reclass.name;
	mapset = fcb->reclass.mapset;
    }
    else {
	name = fcb->name;
	mapset = fcb->mapset;
    }

    dummy = G_find_file2_misc("cell_misc", NULL_FILE, name, mapset);

    if (!dummy) {
	/* G_warning("unable to find [%s]",path); */
	fcb->null_file_exists = 0;
	return -1;
    }

    null_fd = G_open_old_misc("cell_misc", NULL_FILE, name, mapset);
    if (null_fd < 0)
	return -1;

    fcb->null_file_exists = 1;

    return null_fd;
}

static int read_null_bits(int null_fd, unsigned char *flags, int row,
			  int cols, int fd)
{
    off_t offset;
    ssize_t size;
    int R;

    if (compute_window_row(fd, row, &R) <= 0) {
	G__init_null_bits(flags, cols);
	return 1;
    }

    if (null_fd < 0)
	return -1;

    size = G__null_bitstream_size(cols);
    offset = (off_t) size *R;

    if (lseek(null_fd, offset, SEEK_SET) < 0) {
	G_warning(_("Error reading null row %d"), R);
	return -1;
    }

    if (read(null_fd, flags, size) != size) {
	G_warning(_("Error reading null row %d"), R);
	return -1;
    }

    return 1;
}

static void get_null_value_row_nomask(int fd, char *flags, int row)
{
    struct fileinfo *fcb = &G__.fileinfo[fd];
    int i, j, null_fd;

    if (row > G__.window.rows || row < 0) {
	G_warning(_("Reading raster map <%s@%s> request for row %d is outside region"),
		  fcb->name, fcb->mapset, row);
    }

    if ((fcb->min_null_row > row) ||
	(fcb->min_null_row + NULL_ROWS_INMEM - 1 < row))
	/* the null row row is not in memory */
    {
	unsigned char *null_work_buf = G__alloca(
	    G__null_bitstream_size(fcb->cellhd.cols));

	/* read in NULL_ROWS_INMEM rows from null file 
	   so that the requested row is between fcb->min_null_row
	   and fcb->min_null_row + NULL_ROWS_INMEM */

	fcb->min_null_row = (row / NULL_ROWS_INMEM) * NULL_ROWS_INMEM;

	null_fd = open_null_read(fd);

	for (i = 0; i < NULL_ROWS_INMEM; i++) {
	    /* G__.window.rows doesn't have to be a multiple of NULL_ROWS_INMEM */
	    if (i + fcb->min_null_row >= G__.window.rows)
		break;

	    if (read_null_bits(null_fd, null_work_buf,
			       i + fcb->min_null_row, fcb->cellhd.cols,
			       fd) < 0) {
		if (fcb->map_type == CELL_TYPE) {
		    /* If can't read null row, assume  that all map 0's are nulls */
		    CELL *mask_buf = G__alloca(G__.window.cols * sizeof(CELL));

		    get_map_row_nomask(fd, mask_buf, i + fcb->min_null_row,
				       CELL_TYPE);
		    for (j = 0; j < G__.window.cols; j++)
			flags[j] = (mask_buf[j] == 0);

		    G__freea(mask_buf);
		}
		else {		/* fp map */

		    /* if can't read null row, assume  that all data is valid */
		    G_zero(flags, sizeof(char) * G__.window.cols);
		    /* the flags row is ready now */
		}
	    }			/*if no null file */
	    else {
		/* copy null row to flags row translated by window column mapping */
		/* the fcb->NULL_ROWS[row-fcb->min_null_row] has G__.window.cols bits, */
		/* the null_work_buf has size fcb->cellhd.cols */
		for (j = 0; j < G__.window.cols; j++) {
		    if (!fcb->col_map[j])
			flags[j] = 1;
		    else
			flags[j] = G__check_null_bit(null_work_buf,
						     fcb->col_map[j] - 1,
						     fcb->cellhd.cols);
		}
	    }
	    /* remember the null row for i for the future reference */

	    /*bf-We should take of the size - or we get 
	       zeros running on their own after flags convertions -A.Sh. */
	    fcb->NULL_ROWS[i] = G_realloc(fcb->NULL_ROWS[i],
					  G__null_bitstream_size(G__.window.
								 cols) + 1);
	    if (fcb->NULL_ROWS[i] == NULL)
		G_fatal_error("get_null_value_row_nomask: %s",
			      _("Unable to realloc buffer"));

	    G__convert_01_flags(flags, fcb->NULL_ROWS[i], G__.window.cols);

	}			/* for loop */

	if (null_fd > 0)
	    close(null_fd);

	G__freea(null_work_buf);
    }				/* row is not in memory */

    /* copy null file data translated by column mapping to user null row */
    /* the user requested flags row is of size G__.window.cols */
    G__convert_flags_01(flags, fcb->NULL_ROWS[row - fcb->min_null_row],
			G__.window.cols);
}

/*--------------------------------------------------------------------------*/

#ifdef HAVE_GDAL

static void get_null_value_row_gdal(int fd, char *flags, int row)
{
    struct fileinfo *fcb = &G__.fileinfo[fd];
    DCELL *tmp_buf = G_allocate_d_raster_buf();
    int i;

    if (get_map_row_nomask(fd, tmp_buf, row, DCELL_TYPE) <= 0) {
	memset(flags, 1, G__.window.cols);
	G_free(tmp_buf);
	return;
    }

    for (i = 0; i < G__.window.cols; i++)
	/* note: using == won't work if the null value is NaN */
	flags[i] = memcmp(&tmp_buf[i], &fcb->gdal->null_val, sizeof(DCELL)) == 0;

    G_free(tmp_buf);
}

#endif

/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/

static void embed_mask(char *flags, int row)
{
    CELL *mask_buf = G__alloca(G__.window.cols * sizeof(CELL));
    int i;

    if (G__.auto_mask <= 0)
	return;

    if (get_map_row_nomask(G__.mask_fd, mask_buf, row, CELL_TYPE) < 0) {
	G__freea(mask_buf);
	return;
    }

    if (G__.fileinfo[G__.mask_fd].reclass_flag)
	do_reclass_int(G__.mask_fd, mask_buf, 1);

    for (i = 0; i < G__.window.cols; i++)
	if (mask_buf[i] == 0)
	    flags[i] = 1;

    G__freea(mask_buf);
}

static void get_null_value_row(int fd, char *flags, int row, int with_mask)
{
#ifdef HAVE_GDAL
    struct fileinfo *fcb = &G__.fileinfo[fd];
    if (fcb->gdal)
	get_null_value_row_gdal(fd, flags, row);
    else
#endif
    get_null_value_row_nomask(fd, flags, row);

    if (with_mask)
	embed_mask(flags, row);
}

static int embed_nulls(int fd, void *buf, int row, RASTER_MAP_TYPE map_type,
		       int null_is_zero, int with_mask)
{
    struct fileinfo *fcb = &G__.fileinfo[fd];
    char *null_buf;
    int i;

    /* this is because without null file the nulls can be only due to 0's
       in data row or mask */
    if (null_is_zero && !fcb->null_file_exists
	&& (G__.auto_mask <= 0 || !with_mask))
	return 1;

    null_buf = G__alloca(G__.window.cols);

    get_null_value_row(fd, null_buf, row, with_mask);

    for (i = 0; i < G__.window.cols; i++) {
	/* also check for nulls which might be already embedded by quant
	   rules in case of fp map. */
	if (null_buf[i] || G_is_null_value(buf, map_type)) {
	    /* G__set_[f/d]_null_value() sets it to 0 is the embedded mode
	       is not set and calls G_set_[f/d]_null_value() otherwise */
	    G__set_null_value(buf, 1, null_is_zero, map_type);
	}
	buf = G_incr_void_ptr(buf, G_raster_size(map_type));
    }

    G__freea(null_buf);

    return 1;
}

/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/

/*!
   \brief Read or simmulate null value row

   Read or simmulate null value row and set the cells corresponding 
   to null value to 1. The masked out cells are set to null when the
   mask exists. (the MASK is taken care of by null values
   (if the null file doesn't exist for this map, then the null row
   is simulated by assuming that all zeros in raster map are nulls.
   Also all masked out cells become nulls.

   \param fd file descriptor for the opened map
   \param buf buffer for the row to be placed into
   \param row data row desired

   \return 1
 */
int G_get_null_value_row(int fd, char *flags, int row)
{
    get_null_value_row(fd, flags, row, 1);

    return 1;
}
