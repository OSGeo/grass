/**********************************************************
 *
 *  G_open_cell_old (name, mapset)
 *      char *name            map file name
 *      char *mapset          mapset containing map "name"
 *
 *  opens the existing cell file 'name' in the 'mapset'
 *  for reading by G_get_map_row()
 *  with mapping into the current window
 *
 *  returns: open file descriptor ( >= 0) if successful
 *           negative integer if error
 *
 *  diagnostics: warning message printed if open fails
 ***********************************************************
 *
 *  G_open_cell_new (name)
 *      char *name            map file name
 *
 *  opens a new cell file 'name' in the current mapset
 *  for writing by G_put_map_row()
 *
 *  the file is created and filled with no data
 *  it is assumed that the new cell file is to conform to
 *  the current window.
 *
 *  The file must be written sequentially.
 *   (Use G_open_cell_new_random() for non sequential writes)
 *
 *  note: the open actually creates a temporary file
 *        G_close_cell() will move the temporary file
 *        to the cell file and write out the necessary 
 *        support files (cellhd, cats, hist, etc.)
 *
 *  returns: open file descriptor ( >= 0) if successful
 *           negative integer if error
 *
 *  diagnostics: warning message printed if open fails
 *
 *  warning: calls to G_set_window() made after opening a new
 *           cell file may create confusion and should be avoided
 *           the new cell file will be created to conform
 *           to the window at the time of the open.
 ***********************************************************
 *
 *  G_open_fp_cell_new (name)
 *      char *name            map file name
 *
 *  opens a new floating-point map "name" in the current mapset for writing. The type
 *  of the file (i.e. either double or float) is determined and fixed at
 *  this point. The default is FCELL_TYPE. In order to change this default
 *  USE G_set_fp_type(type) where type is one of DCELL_TYPE or FCELL_TYPE..
 *  see warnings and notes for G_open_cell_new
 *
 ***********************************************************
 *
 *  G_open_raster_new(char *name, RASTER_MAP_TYPE map_type
 *
 *  opens a new raster map of type map_type
 *  see warnings and notes for G_open_cell_new
 *
 ***********************************************************
 *
 * RASTER_MAP_TYPE G_raster_map_type(name, mapset)
 * returns the type of raster map: DCELL_TYPE, FCELL_TYPE or CELL_TYPE
 *
 ***********************************************************
 *
 * 
 * int G_raster_map_is_fp(name, set)
 * returns 1 if map is float or double, 0 otherwise
 *
 ***********************************************************
 * G_set_cell_format(n)
 *
 * sets the format for subsequent opens on new integer cell files
 * (uncompressed and random only)
 * warning - subsequent put_row calls will only write n+1 bytes
 *           per cell. If the data requires more, the cell file
 *           will be written incorrectly (but with n+1 bytes per cell)
 *
 ***********************************************************
 * G_set_quant_rules(int fd, struct Quant q)
 *
 * Sets quant translation rules for raster map opened for reading.
 * fd is a file descriptor returned by G_open_cell_old().
 * After calling this function, G_get_c_raster_row() and
 * G_get_map_row() will use rules defined by q (instead of using
 * rules defined in map's quant file) to convert floats to ints.
 *
 ***********************************************************
 * G_want_histogram(flag)
 *
 * If newly created cell files should have histograms, set flag=1
 * otherwise set flag=0. Applies to subsequent opens.
 *
 ***********************************************************/

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

static int allocate_compress_buf(int);

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
    for (i = oldsize; i < newsize; i++)
	G__.fileinfo[i].open_mode = -1;

    G__.fileinfo_count = newsize;

    return &G__.fileinfo[fd];
}

/*!
 * \brief open an existing raster map
 *
 *  This routine opens the raster map <b>name</b> in
 * <b>mapset</b> for reading. A nonnegative file descriptor is returned if the
 * open is successful. Otherwise a diagnostic message is printed and a negative
 * value is returned. This routine does quite a bit of work. Since GRASS users
 * expect that all raster maps will be resampled into the current region, the
 * resampling index for the raster map is prepared by this routine after the
 * file is opened. The resampling is based on the active module
 * region.\remarks{See also The_Region.} Preparation required for
 * reading the various raster file formats\remarks{See
 * Raster_File_Format for an explanation of the various raster file
 * formats.} is also done.
 *
 *  \param name
 *  \param mapset
 *  \return int
 */

 
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


static int G__open_raster_new(const char *name, int open_mode);

int G_open_cell_old (
    const char *name,
    const char *mapset)
{
    int fd;

    if ((fd = G__open_cell_old (name, mapset)) < 0)
    {
        G_warning (_("unable to open raster map [%s in %s]"),
            name, mapset);
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


/********************************************************************
 * G__open_cell_old (name, mapset)
 *
 * function:
 *   This is the work horse. It is used to open cell files, supercell
 *   files, and the MASK file. 
 *
 * parms:
 *   name, mapset    name and mapset of cell file to be opened.
 *
 * actions:
 *   opens the named cell file, following reclass reference if
 *     named layer is a reclass layer.
 *   creates the required mapping between the data and the window
 *     for use by the get_map_row family of routines.
 *
 * returns:
 *   open file descriptor or -1 if error.
 *
 * diagnostics:
 *   errors other than actual open failure will cause a diagnostic to be
 *   delivered thru G_warning() open failure messages are left to the
 *   calling routine since the masking logic will want to issue a different
 *   warning.
 *
 * note:
 *  this routine does NOT open the MASK layer. If it did we would get
 *  infinite recursion.  This routine is called to open the mask by
 *  G__check_for_auto_masking() which is called by G_open_cell().
 ***********************************************************************/
int G__open_cell_old (
    const char *name,
    const char *mapset)
{
    struct fileinfo *fcb;
    int fd;
    char cell_dir[100];
    const char *r_name ;
    const char *r_mapset ;
    struct Cell_head cellhd ;
    int CELL_nbytes = 0;                   /* bytes per cell in CELL map */
    int INTERN_SIZE;
    int reclass_flag, i;
    int MAP_NBYTES;
    RASTER_MAP_TYPE MAP_TYPE;
    struct Reclass reclass;

/* make sure window is set    */
    G__init_window ();

/* Check for reclassification */
    reclass_flag = G_get_reclass (name, mapset, &reclass) ;

    switch (reclass_flag)
    {
        case 0:
            r_name = name ;
            r_mapset = mapset ;
            break ;
        case 1:
            r_name = reclass.name ;
            r_mapset = reclass.mapset ;
	    if (G_find_cell2 (r_name, r_mapset) == NULL)
	    {
		G_warning (
		    _("unable to open [%s] in [%s] since it is a reclass of [%s] in [%s] which does not exist"),
		    name,mapset,r_name,r_mapset);
		return -1;
	    }
            break ;
        default:           /* Error reading cellhd/reclass file */
            return -1 ;
    }

/* read the cell header */
    if(G_get_cellhd (r_name, r_mapset, &cellhd) < 0)
        return -1;

    /* now check the type */
    MAP_TYPE = G_raster_map_type (r_name, r_mapset);
    if (MAP_TYPE < 0) return -1;

    if(MAP_TYPE == CELL_TYPE)
    /* set the number of bytes for CELL map */
    {
       CELL_nbytes = cellhd.format + 1;
       if (CELL_nbytes < 1)
       {
           G_warning(_("[%s] in mapset [%s]-format field in header file invalid"),
               r_name, r_mapset);
           return -1;
       }
    }

    if (cellhd.proj != G__.window.proj)
    {
        G_warning (
            _("[%s] in mapset [%s] - in different projection than current region:\n found map [%s] in: <%s>, should be <%s> "),
            name, mapset, name, G__projection_name(cellhd.proj), G__projection_name(G__.window.proj));
        return -1;
    }
    if (cellhd.zone != G__.window.zone)
    {
        G_warning (
            _("[%s] in mapset [%s] - in different zone [%d] than current region [%d]"),
            name, mapset, cellhd.zone, G__.window.zone);
        return -1;
    }

/* when map is int warn if too large cell size */
    if (MAP_TYPE == CELL_TYPE && CELL_nbytes > sizeof(CELL))
    {
        G_warning ( _("[%s] in [%s] - bytes per cell (%d) too large"),
            name, mapset, CELL_nbytes);
	return -1;
    }

/* record number of bytes per cell */
    if(MAP_TYPE == FCELL_TYPE)
    {
      strcpy(cell_dir, "fcell");
      INTERN_SIZE = sizeof(FCELL);
      MAP_NBYTES = XDR_FLOAT_NBYTES;
    }
    else
      if (MAP_TYPE == DCELL_TYPE)
      {
         strcpy(cell_dir, "fcell");
         INTERN_SIZE = sizeof(DCELL);
         MAP_NBYTES = XDR_DOUBLE_NBYTES;
      }
      else /* integer */
      {
         strcpy(cell_dir, "cell");
         INTERN_SIZE = sizeof(CELL);
         MAP_NBYTES = CELL_nbytes;
      }

/* now actually open file for reading */
    fd = G_open_old (cell_dir, r_name, r_mapset);
    if (fd < 0)
        return -1;

    fcb = new_fileinfo(fd);

    fcb->map_type = MAP_TYPE;

/* Save cell header */
    G_copy ((char *) &fcb->cellhd, (char *) &cellhd, sizeof(cellhd));

    /* allocate null bitstream buffers for reading null rows */
    for (i=0;i< NULL_ROWS_INMEM; i++)
       fcb->NULL_ROWS[i] = G__allocate_null_bits(G__.window.cols);
    fcb->null_work_buf = G__allocate_null_bits(fcb->cellhd.cols);
    /* initialize : no NULL rows in memory */
    fcb->min_null_row = (-1) * NULL_ROWS_INMEM; 

/* mark closed */
    fcb->open_mode = -1;

/* save name and mapset */
    {
    char xname[GNAME_MAX], xmapset[GMAPSET_MAX];
    if (G__name_is_fully_qualified(name, xname, xmapset))
	fcb->name   = G_store (xname);
    else
	fcb->name   = G_store (name);
    }
    fcb->mapset = G_store (mapset);

/* mark no data row in memory  */
    fcb->cur_row = -1;
/* fcb->null_cur_row is not used for reading, only for writing */
    fcb->null_cur_row = -1;

/* if reclass, copy reclass structure */
    if ((fcb->reclass_flag = reclass_flag))
	G_copy (&fcb->reclass, &reclass, sizeof(reclass));

/* check for compressed data format, making initial reads if necessary */
    if(G__check_format (fd) < 0)
    {
        close (fd); /* warning issued by check_format() */
        return -1;
    }

/* create the mapping from cell file to window */
    G__create_window_mapping (fd);

/*
 * allocate the data buffer
 * number of bytes per cell is cellhd.format+1
 */

    /* for reading fcb->data is allocated to be fcb->cellhd.cols * fcb->nbytes 
      (= XDR_FLOAT/DOUBLE_NBYTES) and G__.work_buf to be G__.window.cols * 
      sizeof(CELL or DCELL or FCELL) */
    fcb->data = (unsigned char *) G_calloc (fcb->cellhd.cols, MAP_NBYTES);
    
    G__reallocate_work_buf(INTERN_SIZE);
    G__reallocate_mask_buf();
    G__reallocate_null_buf();
    G__reallocate_temp_buf();
    /* work_buf is used as intermediate buf for conversions */
/*
 * allocate/enlarge the compressed data buffer needed by get_map_row()
 */
    allocate_compress_buf (fd);

/* initialize/read in quant rules for float point maps */
    if(fcb->map_type != CELL_TYPE)
    {
     if (fcb->reclass_flag)
       G_read_quant (fcb->reclass.name, fcb->reclass.mapset, &(fcb->quant));
     else
       G_read_quant (fcb->name, fcb->mapset, &(fcb->quant));
    }

/* now mark open for read: this must follow create_window_mapping() */
    fcb->open_mode = OPEN_OLD;
    fcb->io_error = 0;
    fcb->map_type = MAP_TYPE;
    fcb->nbytes = MAP_NBYTES;
    fcb->null_file_exists = -1;

    if(fcb->map_type != CELL_TYPE)
        	xdrmem_create (&fcb->xdrstream, (caddr_t) fcb->data, 
            (u_int) (fcb->nbytes * fcb->cellhd.cols), XDR_DECODE);

    return fd;
}

/*****************************************************************/

static int WRITE_NBYTES = sizeof(CELL);
/* bytes per cell for current map */

static int NBYTES = sizeof(CELL);
/* bytes per cell for writing integer maps */

static RASTER_MAP_TYPE WRITE_MAP_TYPE = CELL_TYPE;
/* a type of current map */

static int COMPRESSION_TYPE = 0;

#define FP_NBYTES G__.fp_nbytes
/* bytes per cell for writing floating point maps */
#define FP_TYPE  G__.fp_type
/* a type of floating maps to be open */
static int FP_TYPE_SET=0;  /* wether or not the f.p. type was set explicitly
			 by calling G_set_fp_type() */

static char cell_dir[100];
/* The mapset element for the raster map to be open: fcell, or cell */


int G_open_cell_new (const char *name)
{
    WRITE_MAP_TYPE = CELL_TYPE;
    strcpy(cell_dir, "cell");
    /* bytes per cell for current map */
    WRITE_NBYTES = NBYTES;
    return G__open_raster_new (name, OPEN_NEW_COMPRESSED);
}

int G_open_cell_new_random (const char *name)
{
    WRITE_MAP_TYPE = CELL_TYPE;
    /* bytes per cell for current map */
    WRITE_NBYTES = NBYTES;
    strcpy(cell_dir, "cell");
    return G__open_raster_new (name, OPEN_NEW_RANDOM);
}

int G_open_cell_new_uncompressed (const char *name)
{
    WRITE_MAP_TYPE = CELL_TYPE; /* a type of current map */
    strcpy(cell_dir, "cell");
    /* bytes per cell for current map */
    WRITE_NBYTES = NBYTES;
    return G__open_raster_new (name, OPEN_NEW_UNCOMPRESSED);
}

int G_want_histogram (int flag)
{
    G__.want_histogram = flag;

    return 0;
}

/* when writing float map: format is -1  */
int G_set_cell_format ( int n)
/* sets the format for integer raster map */
{
    if(WRITE_MAP_TYPE == CELL_TYPE)
    {
        NBYTES = n+1;
        if (NBYTES <= 0)
    	    NBYTES = 1;
        if (NBYTES > sizeof(CELL))
    	NBYTES = sizeof(CELL);
    }

    return 0;
}

int G_cellvalue_format (CELL v)
{
    int i;
    if (v >= 0)
	for (i = 0; i < sizeof(CELL); i++)
	    if (!(v /= 256))
		return i;
    return sizeof(CELL)-1;
}

int G_open_fp_cell_new (const char *name)
{
    /* use current float. type for writing float point maps */
    /* if the FP type was NOT explicitly set by G_set_fp_type()
       use environment variable */
    if(!FP_TYPE_SET)
    {
       if (getenv("GRASS_FP_DOUBLE"))
       {
          FP_TYPE = DCELL_TYPE;
          FP_NBYTES = XDR_DOUBLE_NBYTES;
       }
       else
       {
          FP_TYPE = FCELL_TYPE;
          FP_NBYTES = XDR_FLOAT_NBYTES;
       }
    }
    WRITE_MAP_TYPE = FP_TYPE;
    WRITE_NBYTES = FP_NBYTES;
     
    strcpy(cell_dir, "fcell");
    return G__open_raster_new (name, OPEN_NEW_COMPRESSED);
}	

int 
G_open_fp_cell_new_uncompressed (const char *name)
{
    /* use current float. type for writing float point maps */
    if(!FP_TYPE_SET)
    {
       if (getenv("GRASS_FP_DOUBLE"))
       {
          FP_TYPE = DCELL_TYPE;
          FP_NBYTES = XDR_DOUBLE_NBYTES;
       }
       else
       {
          FP_TYPE = FCELL_TYPE;
          FP_NBYTES = XDR_FLOAT_NBYTES;
       }
    }
    WRITE_MAP_TYPE = FP_TYPE;
    WRITE_NBYTES = FP_NBYTES;
     
    strcpy(cell_dir, "fcell");
    return G__open_raster_new (name, OPEN_NEW_UNCOMPRESSED);
}	

static int
clean_check_raster_name (const char *inmap, char **outmap, char **outmapset)
{
	/* Remove mapset part of name if exists.  Also, if mapset
	 * part exists, make sure it matches current mapset.
	 */
	int status = 0;
	char *ptr;
	char *buf;

	buf = G_store (inmap);
	if ((ptr = strpbrk (buf, "@")) != NULL)
	{
		*ptr = '\0';
		ptr++;
		*outmapset = G_store(G_mapset());
		if ((status = strcmp(ptr, *outmapset)))
		{
			G_free (buf);
			G_free (*outmapset);
		}
		else
		{
			*outmap = G_store (buf);
			G_free (buf);
		}
	}
	else
	{
		*outmap = buf;
		*outmapset = G_store(G_mapset());
	}
	return status;
}
	
/* opens a f-cell or cell file depending on WRITE_MAP_TYPE */
static int G__open_raster_new (const char *name, int open_mode)
{
    char xname[GNAME_MAX], xmapset[GMAPSET_MAX];
    struct fileinfo *fcb;
    int i, null_fd, fd;
    char *tempname;
    char *map;
    char *mapset;

/* check for fully-qualfied name */
    if (G__name_is_fully_qualified(name, xname, xmapset))
    {
	if (strcmp(xmapset, G_mapset()) != 0)
	    G_fatal_error ("%s is not in the current mapset (%s)", name, G_mapset());
	name = xname;
    }

/* check for legal grass name */
    if (G_legal_filename (name) < 0)
    {
	G_warning (_("opencell: %s - illegal file name"), name);
	return -1;
    }
    
    if(clean_check_raster_name (name, &map, &mapset) != 0)
    {
	    G_warning ("opencell: %s - bad mapset", name);
	    return -1;
    }

/* make sure window is set */
    G__init_window();

/* open a tempfile name */
    tempname = G_tempfile ();
    fd = creat (tempname, 0666);
    if (fd < 0)
    {   
        G_warning ("G__open_raster_new: no temp files available");
        G_free (tempname);
	G_free (map);
	G_free (mapset);
        return -1;
    }

    fcb = new_fileinfo(fd);
/*
 * since we are bypassing the normal open logic
 * must create the cell element 
 */
    G__make_mapset_element (cell_dir);

/* mark closed */
    fcb->map_type = WRITE_MAP_TYPE;
    fcb->open_mode = -1;

    /* for writing fcb->data is allocated to be G__.window.cols * 
       sizeof(CELL or DCELL or FCELL) and G__.work_buf to be G__.window.cols *
       fcb->nbytes (= XDR_FLOAT/DOUBLE_NBYTES) */
    fcb->data = (unsigned char *) G_calloc (G__.window.cols, 
		  G_raster_size(fcb->map_type));

    G__reallocate_null_buf();
    /* we need null buffer to automatically write embeded nulls in put_row */

    if (open_mode == OPEN_NEW_COMPRESSED && !COMPRESSION_TYPE)
	COMPRESSION_TYPE = getenv("GRASS_INT_ZLIB") ? 2 : 1;

/*
 * copy current window into cell header
 * set format to cell/supercell
 * for compressed writing
 *   allocate space to hold the row address array
 *   allocate/enlarge both the compress_buf and the work_buf
 */
    G_copy ((char *) &fcb->cellhd, (char *) &G__.window, sizeof (fcb->cellhd));

    if (open_mode == OPEN_NEW_COMPRESSED && fcb->map_type == CELL_TYPE)
    {
	fcb->row_ptr = G_calloc(fcb->cellhd.rows + 1, sizeof(off_t)) ;
	G_zero(fcb->row_ptr,(fcb->cellhd.rows + 1) * sizeof(off_t)) ;
	G__write_row_ptrs (fd);
	fcb->cellhd.compressed = COMPRESSION_TYPE;

	allocate_compress_buf(fd);
	fcb->nbytes = 1;		/* to the minimum */
        G__reallocate_work_buf(sizeof(CELL));
        G__reallocate_mask_buf();
        G__reallocate_temp_buf();
    }
    else
    {
        fcb->nbytes = WRITE_NBYTES ;
        if(open_mode == OPEN_NEW_COMPRESSED)
        {
	      fcb->row_ptr = G_calloc(fcb->cellhd.rows + 1, sizeof(off_t)) ;
      	      G_zero(fcb->row_ptr,(fcb->cellhd.rows + 1) * sizeof(off_t)) ;
	      G__write_row_ptrs (fd);
	      fcb->cellhd.compressed = COMPRESSION_TYPE;
        }
        else
	      fcb->cellhd.compressed = 0;
        G__reallocate_work_buf(fcb->nbytes);
        G__reallocate_mask_buf();
        G__reallocate_temp_buf();

        if(fcb->map_type != CELL_TYPE)
        {
             G_quant_init (&(fcb->quant));
        }

	if (open_mode == OPEN_NEW_RANDOM)
        {
            G_warning(_("Can't write embedded null values for map open for random access"));
            if(fcb->map_type == CELL_TYPE)
                    G_write_zeros (fd, (long) WRITE_NBYTES * fcb->cellhd.cols * fcb->cellhd.rows);
            else if(fcb->map_type == FCELL_TYPE)
            {
                    if (G__random_f_initialize_0 (fd, fcb->cellhd.rows, fcb->cellhd.cols)<0)
                          return -1;
            }
            else
            {
                    if (G__random_d_initialize_0 (fd, fcb->cellhd.rows, fcb->cellhd.cols)<0)
                          return -1;
            }
        }
    }

/* save name and mapset, and tempfile name */
    fcb->name      = map;
    fcb->mapset    = mapset;
    fcb->temp_name = tempname;

/* next row to be written (in order) is zero */
    fcb->cur_row = 0;

/* open a null tempfile name */
    tempname = G_tempfile ();
    null_fd = creat (tempname, 0666);
    if (null_fd < 0)
    {   
        G_warning ("opencell opening temp null file: no temp files available");
        G_free (tempname);
	G_free (fcb->name);
	G_free (fcb->mapset);
	G_free (fcb->temp_name);
	close (fd);
        return -1;
    }

    fcb->null_temp_name = tempname;
    close(null_fd);

/* next row to be written (in order) is zero */
    fcb->null_cur_row = 0;

    /* allocate null bitstream buffers for writing */
    for (i=0;i< NULL_ROWS_INMEM; i++)
       fcb->NULL_ROWS[i] = G__allocate_null_bits(fcb->cellhd.cols);
    fcb->min_null_row = (-1) * NULL_ROWS_INMEM;
    fcb->null_work_buf = G__allocate_null_bits(fcb->cellhd.cols);

/* init cell stats */
/* now works only for int maps */
if(fcb->map_type == CELL_TYPE)
    if ((fcb->want_histogram = G__.want_histogram))
	G_init_cell_stats (&fcb->statf);

/* init range and if map is double/float init d/f_range */
    G_init_range (&fcb->range);

    if(fcb->map_type != CELL_TYPE)
        G_init_fp_range (&fcb->fp_range);
  
/* mark file as open for write */
    fcb->open_mode = open_mode;
    fcb->io_error = 0;

    return fd;
}
/*
 * allocate/enlarge the compressed data buffer needed by get_map_row()
 * and put_map_row()
 * note: compressed format is repeat, value:
 *  repeat takes 1 byte, value takes up to sizeof(CELL)
 *  plus 1 byte header for nbytes needed to store row
 */
static int allocate_compress_buf(int fd)
{
    struct fileinfo *fcb = &G__.fileinfo[fd];
    int n;
    n = fcb->cellhd.cols * (sizeof(CELL) + 1) + 1;
    if (fcb->cellhd.compressed && fcb->map_type == CELL_TYPE && (n > G__.compressed_buf_size))
    {
        if (G__.compressed_buf_size <= 0)
            G__.compressed_buf = (unsigned char *) G_malloc (n);
        else
            G__.compressed_buf = (unsigned char *) G_realloc((char *) G__.compressed_buf,n);
        G__.compressed_buf_size  = n;
    }

    return 0;
}
/*
 * allocate/enlarge the work data buffer needed by get_map_row and put_map_row()
 */
int G__reallocate_work_buf (int bytes_per_cell)
{
    int n;
    n = G__.window.cols * (bytes_per_cell + 1) + 1;
    if (n > G__.work_buf_size)
    {
        if (G__.work_buf_size <= 0)
            G__.work_buf = (unsigned char *) G_malloc (n);
        else
            G__.work_buf = (unsigned char *) G_realloc((char *) G__.work_buf,n);
        G__.work_buf_size  = n;
    }

    return 0;
}

/*
 * allocate/enlarge the null data buffer needed by get_map_row()
 * and for conversion in put_row 
 */
int G__reallocate_null_buf (void)
{
    int n;
    n = (G__.window.cols + 1) * sizeof(char);
    if (n > G__.null_buf_size)
    {
        if (G__.null_buf_size <= 0)
            G__.null_buf = (char *) G_malloc (n);
        else
            G__.null_buf = (char *) G_realloc(G__.null_buf,n);
        G__.null_buf_size  = n;
    }

    return 0;
}

/*
 * allocate/enlarge the mask buffer needed by get_map_row()
 */
int G__reallocate_mask_buf (void)
{
    int n;
    n = (G__.window.cols + 1) * sizeof(CELL);
    if (n > G__.mask_buf_size)
    {
        if (G__.mask_buf_size <= 0)
            G__.mask_buf = (CELL *) G_malloc (n);
        else
            G__.mask_buf = (CELL *) G_realloc((char *) G__.mask_buf,n);
        G__.mask_buf_size  = n;
    }

    return 0;
}

/*
 * allocate/enlarge the temporary buffer needed by G_get_raster_row[_nomask]
 */
int G__reallocate_temp_buf (void)
{
    int n;
    n = (G__.window.cols + 1) * sizeof(CELL);
    if (n > G__.temp_buf_size)
    {
        if (G__.temp_buf_size <= 0)
            G__.temp_buf = (CELL *) G_malloc (n);
        else
            G__.temp_buf = (CELL *) G_realloc((char *) G__.temp_buf,n);
        G__.temp_buf_size  = n;
    }

    return 0;
}


/*!
 * \brief 
 *
 * This controls the storage type for floating-point maps. It affects
 * subsequent calls to <tt>G_open_fp_map_new()</tt>. The <em>type</em>
 * must be one of <tt>FCELL_TYPE</tt> (float) or <tt>DCELL_TYPE</tt>
 * (double). The use of this routine by applications is discouraged
 * since its use would override user preferences.
 *
 *  \param type
 *  \return int
 */
int G_set_fp_type (RASTER_MAP_TYPE map_type)
{
    FP_TYPE_SET = 1;
    if (map_type!=FCELL_TYPE && map_type != DCELL_TYPE) 
    {
        G_warning("G_set_fp_type() can only be called with FCELL_TYPE or DCELL_TYPE");
        return -1;
    }
    FP_TYPE = map_type;
    if(map_type == DCELL_TYPE)
         FP_NBYTES = XDR_DOUBLE_NBYTES;
    else
         FP_NBYTES = XDR_FLOAT_NBYTES;

    return 1;
}


#define FORMAT_FILE "f_format"


/*!
 * \brief 
 *
 * Returns true(1) if raster map <em>name</em> in <em>mapset</em>
 * is a floating-point dataset; false(0) otherwise.
 *
 *  \param name
 *  \param mapset
 *  \return int
 */
int G_raster_map_is_fp (const char *name, const char *mapset)
{
   char path[GPATH_MAX];

   if (G_find_cell2 (name, mapset) == NULL)
   {
      G_warning (_("unable to find [%s] in [%s]"),name,mapset);
      return -1;
   }
   G__file_name(path,"fcell", name, mapset);
   if (access(path,0) == 0) return 1;
   G__file_name(path, "g3dcell", name, mapset);
   if (access(path,0) == 0) return 1;
   return 0;
}




/*!
 * \brief Determine raster type
 *
 * Determines if the raster map is floating point or integer. Returns
 * DCELL_TYPE for double maps, FCELL_TYPE for float maps, CELL_TYPE for 
 * integer maps, -1 if error has occured
 *
 * NOTE: If the user specifies a fully qualified raster name which exists,
 * then <i>G_raster_map_type()</i> modifies <b>name</b> by removing the
 * "@<i>mapset</i>". (due to it calling <i>G_find_cell()</i>)
 *
 *  \param name
 *  \param mapset
 *  \return RASTER_MAP_TYPE
 */
RASTER_MAP_TYPE G_raster_map_type (const char *name, const char *mapset)
{
   return G_raster_map_type2(name, mapset);
}



/*!
 * \brief Determine raster type (look but don't touch)
 *
 * Same as G_raster_map_type() but use G_find_cell2() so <b>name</b> is not
 * modified.
 *
 * Determines if the raster map is floating point or integer. Returns
 * DCELL_TYPE for double maps, FCELL_TYPE for float maps, CELL_TYPE for 
 * integer maps, -1 if error has occured
 *
 *  \param name
 *  \param mapset
 *  \return RASTER_MAP_TYPE
 */
RASTER_MAP_TYPE G_raster_map_type2 (const char *name, const char *mapset)
{
   char path[GPATH_MAX];

   if (G_find_cell2 (name, mapset) == NULL)
   {
      G_warning (_("unable to find [%s] in [%s]"),name,mapset);
      return -1;
   }
   G__file_name(path,"fcell", name, mapset);

   if (access(path,0) == 0) return G__check_fp_type(name,mapset);

   G__file_name(path, "g3dcell", name, mapset);

   if (access(path,0) == 0) return DCELL_TYPE;

   return CELL_TYPE;
}



/*!
 * \brief Determine raster type from descriptor
 *
 * Determines if the raster map is floating point or integer. Returns
 * DCELL_TYPE for double maps, FCELL_TYPE for float maps, CELL_TYPE for 
 * integer maps, -1 if error has occured
 *
 *  \param fd
 *  \return RASTER_MAP_TYPE
 */
RASTER_MAP_TYPE G_get_raster_map_type(int fd)
{
    struct fileinfo *fcb = &G__.fileinfo[fd];
    return fcb->map_type;
}



RASTER_MAP_TYPE G__check_fp_type (const char *name, const char *mapset)
/* determines whether the flotsing points cell file has double or float type */
/* returns DCELL_TYPE for double, FCELL_TYPE for float, -1 for error */
/* f_format file:
   type: float/double
   byte_order: xdr/bytes? 
*/

{
   char path[GPATH_MAX];
   struct Key_Value *format_keys;
   int in_stat;
   char *str,*str1;
   RASTER_MAP_TYPE map_type;

   G__file_name_misc(path,"cell_misc",FORMAT_FILE,name,mapset);
     
   if (access(path,0) != 0) {
      G_warning (_("unable to find [%s]"),path);
      return -1;
   }
   format_keys = G_read_key_value_file(path, &in_stat);
   if (in_stat !=0)
   {
      G_warning ( _("Unable to open %s"),path);
      return -1;
   }
   if ((str = G_find_key_value("type",format_keys))!=NULL) {
     G_strip(str);
     if (strcmp(str,"double") == 0) map_type = DCELL_TYPE;
     else 
       if (strcmp(str,"float") == 0) map_type = FCELL_TYPE;
       else {
         G_warning(_("invalid type: field %s in file %s "),str,path);
         G_free_key_value(format_keys);
         return -1;
       }
   }
   else 
   {
       G_free_key_value(format_keys);
       return -1;
   }

   if ((str1 = G_find_key_value("byte_order",format_keys))!=NULL) 
   {
     G_strip(str1);
     if (strcmp(str1,"xdr") != 0)  
         G_warning ("the map %s is not xdr: byte_order: %s",name, str);
         /* here read and translate  byte order if not using xdr */
   }
   G_free_key_value(format_keys);
   return map_type;
} 

int G_open_raster_new (const char *name, RASTER_MAP_TYPE wr_type)
{
    if(wr_type == CELL_TYPE)
       return G_open_cell_new (name);

    G_set_fp_type(wr_type);
    return G_open_fp_cell_new (name);
}

int G_open_raster_new_uncompressed (const char *name, RASTER_MAP_TYPE wr_type)
{
    if(wr_type == CELL_TYPE)
       return G_open_cell_new_uncompressed (name);

    G_set_fp_type(wr_type);
    return G_open_fp_cell_new_uncompressed (name);
}


/*!
 * \brief 
 *
 * Sets quant
 * translation rules for raster map opened for reading. fd is a file descriptor
 * returned by G_open_cell_old(). After calling this function,
 * G_get_c_raster_row() and G_get_map_row() will use rules defined by q
 * (instead of using rules defined in map's quant file) to convert floats to
 * ints.
 *
 *  \param fd
 *  \param q
 *  \return int
 */
int G_set_quant_rules (int fd, struct Quant *q)
{
   struct fileinfo *fcb = &G__.fileinfo[fd];
   CELL cell;
   DCELL dcell;
   struct Quant_table *p;

   if(fcb->open_mode!=OPEN_OLD)
   {
      G_warning("G_set_quant_rules can be called only for raster maps opened for reading");
      return -1;
   }
   /* copy all info from q to fcb->quant) */
   G_quant_init(&fcb->quant);
   if(q->truncate_only) 
   {
      G_quant_truncate (&fcb->quant);
      return 0;
   }
   for (p = &(q->table[q->nofRules - 1]); p >= q->table; p--)
      G_quant_add_rule(&fcb->quant, p->dLow, p->dHigh, p->cLow, p->cHigh);
   if(G_quant_get_neg_infinite_rule (q, &dcell, &cell)>0)
      G_quant_set_neg_infinite_rule (&fcb->quant, dcell, cell);
   if(G_quant_get_pos_infinite_rule (q, &dcell, &cell)>0)
      G_quant_set_pos_infinite_rule (&fcb->quant, dcell, cell);

   return 0;
}
