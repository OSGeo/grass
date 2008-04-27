/***********************************************************************
 *
 *   G_close_cell(fd)
 *      Closes and does housekeeping on an opened cell file
 *
 *   G_unopen_cell(fd)
 *      Closes and does housekeeping on an opened cell file
 *      without creating the cell file
 *
 *   parms:
 *      int fd     open cell file
 *
 *   returns:
 *      -1   on fail
 *       0   on success
 *
 *   note:
 *      On closing of a cell file that was open for writing, dummy cats
 *      and history files are created. Histogram and range info are written.
 *
 **********************************************************************/

#ifdef __MINGW32__
#  include <windows.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include "G.h"

#define FORMAT_FILE "f_format"
#define QUANT_FILE  "f_quant"
#define NULL_FILE   "null"

static int close_old (int);
static int close_new (int,int);
static char CELL_DIR[100];


/*!
 * \brief close a raster map
 *
 * The raster map
 * opened on file descriptor <b>fd</b> is closed. Memory allocated for raster
 * processing is freed. If open for writing, skeletal support files for the new
 * raster map are created as well.
 * <b>Note.</b> If a module wants to explicitly write support files (e.g., a
 * specific color table) for a raster map it creates, it must do so after the
 * raster map is closed. Otherwise the close will overwrite the support files.
 * See Raster_Map_Layer_Support_Routines for routines which write
 * raster support files.
 *
 *  \param fd
 *  \return int
 */

 
/*!
 * \brief 
 *
 * If the map is a new floating point, move the
 * <tt>.tmp</tt> file into the <tt>fcell</tt> element, create an empty file in the
 * <tt>cell</tt> directory; write the floating-point range file; write a default
 * quantization file quantization file is set here to round fp numbers (this is
 * a default for now). create an empty category file, with max cat = max value
 * (for backwards compatibility). Move the <tt>.tmp</tt> NULL-value bitmap file to
 * the <tt>cell_misc</tt> directory.
 *
 *  \return int
 */

int G_close_cell (int fd)
{
    struct fileinfo *fcb = &G__.fileinfo[fd];

    if (fd < 0 || fd >= G__.fileinfo_count || fcb->open_mode <= 0)
	return -1;
    if (fcb->open_mode == OPEN_OLD)
	return close_old (fd);

    return close_new (fd, 1);
}


/*!
 * \brief unopen a raster map
 *
 * The raster map
 * opened on file descriptor <b>fd</b> is closed. Memory allocated for raster
 * processing is freed. If open for writing, the raster map is not created and
 * the temporary file created when the raster map was opened is removed (see
 * Creating_and_Opening_New_Raster_Files).
 * This routine is useful when errors are detected and it is desired to not
 * create the new raster map. While it is true that the raster map will not be
 * created if the module exits without closing the file, the temporary file will
 * not be removed at module exit. GRASS database management will eventually
 * remove the temporary file, but the file can be quite large and will take up
 * disk space until GRASS does remove it. Use this routine as a courtesy to the
 * user.  
 *
 *  \param fd
 *  \return int
 */

int G_unopen_cell (int fd)
{
    struct fileinfo *fcb = &G__.fileinfo[fd];

    if (fd < 0 || fd >= G__.fileinfo_count || fcb->open_mode <= 0)
	return -1;
    if (fcb->open_mode == OPEN_OLD)
	return close_old (fd);
    else
	return close_new (fd, 0);
}

static int close_old (int fd)
{
    struct fileinfo *fcb = &G__.fileinfo[fd];
    int i;

    /* if G__.auto_mask was only allocated for reading map rows to create
       non-existant null rows, and not for actuall mask, free G__.mask_row 
       if(G__.auto_mask <=0)
          G_free (G__.mask_buf);
       This is obsolete since now the mask_bus is always allocated
    */

    for (i=0;i<NULL_ROWS_INMEM;i++)
       G_free (fcb->NULL_ROWS[i]);
    G_free (fcb->null_work_buf);

    if (fcb->cellhd.compressed)
	G_free (fcb->row_ptr);
    G_free (fcb->col_map);
    G_free (fcb->mapset);
    G_free (fcb->data);
    G_free (fcb->name);
    if (fcb->reclass_flag)
	G_free_reclass (&fcb->reclass);
    fcb->open_mode = -1;

    if(fcb->map_type != CELL_TYPE)
    {
        G_quant_free(&fcb->quant);
        xdr_destroy(&fcb->xdrstream);
    } 
    close (fd);

    return 1;
}

static int close_new (int fd,int ok)
{
    struct fileinfo *fcb = &G__.fileinfo[fd];
    int stat;
    struct Categories cats;
    struct History hist;
    char path[GPATH_MAX];
    CELL cell_min, cell_max;
    int row, i, open_mode;

    if (ok) {
        switch (fcb->open_mode)
        {
        case OPEN_NEW_COMPRESSED:
            G_debug(1, "close %s compressed", fcb->name); 
            break;
        case OPEN_NEW_UNCOMPRESSED:
            G_debug(1, "close %s uncompressed", fcb->name);
            break;
        case OPEN_NEW_RANDOM:
            G_debug(1, "close %s random", fcb->name);
            break;
        }

	if (fcb->open_mode != OPEN_NEW_RANDOM && fcb->cur_row < fcb->cellhd.rows) {
	    G_zero_raster_buf (fcb->data, fcb->map_type);
	    for (row = fcb->cur_row; row < fcb->cellhd.rows; row++)
	        G_put_raster_row (fd, fcb->data, fcb->map_type);
            G_free (fcb->data);
	    fcb->data = NULL;
	}

        /* create path : full null file name */
        G__make_mapset_element_misc("cell_misc", fcb->name);
        G__file_name_misc(path, "cell_misc", NULL_FILE, fcb->name, G_mapset());
        remove ( path );

        if(fcb->null_cur_row > 0) {
        /* if temporary NULL file exists, write it into cell_misc/name/null */
            int null_fd;

            null_fd = G__open_null_write(fd);
	    if(null_fd <= 0) return -1;
            if(null_fd < 1) return -1;

            /* first finish writing null file */
            /* write out the rows stored in memory */
	    for (row = fcb->min_null_row; 
                  row < fcb->null_cur_row; row++)
             G__write_null_bits(null_fd, fcb->NULL_ROWS[row - fcb->min_null_row], 
                                               row, fcb->cellhd.cols, fd); 

            /* write missing rows */
	    if (fcb->open_mode != OPEN_NEW_RANDOM 
                && fcb->null_cur_row < fcb->cellhd.rows)
            {
                G__init_null_bits(fcb->null_work_buf, fcb->cellhd.cols);
	        for (row = fcb->null_cur_row; row < fcb->cellhd.rows; row++)
                      G__write_null_bits(null_fd, fcb->null_work_buf, row, 
                                                          fcb->cellhd.cols, fd);
            }
            close (null_fd);
            
	    if(rename(fcb->null_temp_name, path)) {
		G_warning(_("closecell: can't move %s\nto null file %s"),
			  fcb->null_temp_name, path);
		stat = -1;
	    } else {
                remove ( fcb->null_temp_name );
            }
        } else {
            remove ( fcb->null_temp_name );
            remove ( path );
        } /* null_cur_row > 0 */

        if (fcb->open_mode == OPEN_NEW_COMPRESSED) { /* auto compression */
            fcb->row_ptr[fcb->cellhd.rows] = lseek (fd, 0L, SEEK_CUR);
            G__write_row_ptrs (fd);
        }

        if(fcb->map_type != CELL_TYPE) {  /* floating point map */
           int cell_fd;

            if (G__write_fp_format(fd) != 0) {
               G_warning(_("Error writing floating point format file for map %s"), fcb->name);
               stat = -1;
            }

            /* now write 0-length cell file */
            G__make_mapset_element ("cell");
            cell_fd = creat (G__file_name(path, "cell", fcb->name, fcb->mapset), 0666);
            close( cell_fd);
            strcpy(CELL_DIR, "fcell");
       } else {
            /* remove fcell/name file */
   	    G__file_name (path, "fcell", fcb->name, fcb->mapset);
            remove ( path );
            /* remove cell_misc/name/f_format */
   	    G__file_name_misc(path, "cell_misc", FORMAT_FILE, fcb->name, fcb->mapset);
            remove ( path );
            strcpy(CELL_DIR, "cell");
            close (fd);
       }
    } /* ok */
    /* NOW CLOSE THE FILE DESCRIPTOR */

    close (fd);
    /* remember open_mode */
    open_mode = fcb->open_mode;
    fcb->open_mode = -1;

    if (fcb->data != NULL)
	G_free (fcb->data);

    if (fcb->null_temp_name != NULL)
    {
	G_free (fcb->null_temp_name);
        fcb->null_temp_name = NULL;
    }

/* if the cell file was written to a temporary file
 * move this temporary file into the cell file
 * if the move fails, tell the user, but go ahead and create
 * the support files
 */
    stat = 1;
    if (ok && (fcb->temp_name != NULL)) {
	G__file_name (path, CELL_DIR, fcb->name, fcb->mapset);
        remove ( path );
	if(rename(fcb->temp_name, path)) {
	    G_warning(_("closecell: can't move %s\nto cell file %s"),
		      fcb->temp_name, path);
	    stat = -1;
        } else {
            remove ( fcb->temp_name );
        }
    }

    if (fcb->temp_name != NULL) {
	G_free (fcb->temp_name);
    }

    if (ok) {
	/* remove color table */
	G_remove_colors (fcb->name, "");

	/* create a history file */
        G_short_history (fcb->name, "raster", &hist);
	G_write_history (fcb->name, &hist);

	/* write the range */
        if(fcb->map_type == CELL_TYPE) {
       	     G_write_range (fcb->name, &fcb->range);
             G__remove_fp_range(fcb->name);
        }
	/*NOTE: int range for floating point maps is not written out */
        else /* if(fcb->map_type != CELL_TYPE) */
        {
       	     G_write_fp_range (fcb->name, &fcb->fp_range);
	     G_construct_default_range(&fcb->range);
	    /* this range will be used to add default rule to quant structure */
        }

        if ( fcb->map_type != CELL_TYPE)
           fcb->cellhd.format = -1;
        else /* CELL map */
	   fcb->cellhd.format = fcb->nbytes - 1;

	/* write header file */
        G_put_cellhd (fcb->name, &fcb->cellhd);

	/* if map is floating point write the quant rules, otherwise remove f_quant */
        if(fcb->map_type != CELL_TYPE) {
	/* DEFAULT RANGE QUANT
	     G_get_fp_range_min_max(&fcb->fp_range, &dcell_min, &dcell_max);
	     if(!G_is_d_null_value(&dcell_min) && !G_is_d_null_value(&dcell_max))
             {
		G_get_range_min_max(&fcb->range, &cell_min, &cell_max);
	        G_quant_add_rule(&fcb->quant, dcell_min, dcell_max, 
					     cell_min, cell_max);
             }
        */
	     G_quant_round(&fcb->quant);
             if( G_write_quant (fcb->name, fcb->mapset, &fcb->quant) < 0)
                      G_warning(_("unable to write quant file!"));
        } else {
            /* remove cell_misc/name/f_quant */
   	    G__file_name_misc(path, "cell_misc", QUANT_FILE, fcb->name, fcb->mapset);
            remove ( path );
        }

	/* create empty cats file */
       G_get_range_min_max(&fcb->range, &cell_min, &cell_max);
       if(G_is_c_null_value(&cell_max)) cell_max = 0;
       G_init_cats (cell_max, (char *)NULL, &cats);
       G_write_cats (fcb->name, &cats);
       G_free_cats (&cats);

	/* write the histogram */
	/* only works for integer maps */
       if((fcb->map_type == CELL_TYPE)
            &&(fcb->want_histogram)) {
        	    G_write_histogram_cs (fcb->name, &fcb->statf);
  	            G_free_cell_stats (&fcb->statf);
       } else {
            G_remove_histogram(fcb->name);
       }
    } /* OK */

    G_free (fcb->name);
    G_free (fcb->mapset);

    for (i=0;i<NULL_ROWS_INMEM;i++)
       G_free (fcb->NULL_ROWS[i]);
    G_free (fcb->null_work_buf);

    if(fcb->map_type != CELL_TYPE)
       G_quant_free(&fcb->quant);

    return stat;
}

/* returns 0 on success, 1 on failure */
int G__write_fp_format (int fd)
{
   struct fileinfo *fcb = &G__.fileinfo[fd];
   struct Key_Value *format_kv;
   char path[GPATH_MAX];
   int stat;

   if(fcb->map_type == CELL_TYPE)
   {
       G_warning(_("unable to write f_format file for CELL maps"));
       return 0;
   }
   format_kv = G_create_key_value();
   if(fcb->map_type == FCELL_TYPE)
       G_set_key_value ("type", "float", format_kv);
   else 
       G_set_key_value ("type", "double", format_kv);

   G_set_key_value ("byte_order", "xdr", format_kv);

   if (fcb->open_mode == OPEN_NEW_COMPRESSED)
      G_set_key_value ("lzw_compression_bits", "-1", format_kv);

   G__make_mapset_element_misc("cell_misc", fcb->name);
   G__file_name_misc(path, "cell_misc", FORMAT_FILE, fcb->name, fcb->mapset);
   G_write_key_value_file (path, format_kv, &stat);

   G_free_key_value(format_kv);

   return stat;
}
