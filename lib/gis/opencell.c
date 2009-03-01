/*!
 * \file gis/opencell.c
 * 
 * \brief GIS Library - open raster (cell) file functions
 *
 * (C) 1999-2008 by the GRASS Development Team
 *
 * This program is free software under the GNU General Public
 * License (>=v2). Read the file COPYING that comes with GRASS
 * for details.
 *
 * \author USACERL and many others
 */

#include <rpc/types.h>
#include <rpc/xdr.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <grass/config.h>
#include "G.h"
#include <grass/gis.h>
#include <grass/glocale.h>

static struct fileinfo *new_fileinfo(int fd)
{
    int oldsize = G__.fileinfo_count;
    int newsize = oldsize;
    int i;

    if (fd < oldsize)
	return &G__.fileinfo[fd];

    newsize *= 2;
    if (newsize <= fd)
	newsize = fd + 20;

    G__.fileinfo = G_realloc(G__.fileinfo, newsize * sizeof(struct fileinfo));

    /* Mark all cell files as closed */
    for (i = oldsize; i < newsize; i++) {
	memset(&G__.fileinfo[i], 0, sizeof(struct fileinfo));
	G__.fileinfo[i].open_mode = -1;
    }

    G__.fileinfo_count = newsize;

    return &G__.fileinfo[fd];
}


/*!
 * \brief 
 *
 * Arrange for the NULL-value bitmap to be
 * read as well as the raster map.  If no NULL-value bitmap exists, arrange for
 * the production of NULL-values based on zeros in the raster map.
 * If the map is floating-point, arrange for quantization to integer for 
 * <tt>G_get_c_raster_row()</tt>, et. al., by reading the quantization rules for
 * the map using <tt>G_read_quant()</tt>.
 * If the programmer wants to read the floating point map using uing quant rules
 * other than the ones stored in map's quant file, he/she should call
 * G_set_quant_rules() after the call to G_open_cell_old().
 *
 *  \return int
 */

static int G__open_raster_new(const char *name, int open_mode,
			      RASTER_MAP_TYPE map_type);

/*!
  \brief Open an existing integer raster map (cell)
  
  Opens the existing cell file <i>name</i> in the <i>mapset</i> for
  reading by G_get_raster_row() with mapping into the current window.

  This routine opens the raster map <i>name</i> in <i>mapset</i> for
  reading. A nonnegative file descriptor is returned if the open is
  successful. Otherwise a diagnostic message is printed and a negative
  value is returned. This routine does quite a bit of work. Since
  GRASS users expect that all raster maps will be resampled into the
  current region, the resampling index for the raster map is prepared
  by this routine after the file is opened. The resampling is based on
  the active module region.\remarks{See also The_Region.} Preparation
  required for reading the various raster file formats\remarks{See
  Raster_File_Format for an explanation of the various raster file
  formats.} is also done.
 
  Diagnostics: warning message printed if open fails.

  \param name map name
  \param mapset mapset name where raster map <i>name</i> lives
  
  \return nonnegative file descriptor (int)
  \return -1 on failure
 */
int G_open_cell_old(const char *name, const char *mapset)
{
    int fd;

    if ((fd = G__open_cell_old(name, mapset)) < 0) {
	G_warning(_("Unable to open raster map <%s@%s>"), name, mapset);
	return fd;
    }

    /* turn on auto masking, if not already on */
    G__check_for_auto_masking();
    /*
       if(G__.auto_mask <= 0)
       G__.mask_buf = G_allocate_cell_buf();
       now we don't ever free it!, so no need to allocate it  (Olga)
     */
    /* mask_buf is used for reading MASK file when mask is set and
       for reading map rows when the null file doesn't exist */

    return fd;
}

/*!
  \brief Lower level function, open cell files, supercell
  files, and the MASK file. 

  Actions:
   - opens the named cell file, following reclass reference if
   named layer is a reclass layer.
   - creates the required mapping between the data and the window
   for use by the get_map_row family of routines.
  
  Diagnostics:
  Errors other than actual open failure will cause a diagnostic to be
  delivered thru G_warning() open failure messages are left to the
  calling routine since the masking logic will want to issue a different
  warning.
  
  Note:
  This routine does NOT open the MASK layer. If it did we would get
  infinite recursion.  This routine is called to open the mask by
  G__check_for_auto_masking() which is called by G_open_cell_old().
 
  \param name map name
  \param mapset mapset of cell file to be opened
 
  \return open file descriptor
  \return -1 if error
*/
int G__open_cell_old(const char *name, const char *mapset)
{
    struct fileinfo *fcb;
    int fd;
    char *cell_dir;
    const char *r_name;
    const char *r_mapset;
    struct Cell_head cellhd;
    int CELL_nbytes = 0;	/* bytes per cell in CELL map */
    int INTERN_SIZE;
    int reclass_flag, i;
    int MAP_NBYTES;
    RASTER_MAP_TYPE MAP_TYPE;
    struct Reclass reclass;
    char xname[GNAME_MAX], xmapset[GMAPSET_MAX];
    struct GDAL_link *gdal;

    /* make sure window is set    */
    G__init_window();

    G__unqualified_name(name, mapset, xname, xmapset);
    name = xname;
    mapset = xmapset;

    if (!G_find_cell2(name, mapset)) {
	G_warning(_("Unable to find <%s@%s>"), name, mapset);
	return -1;
    }

    /* Check for reclassification */
    reclass_flag = G_get_reclass(name, mapset, &reclass);

    switch (reclass_flag) {
    case 0:
	r_name = name;
	r_mapset = mapset;
	break;
    case 1:
	r_name = reclass.name;
	r_mapset = reclass.mapset;
	if (G_find_cell2(r_name, r_mapset) == NULL) {
	    G_warning(_("Unable to open raster map <%s@%s> since it is a reclass "
			"of raster map <%s@%s> which does not exist"),
		      name, mapset, r_name, r_mapset);
	    return -1;
	}
	break;
    default:			/* Error reading cellhd/reclass file */
	return -1;
    }

    /* read the cell header */
    if (G_get_cellhd(r_name, r_mapset, &cellhd) < 0)
	return -1;

    /* now check the type */
    MAP_TYPE = G_raster_map_type(r_name, r_mapset);
    if (MAP_TYPE < 0)
	return -1;

    if (MAP_TYPE == CELL_TYPE)
	/* set the number of bytes for CELL map */
    {
	CELL_nbytes = cellhd.format + 1;
	if (CELL_nbytes < 1) {
	    G_warning(_("Raster map <%s@%s>: format field in header file invalid"),
		      r_name, r_mapset);
	    return -1;
	}
    }

    if (cellhd.proj != G__.window.proj) {
	G_warning(_("Raster map <%s@%s> is in different projection than current region. "
		    "Found raster map <%s@%s>, should be <%s>."),
		  name, mapset, name, G__projection_name(cellhd.proj),
		  G__projection_name(G__.window.proj));
	return -1;
    }
    if (cellhd.zone != G__.window.zone) {
	G_warning(_("Raster map <%s@%s> is in different zone (%d) than current region (%d)"),
		  name, mapset, cellhd.zone, G__.window.zone);
	return -1;
    }

    /* when map is int warn if too large cell size */
    if (MAP_TYPE == CELL_TYPE && (unsigned int) CELL_nbytes > sizeof(CELL)) {
	G_warning(_("Raster map <%s@%s>: bytes per cell (%d) too large"),
		  name, mapset, CELL_nbytes);
	return -1;
    }

    /* record number of bytes per cell */
    if (MAP_TYPE == FCELL_TYPE) {
	cell_dir = "fcell";
	INTERN_SIZE = sizeof(FCELL);
	MAP_NBYTES = XDR_FLOAT_NBYTES;
    }
    else if (MAP_TYPE == DCELL_TYPE) {
	cell_dir = "fcell";
	INTERN_SIZE = sizeof(DCELL);
	MAP_NBYTES = XDR_DOUBLE_NBYTES;
    }
    else {			/* integer */

	cell_dir = "cell";
	INTERN_SIZE = sizeof(CELL);
	MAP_NBYTES = CELL_nbytes;
    }

    gdal = G_get_gdal_link(r_name, r_mapset);
    if (gdal) {
#ifdef HAVE_GDAL
	/* dummy descriptor to reserve the fileinfo slot */
	fd = open(G_DEV_NULL, O_RDONLY);
#else
	G_warning(_("map <%s@%s> is a GDAL link but GRASS is compiled without GDAL support"),
		  r_name, r_mapset);
	return -1;
#endif
    }
    else
	/* now actually open file for reading */
	fd = G_open_old(cell_dir, r_name, r_mapset);

    if (fd < 0)
	return -1;

    fcb = new_fileinfo(fd);

    fcb->map_type = MAP_TYPE;

    /* Save cell header */
    G_copy((char *)&fcb->cellhd, (char *)&cellhd, sizeof(cellhd));

    /* allocate null bitstream buffers for reading null rows */
    for (i = 0; i < NULL_ROWS_INMEM; i++)
	fcb->NULL_ROWS[i] = G__allocate_null_bits(G__.window.cols);
    /* initialize : no NULL rows in memory */
    fcb->min_null_row = (-1) * NULL_ROWS_INMEM;

    /* mark closed */
    fcb->open_mode = -1;

    /* save name and mapset */
    fcb->name = G_store(name);
    fcb->mapset = G_store(mapset);

    /* mark no data row in memory  */
    fcb->cur_row = -1;
    /* fcb->null_cur_row is not used for reading, only for writing */
    fcb->null_cur_row = -1;

    /* if reclass, copy reclass structure */
    if ((fcb->reclass_flag = reclass_flag))
	G_copy(&fcb->reclass, &reclass, sizeof(reclass));

    fcb->gdal = gdal;
    if (!gdal)
	/* check for compressed data format, making initial reads if necessary */
	if (G__check_format(fd) < 0) {
	    close(fd);		/* warning issued by check_format() */
	    return -1;
	}

    /* create the mapping from cell file to window */
    G__create_window_mapping(fd);

    /*
     * allocate the data buffer
     * number of bytes per cell is cellhd.format+1
     */

    /* for reading fcb->data is allocated to be fcb->cellhd.cols * fcb->nbytes 
       (= XDR_FLOAT/DOUBLE_NBYTES) */
    fcb->data = (unsigned char *)G_calloc(fcb->cellhd.cols, MAP_NBYTES);

    /* initialize/read in quant rules for float point maps */
    if (fcb->map_type != CELL_TYPE) {
	if (fcb->reclass_flag)
	    G_read_quant(fcb->reclass.name, fcb->reclass.mapset,
			 &(fcb->quant));
	else
	    G_read_quant(fcb->name, fcb->mapset, &(fcb->quant));
    }

    /* now mark open for read: this must follow create_window_mapping() */
    fcb->open_mode = OPEN_OLD;
    fcb->io_error = 0;
    fcb->map_type = MAP_TYPE;
    fcb->nbytes = MAP_NBYTES;
    fcb->null_file_exists = -1;

    if (fcb->map_type != CELL_TYPE)
	xdrmem_create(&fcb->xdrstream, (caddr_t) fcb->data,
		      (u_int) (fcb->nbytes * fcb->cellhd.cols), XDR_DECODE);

    return fd;
}

/*****************************************************************/

/*!
  \brief Opens a new cell file in a database (compressed)

  Opens a new cell file <i>name</i> in the current mapset for writing
  by G_put_raster_row().
 
  The file is created and filled with no data it is assumed that the
  new cell file is to conform to the current window.
 
  The file must be written sequentially. Use G_open_cell_new_random()
  for non sequential writes.
  
  Note: the open actually creates a temporary file G_close_cell() will
  move the temporary file to the cell file and write out the necessary
  support files (cellhd, cats, hist, etc.).

  Diagnostics: warning message printed if open fails
 
  Warning: calls to G_set_window() made after opening a new cell file
  may create confusion and should be avoided the new cell file will be
  created to conform to the window at the time of the open.

  \param name map name

  \return open file descriptor ( >= 0) if successful
  \return negative integer if error
*/
int G_open_cell_new(const char *name)
{
    return G__open_raster_new(name, OPEN_NEW_COMPRESSED, CELL_TYPE);
}

/*!
  \brief Opens a new cell file in a database (uncompressed)

  See also G_open_cell_new().
 
  \param name map name

  \return open file descriptor ( >= 0) if successful
  \return negative integer if error
*/
int G_open_cell_new_uncompressed(const char *name)
{
    return G__open_raster_new(name, OPEN_NEW_UNCOMPRESSED, CELL_TYPE);
}

/*!
  \brief Save histogram for newly create raster map (cell)

  If newly created cell files should have histograms, set flag=1
  otherwise set flag=0. Applies to subsequent opens.

  \param flag flag indicator

  \return 0
*/
void G_want_histogram(int flag)
{
    G__.want_histogram = flag;
}

/*!
  \brief Sets the format for subsequent opens on new integer cell files
  (uncompressed and random only).

  Warning: subsequent put_row calls will only write n+1 bytes
  per cell. If the data requires more, the cell file
  will be written incorrectly (but with n+1 bytes per cell)

  When writing float map: format is -1

  \param n format
  
  \return 0
*/
void G_set_cell_format(int n)
/* sets the format for integer raster map */
{
    G__.nbytes = n + 1;
    if (G__.nbytes <= 0)
	G__.nbytes = 1;
    if (G__.nbytes > sizeof(CELL))
	G__.nbytes = sizeof(CELL);
}

/*!
  \brief Get cell value format

  \param v cell

  \return cell format
*/
int G_cellvalue_format(CELL v)
{
    unsigned int i;

    if (v >= 0)
	for (i = 0; i < sizeof(CELL); i++)
	    if (!(v /= 256))
		return i;
    return sizeof(CELL) - 1;
}

/*!
  \brief Opens new fcell file in a database

  Opens a new floating-point map <i>name</i> in the current mapset for
  writing. The type of the file (i.e. either double or float) is
  determined and fixed at this point. The default is FCELL_TYPE. In
  order to change this default

  Use G_set_fp_type() where type is one of DCELL_TYPE or FCELL_TYPE.

  See warnings and notes for G_open_cell_new().

  \param name map name

  \return nonnegative file descriptor (int)
  \return -1 on error
*/
int G_open_fp_cell_new(const char *name)
{
    return G__open_raster_new(name, OPEN_NEW_COMPRESSED, G__.fp_type);
}

/*!
  \brief Opens new fcell file in a database (uncompressed)

  See G_open_fp_cell_new() for details.

  \param name map name

  \return nonnegative file descriptor (int)
  \return -1 on error
*/
int G_open_fp_cell_new_uncompressed(const char *name)
{
    return G__open_raster_new(name, OPEN_NEW_UNCOMPRESSED, G__.fp_type);
}

#ifdef HAVE_GDAL
static int G__open_raster_new_gdal(char *map, char *mapset, RASTER_MAP_TYPE map_type)
{
    int fd;
    struct fileinfo *fcb;
    int i;

    /* dummy descriptor to reserve the fileinfo slot */
    fd = open("/dev/null", O_RDONLY);
    if (fd < 0)
	return -1;

    fcb = new_fileinfo(fd);

    /* mark closed */
    fcb->map_type = map_type;
    fcb->open_mode = -1;

    fcb->gdal = G_create_gdal_link(map, map_type);
    if (!fcb->gdal)
	return -1;

    fcb->cellhd = G__.window;
    fcb->cellhd.compressed = 0;
    fcb->nbytes = G_raster_size(fcb->map_type);
    /* for writing fcb->data is allocated to be G__.window.cols * 
       sizeof(CELL or DCELL or FCELL)  */
    fcb->data = G_calloc(G__.window.cols, fcb->nbytes);

    fcb->name = map;
    fcb->mapset = mapset;
    fcb->cur_row = 0;

    fcb->row_ptr = NULL;
    fcb->temp_name = NULL;
    fcb->null_temp_name = NULL;
    fcb->null_cur_row = 0;
    fcb->min_null_row = 0;
    for (i = 0; i < NULL_ROWS_INMEM; i++)
	fcb->NULL_ROWS[i] = NULL;

    if (fcb->map_type != CELL_TYPE)
	G_quant_init(&(fcb->quant));

    /* init cell stats */
    /* now works only for int maps */
    if (fcb->map_type == CELL_TYPE)
	if ((fcb->want_histogram = G__.want_histogram))
	    G_init_cell_stats(&fcb->statf);

    /* init range and if map is double/float init d/f_range */
    G_init_range(&fcb->range);

    if (fcb->map_type != CELL_TYPE)
	G_init_fp_range(&fcb->fp_range);

    /* mark file as open for write */
    fcb->open_mode = OPEN_NEW_UNCOMPRESSED;
    fcb->io_error = 0;

    return fd;
}
#endif /* HAVE_GDAL */

static int G__open_raster_new(const char *name, int open_mode,
			      RASTER_MAP_TYPE map_type)
{
    char xname[GNAME_MAX], xmapset[GMAPSET_MAX];
    struct fileinfo *fcb;
    int i, null_fd, fd;
    char *tempname;
    char *map;
    char *mapset;
    const char *cell_dir;
    int nbytes;

    switch (map_type) {
    case CELL_TYPE:
	cell_dir = "cell";
	nbytes = G__.nbytes;
	break;
    case FCELL_TYPE:
	nbytes = XDR_FLOAT_NBYTES;
	cell_dir = "fcell";
	break;
    case DCELL_TYPE:
	nbytes =  XDR_DOUBLE_NBYTES;
	cell_dir = "fcell";
	break;
    default:
	G_fatal_error(_("Invalid map type <%d>"), map_type);
	break;
    }

    if (G__unqualified_name(name, G_mapset(), xname, xmapset) < 0)
	G_fatal_error(_("Raster map <%s> is not in the current mapset (%s)"),
		      name, G_mapset());
    map = G_store(xname);
    mapset = G_store(xmapset);

    /* check for legal grass name */
    if (G_legal_filename(map) < 0) {
	G_warning(_("<%s> is an illegal file name"), map);
	return -1;
    }

    /* make sure window is set */
    G__init_window();

#ifdef HAVE_GDAL
    if (G_find_file2("", "GDAL", G_mapset()))
	return G__open_raster_new_gdal(map, mapset, map_type);
#endif

    /* open a tempfile name */
    tempname = G_tempfile();
    fd = creat(tempname, 0666);
    if (fd < 0) {
	G_warning(_("G__open_raster_new(): no temp files available"));
	G_free(mapset);
	G_free(tempname);
	G_free(map);
	return -1;
    }

    fcb = new_fileinfo(fd);
    /*
     * since we are bypassing the normal open logic
     * must create the cell element 
     */
    G__make_mapset_element(cell_dir);

    /* mark closed */
    fcb->map_type = map_type;
    fcb->open_mode = -1;
    fcb->gdal = NULL;

    /* for writing fcb->data is allocated to be G__.window.cols * 
       sizeof(CELL or DCELL or FCELL)  */
    fcb->data = (unsigned char *)G_calloc(G__.window.cols,
					  G_raster_size(fcb->map_type));

    /*
     * copy current window into cell header
     * set format to cell/supercell
     * for compressed writing
     *   allocate space to hold the row address array
     */
    G_copy((char *)&fcb->cellhd, (char *)&G__.window, sizeof(fcb->cellhd));

    if (open_mode == OPEN_NEW_COMPRESSED && fcb->map_type == CELL_TYPE) {
	fcb->row_ptr = G_calloc(fcb->cellhd.rows + 1, sizeof(off_t));
	G_zero(fcb->row_ptr, (fcb->cellhd.rows + 1) * sizeof(off_t));
	G__write_row_ptrs(fd);
	fcb->cellhd.compressed = G__.compression_type;

	fcb->nbytes = 1;	/* to the minimum */
    }
    else {
	fcb->nbytes = nbytes;
	if (open_mode == OPEN_NEW_COMPRESSED) {
	    fcb->row_ptr = G_calloc(fcb->cellhd.rows + 1, sizeof(off_t));
	    G_zero(fcb->row_ptr, (fcb->cellhd.rows + 1) * sizeof(off_t));
	    G__write_row_ptrs(fd);
	    fcb->cellhd.compressed = G__.compression_type;
	}
	else
	    fcb->cellhd.compressed = 0;

	if (fcb->map_type != CELL_TYPE) {
	    G_quant_init(&(fcb->quant));
	}
    }

    /* save name and mapset, and tempfile name */
    fcb->name = map;
    fcb->mapset = mapset;
    fcb->temp_name = tempname;

    /* next row to be written (in order) is zero */
    fcb->cur_row = 0;

    /* open a null tempfile name */
    tempname = G_tempfile();
    null_fd = creat(tempname, 0666);
    if (null_fd < 0) {
	G_warning(_("G__open_raster_new(): no temp files available"));
	G_free(tempname);
	G_free(fcb->name);
	G_free(fcb->mapset);
	G_free(fcb->temp_name);
	close(fd);
	return -1;
    }

    fcb->null_temp_name = tempname;
    close(null_fd);

    /* next row to be written (in order) is zero */
    fcb->null_cur_row = 0;

    /* allocate null bitstream buffers for writing */
    for (i = 0; i < NULL_ROWS_INMEM; i++)
	fcb->NULL_ROWS[i] = G__allocate_null_bits(fcb->cellhd.cols);
    fcb->min_null_row = (-1) * NULL_ROWS_INMEM;

    /* init cell stats */
    /* now works only for int maps */
    if (fcb->map_type == CELL_TYPE)
	if ((fcb->want_histogram = G__.want_histogram))
	    G_init_cell_stats(&fcb->statf);

    /* init range and if map is double/float init d/f_range */
    G_init_range(&fcb->range);

    if (fcb->map_type != CELL_TYPE)
	G_init_fp_range(&fcb->fp_range);

    /* mark file as open for write */
    fcb->open_mode = open_mode;
    fcb->io_error = 0;

    return fd;
}

/*!
  \brief Set raster map floating-point data format.
  
  This controls the storage type for floating-point maps. It affects
  subsequent calls to G_open_fp_map_new(). The <em>type</em>
  must be one of FCELL_TYPE (float) or DCELL_TYPE
  (double). The use of this routine by applications is discouraged
  since its use would override user preferences.
  
  \param type raster data type

  \return 1 on success
  \return -1 on error
*/
int G_set_fp_type(RASTER_MAP_TYPE map_type)
{
    switch (map_type) {
    case FCELL_TYPE:
    case DCELL_TYPE:
	G__.fp_type = map_type;
	return 1;
    default:
	G_warning(_("G_set_fp_type(): can only be called with FCELL_TYPE or DCELL_TYPE"));
	return -1;
    }
}


#define FORMAT_FILE "f_format"


/*!
  \brief Check if raster map is floating-point
 
  Returns true (1) if raster map <i>name</i> in <i>mapset</i>
  is a floating-point dataset; false(0) otherwise.
 
  \param name map name
  \param mapset mapset name

  \return 1 floating-point
  \return 0 int
*/
int G_raster_map_is_fp(const char *name, const char *mapset)
{
    char path[GPATH_MAX];
    const char *xmapset;

    xmapset = G_find_cell2(name, mapset);
    if (!xmapset) {
	G_warning(_("Unable to find '%s' in '%s'"), name, mapset);
	return -1;
    }
    G__file_name(path, "fcell", name, xmapset);
    if (access(path, 0) == 0)
	return 1;
    G__file_name(path, "g3dcell", name, xmapset);
    if (access(path, 0) == 0)
	return 1;
    
    return 0;
}

/*!
  \brief Determine raster data type
  
  Determines if the raster map is floating point or integer. Returns
  DCELL_TYPE for double maps, FCELL_TYPE for float maps, CELL_TYPE for 
  integer maps, -1 if error has occured

  \param name map name 
  \param mapset mapset where map <i>name</i> lives

  \return raster data type
*/
RASTER_MAP_TYPE G_raster_map_type(const char *name, const char *mapset)
{
    char path[GPATH_MAX];
    const char *xmapset;

    xmapset = G_find_cell2(name, mapset);
    if (!xmapset) {
	if (mapset && *mapset)
	    G_warning(_("Raster map <%s> not found in mapset <%s>"), name, mapset);
	else
	    G_warning(_("Raster map <%s> not found"), name);
	return -1;
    }
    G__file_name(path, "fcell", name, xmapset);

    if (access(path, 0) == 0)
	return G__check_fp_type(name, xmapset);

    G__file_name(path, "g3dcell", name, xmapset);

    if (access(path, 0) == 0)
	return DCELL_TYPE;

    return CELL_TYPE;
}

/*!
  \brief Determine raster type from descriptor
  
  Determines if the raster map is floating point or integer. Returns
  DCELL_TYPE for double maps, FCELL_TYPE for float maps, CELL_TYPE for
  integer maps, -1 if error has occured
  
  \param fd file descriptor

  \return raster data type
 */
RASTER_MAP_TYPE G_get_raster_map_type(int fd)
{
    struct fileinfo *fcb = &G__.fileinfo[fd];

    return fcb->map_type;
}

/*!
  \brief Determines whether the floating points cell file has double or float type

  \param name map name
  \param mapset mapset where map <i>name</i> lives
  
  \return raster type (fcell, dcell)
*/
RASTER_MAP_TYPE G__check_fp_type(const char *name, const char *mapset)
{
    char path[GPATH_MAX];
    struct Key_Value *format_keys;
    int in_stat;
    const char *str, *str1;
    RASTER_MAP_TYPE map_type;
    const char *xmapset;

    xmapset = G_find_cell2(name, mapset);
    if (!xmapset) {
	G_warning(_("Unable to find '%s' in '%s'"), name, mapset);
	return -1;
    }
    G__file_name_misc(path, "cell_misc", FORMAT_FILE, name, xmapset);

    if (access(path, 0) != 0) {
	G_warning(_("Unable to find '%s'"), path);
	return -1;
    }
    format_keys = G_read_key_value_file(path, &in_stat);
    if (in_stat != 0) {
	G_warning(_("Unable to open '%s'"), path);
	return -1;
    }
    if ((str = G_find_key_value("type", format_keys)) != NULL) {
	if (strcmp(str, "double") == 0)
	    map_type = DCELL_TYPE;
	else if (strcmp(str, "float") == 0)
	    map_type = FCELL_TYPE;
	else {
	    G_warning(_("Invalid type: field '%s' in file '%s'"),
		      str, path);
	    G_free_key_value(format_keys);
	    return -1;
	}
    }
    else {
	G_free_key_value(format_keys);
	return -1;
    }

    if ((str1 = G_find_key_value("byte_order", format_keys)) != NULL) {
	if (strcmp(str1, "xdr") != 0)
	    G_warning(_("Raster map <%s> is not xdr: byte_order: %s"),
			name, str);
	/* here read and translate  byte order if not using xdr */
    }
    G_free_key_value(format_keys);
    return map_type;
}

/*!
  \brief Opens a new raster map

  Opens a new raster map of type <i>wr_type</i>

  See warnings and notes for G_open_cell_new().

  Supported data types:
   - CELL_TYPE
   - FCELL_TYPE
   - DCELL_TYPE
  
  On CELL_TYPE calls G_open_cell_new() otherwise G_open_fp_cell_new().
 
  \param name map name
  \param wr_type raster data type

  \return nonnegative file descriptor (int)
  \return -1 on error
*/
int G_open_raster_new(const char *name, RASTER_MAP_TYPE wr_type)
{
    return G__open_raster_new(name, OPEN_NEW_COMPRESSED, wr_type);
}

/*!
  \brief Opens a new raster map (uncompressed)

  See G_open_raster_new().

  \param name map name
  \param wr_type raster data type

  \return nonnegative file descriptor (int)
  \return -1 on error
*/
int G_open_raster_new_uncompressed(const char *name, RASTER_MAP_TYPE wr_type)
{
    return G__open_raster_new(name, OPEN_NEW_UNCOMPRESSED, wr_type);
}

/*!
  \brief Sets quant translation rules for raster map opened for
  reading.

  Returned by G_open_cell_old(). After calling this function,
  G_get_c_raster_row() and G_get_map_row() will use rules defined by q
  (instead of using rules defined in map's quant file) to convert floats to
  ints.
  
  \param fd file descriptor (cell file)
  \param q pointer to Quant structure

  \return 0 success
  \return -1 failure
*/
int G_set_quant_rules(int fd, struct Quant *q)
{
    struct fileinfo *fcb = &G__.fileinfo[fd];
    CELL cell;
    DCELL dcell;
    struct Quant_table *p;

    if (fcb->open_mode != OPEN_OLD) {
	G_warning(_("G_set_quant_rules() can be called only for "
		    "raster maps opened for reading"));
	return -1;
    }
    /* copy all info from q to fcb->quant) */
    G_quant_init(&fcb->quant);
    if (q->truncate_only) {
	G_quant_truncate(&fcb->quant);
	return 0;
    }
    for (p = &(q->table[q->nofRules - 1]); p >= q->table; p--)
	G_quant_add_rule(&fcb->quant, p->dLow, p->dHigh, p->cLow, p->cHigh);
    if (G_quant_get_neg_infinite_rule(q, &dcell, &cell) > 0)
	G_quant_set_neg_infinite_rule(&fcb->quant, dcell, cell);
    if (G_quant_get_pos_infinite_rule(q, &dcell, &cell) > 0)
	G_quant_set_pos_infinite_rule(&fcb->quant, dcell, cell);

    return 0;
}
