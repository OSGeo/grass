/*!
   \file lib/raster/get_row.c

   \brief Raster library - Get raster row

   (C) 2003-2009 by the GRASS Development Team

   This program is free software under the GNU General Public License
   (>=v2).  Read the file COPYING that comes with GRASS for details.

   \author Original author CERL
 */

#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>

#include <grass/config.h>
#include <grass/raster.h>
#include <grass/glocale.h>

#include "R.h"

static void embed_nulls(int, void *, int, RASTER_MAP_TYPE, int, int);

static int compute_window_row(int fd, int row, int *cellRow)
{
    struct fileinfo *fcb = &R__.fileinfo[fd];
    double f;
    int r;

    /* check for row in window */
    if (row < 0 || row >= R__.rd_window.rows) {
	G_fatal_error(_("Reading raster map <%s@%s> request for row %d is outside region"),
		      fcb->name, fcb->mapset, row);
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

static void do_reclass_int(int fd, void *cell, int null_is_zero)
{
    struct fileinfo *fcb = &R__.fileinfo[fd];
    CELL *c = cell;
    CELL *reclass_table = fcb->reclass.table;
    CELL min = fcb->reclass.min;
    CELL max = fcb->reclass.max;
    int i;

    for (i = 0; i < R__.rd_window.cols; i++) {
	if (Rast_is_c_null_value(&c[i])) {
	    if (null_is_zero)
		c[i] = 0;
	    continue;
	}

	if (c[i] < min || c[i] > max) {
	    if (null_is_zero)
		c[i] = 0;
	    else
		Rast_set_c_null_value(&c[i], 1);
	    continue;
	}

	c[i] = reclass_table[c[i] - min];

	if (null_is_zero && Rast_is_c_null_value(&c[i]))
	    c[i] = 0;
    }
}

static void read_data_fp_compressed(int fd, int row, unsigned char *data_buf,
				    int *nbytes)
{
    struct fileinfo *fcb = &R__.fileinfo[fd];
    off_t t1 = fcb->row_ptr[row];
    off_t t2 = fcb->row_ptr[row + 1];
    size_t readamount = t2 - t1;
    size_t bufsize = fcb->cellhd.cols * fcb->nbytes;
    int ret;

    if (lseek(fcb->data_fd, t1, SEEK_SET) < 0)
	G_fatal_error(_("Error seeking fp raster data file for row %d of <%s>: %s"),
		      row, fcb->name, strerror(errno));

    *nbytes = fcb->nbytes;

    ret = G_read_compressed(fcb->data_fd, readamount, data_buf,
			    bufsize, fcb->cellhd.compressed);
    if (ret <= 0)
	G_fatal_error(_("Error uncompressing fp raster data for row %d of <%s>: error code %d"),
		      row, fcb->name, ret);
}

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

static void read_data_compressed(int fd, int row, unsigned char *data_buf,
				 int *nbytes)
{
    struct fileinfo *fcb = &R__.fileinfo[fd];
    off_t t1 = fcb->row_ptr[row];
    off_t t2 = fcb->row_ptr[row + 1];
    ssize_t readamount = t2 - t1;
    size_t bufsize;
    unsigned char *cmp, *cmp2;
    int n;

    if (lseek(fcb->data_fd, t1, SEEK_SET) < 0)
	G_fatal_error(_("Error seeking raster data file for row %d of <%s>: %s"),
		      row, fcb->name, strerror(errno));

    cmp = G_malloc(readamount);

    if (read(fcb->data_fd, cmp, readamount) != readamount) {
	G_free(cmp);
	G_fatal_error(_("Error reading raster data for row %d of <%s>: %s"),
		      row, fcb->name, strerror(errno));
    }

    /* save cmp for free below */
    cmp2 = cmp;

    /* Now decompress the row */
    if (fcb->cellhd.compressed > 0) {
	/* one byte is nbyte count */
	n = *nbytes = *cmp++;
	readamount--;
    }
    else
	/* pre 3.0 compression */
	n = *nbytes = fcb->nbytes;

    bufsize = n * fcb->cellhd.cols;
    if (fcb->cellhd.compressed < 0 || readamount < bufsize) {
	if (fcb->cellhd.compressed == 1)
	    rle_decompress(data_buf, cmp, n, readamount);
	else {
	    if (G_expand(cmp, readamount, data_buf, bufsize,
		     fcb->cellhd.compressed) != bufsize)
	    G_fatal_error(_("Error uncompressing raster data for row %d of <%s>"),
			  row, fcb->name);
	}
    }
    else
	memcpy(data_buf, cmp, readamount);

    G_free(cmp2);
}

static void read_data_uncompressed(int fd, int row, unsigned char *data_buf,
				   int *nbytes)
{
    struct fileinfo *fcb = &R__.fileinfo[fd];
    ssize_t bufsize = fcb->cellhd.cols * fcb->nbytes;

    *nbytes = fcb->nbytes;

    if (lseek(fcb->data_fd, (off_t) row * bufsize, SEEK_SET) == -1)
	G_fatal_error(_("Error reading raster data for row %d of <%s>"),
		      row, fcb->name);

    if (read(fcb->data_fd, data_buf, bufsize) != bufsize)
	G_fatal_error(_("Error reading raster data for row %d of <%s>"),
		      row, fcb->name);
}

#ifdef HAVE_GDAL
static void read_data_gdal(int fd, int row, unsigned char *data_buf,
			   int *nbytes)
{
    struct fileinfo *fcb = &R__.fileinfo[fd];
    unsigned char *buf;
    CPLErr err;

    *nbytes = fcb->nbytes;

    if (fcb->gdal->vflip)
	row = fcb->cellhd.rows - 1 - row;

    buf = fcb->gdal->hflip ? G_malloc(fcb->cellhd.cols * fcb->cur_nbytes)
	: data_buf;

    err =
	Rast_gdal_raster_IO(fcb->gdal->band, GF_Read, 0, row,
			    fcb->cellhd.cols, 1, buf, fcb->cellhd.cols, 1,
			    fcb->gdal->type, 0, 0);

    if (fcb->gdal->hflip) {
	int i;

	for (i = 0; i < fcb->cellhd.cols; i++)
	    memcpy(data_buf + i * fcb->cur_nbytes,
		   buf + (fcb->cellhd.cols - 1 - i) * fcb->cur_nbytes,
		   fcb->cur_nbytes);
	G_free(buf);
    }

    if (err != CE_None)
	G_fatal_error(_("Error reading raster data via GDAL for row %d of <%s>"),
		      row, fcb->name);
}
#endif

static void read_data(int fd, int row, unsigned char *data_buf, int *nbytes)
{
    struct fileinfo *fcb = &R__.fileinfo[fd];

#ifdef HAVE_GDAL
    if (fcb->gdal) {
	read_data_gdal(fd, row, data_buf, nbytes);
	return;
    }
#endif

    if (!fcb->cellhd.compressed)
	read_data_uncompressed(fd, row, data_buf, nbytes);
    else if (fcb->map_type == CELL_TYPE)
	read_data_compressed(fd, row, data_buf, nbytes);
    else
	read_data_fp_compressed(fd, row, data_buf, nbytes);
}

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

static void cell_values_float(int fd, const unsigned char *data,
			      const COLUMN_MAPPING * cmap, int nbytes,
			      void *cell, int n)
{
    struct fileinfo *fcb = &R__.fileinfo[fd];
    const float *work_buf = (const float *) fcb->data;
    FCELL *c = cell;
    int i;

    for (i = 0; i < n; i++) {
	if (!cmap[i]) {
	    c[i] = 0;
	    continue;
	}

	G_xdr_get_float(&c[i], &work_buf[cmap[i]-1]);
    }
}

static void cell_values_double(int fd, const unsigned char *data,
			       const COLUMN_MAPPING * cmap, int nbytes,
			       void *cell, int n)
{
    struct fileinfo *fcb = &R__.fileinfo[fd];
    const double *work_buf = (const double *) fcb->data;
    DCELL *c = cell;
    int i;

    for (i = 0; i < n; i++) {
	if (!cmap[i]) {
	    c[i] = 0;
	    continue;
	}

	G_xdr_get_double(&c[i], &work_buf[cmap[i]-1]);
    }
}

#ifdef HAVE_GDAL
static void gdal_values_int(int fd, const unsigned char *data,
			    const COLUMN_MAPPING * cmap, int nbytes,
			    CELL * cell, int n)
{
    struct fileinfo *fcb = &R__.fileinfo[fd];
    const unsigned char *d;
    COLUMN_MAPPING cmapold = 0;
    int i;

    for (i = 0; i < n; i++) {
	if (!cmap[i]) {
	    cell[i] = 0;
	    continue;
	}

	if (cmap[i] == cmapold) {
	    cell[i] = cell[i - 1];
	    continue;
	}

	d = data + (cmap[i] - 1) * nbytes;

	switch (fcb->gdal->type) {
	case GDT_Byte:
	    cell[i] = *(GByte *) d;
	    break;
	case GDT_Int16:
	    cell[i] = *(GInt16 *) d;
	    break;
	case GDT_UInt16:
	    cell[i] = *(GUInt16 *) d;
	    break;
	case GDT_Int32:
	    cell[i] = *(GInt32 *) d;
	    break;
	case GDT_UInt32:
	    cell[i] = *(GUInt32 *) d;
	    break;
	default:
	    /* shouldn't happen */
	    Rast_set_c_null_value(&cell[i], 1);
	    break;
	}

	cmapold = cmap[i];
    }
}

static void gdal_values_float(int fd, const float *data,
			      const COLUMN_MAPPING * cmap, int nbytes,
			      FCELL * cell, int n)
{
    COLUMN_MAPPING cmapold = 0;
    int i;

    for (i = 0; i < n; i++) {
	if (!cmap[i]) {
	    cell[i] = 0;
	    continue;
	}

	if (cmap[i] == cmapold) {
	    cell[i] = cell[i - 1];
	    continue;
	}

	cell[i] = data[cmap[i] - 1];

	cmapold = cmap[i];
    }
}

static void gdal_values_double(int fd, const double *data,
			       const COLUMN_MAPPING * cmap, int nbytes,
			       DCELL * cell, int n)
{
    COLUMN_MAPPING cmapold = 0;
    int i;

    for (i = 0; i < n; i++) {
	if (!cmap[i]) {
	    cell[i] = 0;
	    continue;
	}

	if (cmap[i] == cmapold) {
	    cell[i] = cell[i - 1];
	    continue;
	}

	cell[i] = data[cmap[i] - 1];

	cmapold = cmap[i];
    }
}
#endif

/* transfer_to_cell_XY takes bytes from fcb->data, converts these bytes with
   the appropriate procedure (e.g. XDR or byte reordering) into type X 
   values which are put into array work_buf.  
   finally the values in work_buf are converted into 
   type Y and put into 'cell'.
   if type X == type Y the intermediate step of storing the values in 
   work_buf might be omitted. check the appropriate function for XY to
   determine the procedure of conversion. 
 */
static void transfer_to_cell_XX(int fd, void *cell)
{
    static void (*cell_values_type[3]) () = {
    cell_values_int, cell_values_float, cell_values_double};
#ifdef HAVE_GDAL
    static void (*gdal_values_type[3]) () = {
    gdal_values_int, gdal_values_float, gdal_values_double};
#endif
    struct fileinfo *fcb = &R__.fileinfo[fd];

#ifdef HAVE_GDAL
    if (fcb->gdal)
	(gdal_values_type[fcb->map_type]) (fd, fcb->data, fcb->col_map,
					   fcb->cur_nbytes, cell,
					   R__.rd_window.cols);
    else
#endif
	(cell_values_type[fcb->map_type]) (fd, fcb->data, fcb->col_map,
					   fcb->cur_nbytes, cell,
					   R__.rd_window.cols);
}

static void transfer_to_cell_fi(int fd, void *cell)
{
    struct fileinfo *fcb = &R__.fileinfo[fd];
    FCELL *work_buf = G_malloc(R__.rd_window.cols * sizeof(FCELL));
    int i;

    transfer_to_cell_XX(fd, work_buf);

    for (i = 0; i < R__.rd_window.cols; i++)
	((CELL *) cell)[i] = (fcb->col_map[i] == 0)
	    ? 0 : Rast_quant_get_cell_value(&fcb->quant, work_buf[i]);

    G_free(work_buf);
}

static void transfer_to_cell_di(int fd, void *cell)
{
    struct fileinfo *fcb = &R__.fileinfo[fd];
    DCELL *work_buf = G_malloc(R__.rd_window.cols * sizeof(DCELL));
    int i;

    transfer_to_cell_XX(fd, work_buf);

    for (i = 0; i < R__.rd_window.cols; i++)
	((CELL *) cell)[i] = (fcb->col_map[i] == 0)
	    ? 0 : Rast_quant_get_cell_value(&fcb->quant, work_buf[i]);

    G_free(work_buf);
}

static void transfer_to_cell_if(int fd, void *cell)
{
    CELL *work_buf = G_malloc(R__.rd_window.cols * sizeof(CELL));
    int i;

    transfer_to_cell_XX(fd, work_buf);

    for (i = 0; i < R__.rd_window.cols; i++)
	((FCELL *) cell)[i] = work_buf[i];

    G_free(work_buf);
}

static void transfer_to_cell_df(int fd, void *cell)
{
    DCELL *work_buf = G_malloc(R__.rd_window.cols * sizeof(DCELL));
    int i;

    transfer_to_cell_XX(fd, work_buf);

    for (i = 0; i < R__.rd_window.cols; i++)
	((FCELL *) cell)[i] = work_buf[i];

    G_free(work_buf);
}

static void transfer_to_cell_id(int fd, void *cell)
{
    CELL *work_buf = G_malloc(R__.rd_window.cols * sizeof(CELL));
    int i;

    transfer_to_cell_XX(fd, work_buf);

    for (i = 0; i < R__.rd_window.cols; i++)
	((DCELL *) cell)[i] = work_buf[i];

    G_free(work_buf);
}

static void transfer_to_cell_fd(int fd, void *cell)
{
    FCELL *work_buf = G_malloc(R__.rd_window.cols * sizeof(FCELL));
    int i;

    transfer_to_cell_XX(fd, work_buf);

    for (i = 0; i < R__.rd_window.cols; i++)
	((DCELL *) cell)[i] = work_buf[i];

    G_free(work_buf);
}

/*
 *   works for all map types and doesn't consider
 *   null row corresponding to the requested row 
 */
static int get_map_row_nomask(int fd, void *rast, int row,
			      RASTER_MAP_TYPE data_type)
{
    static void (*transfer_to_cell_FtypeOtype[3][3]) () = {
	{
	transfer_to_cell_XX, transfer_to_cell_if, transfer_to_cell_id}, {
	transfer_to_cell_fi, transfer_to_cell_XX, transfer_to_cell_fd}, {
	transfer_to_cell_di, transfer_to_cell_df, transfer_to_cell_XX}
    };
    struct fileinfo *fcb = &R__.fileinfo[fd];
    int r;
    int row_status;

    /* is this the best place to read a vrt row, or
     * call Rast_get_vrt_row() earlier ? */
    if (fcb->vrt)
	return Rast_get_vrt_row(fd, rast, row, data_type);

    row_status = compute_window_row(fd, row, &r);

    if (!row_status) {
	fcb->cur_row = -1;
	Rast_zero_input_buf(rast, data_type);
	return 0;
    }

    /* read cell file row if not in memory */
    if (r != fcb->cur_row) {
	fcb->cur_row = r;
	read_data(fd, fcb->cur_row, fcb->data, &fcb->cur_nbytes);
    }

    (transfer_to_cell_FtypeOtype[fcb->map_type][data_type]) (fd, rast);

    return 1;
}

static void get_map_row_no_reclass(int fd, void *rast, int row,
				   RASTER_MAP_TYPE data_type, int null_is_zero,
				   int with_mask)
{
    get_map_row_nomask(fd, rast, row, data_type);
    embed_nulls(fd, rast, row, data_type, null_is_zero, with_mask);
}

static void get_map_row(int fd, void *rast, int row, RASTER_MAP_TYPE data_type,
			int null_is_zero, int with_mask)
{
    struct fileinfo *fcb = &R__.fileinfo[fd];
    int size = Rast_cell_size(data_type);
    CELL *temp_buf = NULL;
    void *buf;
    int type;
    int i;

    if (fcb->reclass_flag && data_type != CELL_TYPE) {
	temp_buf = G_malloc(R__.rd_window.cols * sizeof(CELL));
	buf = temp_buf;
	type = CELL_TYPE;
    }
    else {
	buf = rast;
	type = data_type;
    }

    get_map_row_no_reclass(fd, buf, row, type, null_is_zero, with_mask);

    if (!fcb->reclass_flag)
	return;

    /* if the map is reclass table, get and
       reclass CELL row and copy results to needed type  */

    do_reclass_int(fd, buf, null_is_zero);

    if (data_type == CELL_TYPE)
	return;

    for (i = 0; i < R__.rd_window.cols; i++) {
	Rast_set_c_value(rast, temp_buf[i], data_type);
	rast = G_incr_void_ptr(rast, size);
    }

    if (fcb->reclass_flag && data_type != CELL_TYPE) {
	G_free(temp_buf);
    }
}

/*!
 * \brief Read raster row without masking
 *
 * This routine reads the specified <em>row</em> from the raster map
 * open on file descriptor <em>fd</em> into the <em>buf</em> buffer
 * like Rast_get_c_row() does. The difference is that masking is
 * suppressed. If the user has a mask set, Rast_get_c_row() will apply
 * the mask but Rast_get_c_row_nomask() will ignore it. This routine
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
 * \param fd file descriptor for the opened raster map
 * \param buf buffer for the row to be placed into
 * \param row data row desired
 * \param data_type data type
 *
 * \return void
 */
void Rast_get_row_nomask(int fd, void *buf, int row, RASTER_MAP_TYPE data_type)
{
    get_map_row(fd, buf, row, data_type, 0, 0);
}

/*!
 * \brief Read raster row without masking (CELL type)
 *
 *  Same as Rast_get_c_row() except no masking occurs.
 *
 * \param fd file descriptor for the opened raster map
 * \param buf buffer for the row to be placed into
 * \param row data row desired
 *
 * \return void
 */
void Rast_get_c_row_nomask(int fd, CELL * buf, int row)
{
    Rast_get_row_nomask(fd, buf, row, CELL_TYPE);
}

/*!
 * \brief Read raster row without masking (FCELL type)
 *
 *  Same as Rast_get_f_row() except no masking occurs.
 *
 * \param fd file descriptor for the opened raster map
 * \param buf buffer for the row to be placed into
 * \param row data row desired
 *
 * \return void
 */
void Rast_get_f_row_nomask(int fd, FCELL * buf, int row)
{
    Rast_get_row_nomask(fd, buf, row, FCELL_TYPE);
}

/*!
 * \brief Read raster row without masking (DCELL type)
 *
 *  Same as Rast_get_d_row() except no masking occurs.
 *
 * \param fd file descriptor for the opened raster map
 * \param buf buffer for the row to be placed into
 * \param row data row desired
 *
 * \return void
 */
void Rast_get_d_row_nomask(int fd, DCELL * buf, int row)
{
    Rast_get_row_nomask(fd, buf, row, DCELL_TYPE);
}

/*!
 * \brief Get raster row
 *
 * If <em>data_type</em> is
 *  - CELL_TYPE, calls Rast_get_c_row()
 *  - FCELL_TYPE, calls Rast_get_f_row()
 *  - DCELL_TYPE, calls Rast_get_d_row()
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
 *            of Rast_get_row() and assuming that all data is valid 
 *            in case of G_get_f/d_raster_row(). In case of deprecated function
 *            Rast_get_c_row() all nulls are converted to zeros (so there are
 *            no embedded nulls at all). Also all masked out cells become zeros.
 *
 * \param fd file descriptor for the opened raster map
 * \param buf buffer for the row to be placed into
 * \param row data row desired
 * \param data_type data type
 *
 * \return void
 */
void Rast_get_row(int fd, void *buf, int row, RASTER_MAP_TYPE data_type)
{
    get_map_row(fd, buf, row, data_type, 0, 1);
}

/*!
 * \brief Get raster row (CELL type)
 *
 * Reads a row of raster data and leaves the NULL values intact. (As
 * opposed to the deprecated function Rast_get_c_row() which
 * converts NULL values to zero.) 
 *
 * <b>NOTE.</b> When the raster map is old and null file doesn't
 * exist, it is assumed that all 0-cells are no-data. When map is
 * floating point, uses quant rules set explicitly by
 * Rast_set_quant_rules() or stored in map's quant file to convert floats
 * to integers.
 *
 * \param fd file descriptor for the opened raster map
 * \param buf buffer for the row to be placed into
 * \param row data row desired
 *
 * \return void
 */
void Rast_get_c_row(int fd, CELL * buf, int row)
{
    Rast_get_row(fd, buf, row, CELL_TYPE);
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
 * \return void
 */
void Rast_get_f_row(int fd, FCELL * buf, int row)
{
    Rast_get_row(fd, buf, row, FCELL_TYPE);
}

/*!
 * \brief Get raster row (DCELL type)
 *
 * Same as Rast_get_f_row() except that the array <em>dcell</em>
 * is <tt>double</tt>.
 *
 * \param fd file descriptor for the opened raster map
 * \param buf buffer for the row to be placed into
 * \param row data row desired
 *
 * \return void
 */
void Rast_get_d_row(int fd, DCELL * buf, int row)
{
    Rast_get_row(fd, buf, row, DCELL_TYPE);
}

static int read_null_bits_compressed(int null_fd, unsigned char *flags,
				     int row, size_t size, int fd)
{
    struct fileinfo *fcb = &R__.fileinfo[fd];
    off_t t1 = fcb->null_row_ptr[row];
    off_t t2 = fcb->null_row_ptr[row + 1];
    size_t readamount = t2 - t1;
    unsigned char *compressed_buf;

    if (lseek(null_fd, t1, SEEK_SET) < 0)
	G_fatal_error(_("Error seeking compressed null data for row %d of <%s>"),
		      row, fcb->name);

    if (readamount == size) {
	if (read(null_fd, flags, size) != size) {
	    G_fatal_error(_("Error reading compressed null data for row %d of <%s>"),
			  row, fcb->name);
	}
	return 1;
    }

    compressed_buf = G_malloc(readamount);

    if (read(null_fd, compressed_buf, readamount) != readamount) {
	G_free(compressed_buf);
	G_fatal_error(_("Error reading compressed null data for row %d of <%s>"),
		      row, fcb->name);
    }

    /* null bits file compressed with LZ4, see lib/gis/compress.h */
    if (G_lz4_expand(compressed_buf, readamount, flags, size) < 1) {
	G_fatal_error(_("Error uncompressing null data for row %d of <%s>"),
		      row, fcb->name);
    }

    G_free(compressed_buf);

    return 1;
}

int Rast__read_null_bits(int fd, int row, unsigned char *flags)
{
    struct fileinfo *fcb = &R__.fileinfo[fd];
    int null_fd = fcb->null_fd;
    int cols = fcb->cellhd.cols;
    off_t offset;
    ssize_t size;
    int R;

    if (compute_window_row(fd, row, &R) <= 0) {
	Rast__init_null_bits(flags, cols);
	return 1;
    }

    if (null_fd < 0)
	return 0;

    size = Rast__null_bitstream_size(cols);

    if (fcb->null_row_ptr)
	return read_null_bits_compressed(null_fd, flags, R, size, fd);

    offset = (off_t) size * R;

    if (lseek(null_fd, offset, SEEK_SET) < 0)
	G_fatal_error(_("Error seeking null row %d for <%s>"), R, fcb->name);

    if (read(null_fd, flags, size) != size)
	G_fatal_error(_("Error reading null row %d for <%s>"), R, fcb->name);

    return 1;
}

#define check_null_bit(flags, bit_num) ((flags)[(bit_num)>>3] & ((unsigned char)0x80>>((bit_num)&7)) ? 1 : 0)

static void get_null_value_row_nomask(int fd, char *flags, int row)
{
    struct fileinfo *fcb = &R__.fileinfo[fd];
    int j;

    if (row > R__.rd_window.rows || row < 0) {
	G_warning(_("Reading raster map <%s@%s> request for row %d is outside region"),
		  fcb->name, fcb->mapset, row);
	for (j = 0; j < R__.rd_window.cols; j++)
	    flags[j] = 1;
	return;
    }
    if (fcb->vrt) {
	/* vrt: already done when reading the real maps, no extra NULL values */
	for (j = 0; j < R__.rd_window.cols; j++)
	    flags[j] = 0;
	return;
    }

    if (row != fcb->null_cur_row) {
	if (!Rast__read_null_bits(fd, row, fcb->null_bits)) {
	    fcb->null_cur_row = -1;
	    if (fcb->map_type == CELL_TYPE) {
		/* If can't read null row, assume  that all map 0's are nulls */
		CELL *mask_buf = G_malloc(R__.rd_window.cols * sizeof(CELL));

		get_map_row_nomask(fd, mask_buf, row, CELL_TYPE);
		for (j = 0; j < R__.rd_window.cols; j++)
		    flags[j] = (mask_buf[j] == 0);

		G_free(mask_buf);
	    }
	    else {		/* fp map */
		/* if can't read null row, assume  that all data is valid */
		G_zero(flags, sizeof(char) * R__.rd_window.cols);
		/* the flags row is ready now */
	    }

	    return;
	}			/*if no null file */
	else
	    fcb->null_cur_row = row;
    }

    /* copy null row to flags row translated by window column mapping */
    for (j = 0; j < R__.rd_window.cols; j++) {
	if (!fcb->col_map[j])
	    flags[j] = 1;
	else
	    flags[j] = check_null_bit(fcb->null_bits, fcb->col_map[j] - 1);
    }
}

/*--------------------------------------------------------------------------*/

#ifdef HAVE_GDAL

static void get_null_value_row_gdal(int fd, char *flags, int row)
{
    struct fileinfo *fcb = &R__.fileinfo[fd];
    DCELL *tmp_buf = Rast_allocate_d_input_buf();
    int i;

    if (get_map_row_nomask(fd, tmp_buf, row, DCELL_TYPE) <= 0) {
	memset(flags, 1, R__.rd_window.cols);
	G_free(tmp_buf);
	return;
    }

    for (i = 0; i < R__.rd_window.cols; i++)
	/* note: using == won't work if the null value is NaN */
	flags[i] = !fcb->col_map[i] ||
	    tmp_buf[i] == fcb->gdal->null_val ||
	    tmp_buf[i] != tmp_buf[i];

    G_free(tmp_buf);
}

#endif

/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/

static void embed_mask(char *flags, int row)
{
    CELL *mask_buf = G_malloc(R__.rd_window.cols * sizeof(CELL));
    int i;

    if (R__.auto_mask <= 0) {
	G_free(mask_buf);
	return;
    }

    if (get_map_row_nomask(R__.mask_fd, mask_buf, row, CELL_TYPE) < 0) {
	G_free(mask_buf);
	return;
    }

    if (R__.fileinfo[R__.mask_fd].reclass_flag) {
	embed_nulls(R__.mask_fd, mask_buf, row, CELL_TYPE, 0, 0);
	do_reclass_int(R__.mask_fd, mask_buf, 1);
    }

    for (i = 0; i < R__.rd_window.cols; i++)
	if (mask_buf[i] == 0 || Rast_is_c_null_value(&mask_buf[i]))
	    flags[i] = 1;

    G_free(mask_buf);
}

static void get_null_value_row(int fd, char *flags, int row, int with_mask)
{
#ifdef HAVE_GDAL
    struct fileinfo *fcb = &R__.fileinfo[fd];

    if (fcb->gdal)
	get_null_value_row_gdal(fd, flags, row);
    else
#endif
	get_null_value_row_nomask(fd, flags, row);

    if (with_mask)
	embed_mask(flags, row);
}

static void embed_nulls(int fd, void *buf, int row, RASTER_MAP_TYPE map_type,
			int null_is_zero, int with_mask)
{
    struct fileinfo *fcb = &R__.fileinfo[fd];
    size_t size = Rast_cell_size(map_type);
    char *null_buf;
    int i;

    /* this is because without null file the nulls can be only due to 0's
       in data row or mask */
    if (null_is_zero && !fcb->null_file_exists
	&& (R__.auto_mask <= 0 || !with_mask))
	return;

    null_buf = G_malloc(R__.rd_window.cols);

    get_null_value_row(fd, null_buf, row, with_mask);

    for (i = 0; i < R__.rd_window.cols; i++) {
	/* also check for nulls which might be already embedded by quant
	   rules in case of fp map. */
	if (null_buf[i] || Rast_is_null_value(buf, map_type)) {
	    /* G__set_[f/d]_null_value() sets it to 0 is the embedded mode
	       is not set and calls G_set_[f/d]_null_value() otherwise */
	    Rast__set_null_value(buf, 1, null_is_zero, map_type);
	}
	buf = G_incr_void_ptr(buf, size);
    }

    G_free(null_buf);
}

/*!
   \brief Read or simulate null value row

   Read or simulate null value row and set the cells corresponding 
   to null value to 1. The masked out cells are set to null when the
   mask exists. (the MASK is taken care of by null values
   (if the null file doesn't exist for this map, then the null row
   is simulated by assuming that all zeros in raster map are nulls.
   Also all masked out cells become nulls.

   \param fd file descriptor for the opened map
   \param buf buffer for the row to be placed into
   \param flags
   \param row data row desired

   \return void
 */
void Rast_get_null_value_row(int fd, char *flags, int row)
{
    struct fileinfo *fcb = &R__.fileinfo[fd];

    if (!fcb->reclass_flag)
	get_null_value_row(fd, flags, row, 1);
    else {
	CELL *buf = G_malloc(R__.rd_window.cols * sizeof(CELL));
	int i;

	Rast_get_c_row(fd, buf, row);
	for (i = 0; i < R__.rd_window.cols; i++)
	    flags[i] = Rast_is_c_null_value(&buf[i]) ? 1 : 0;

	G_free(buf);
    }
}
