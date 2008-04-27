/**
 * \file debug.c
 *
 * \brief Debug functions.
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author GRASS GIS Development Team
 *
 * \date 1999-2006
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include "G.h"


static int grass_debug_level = -1;


/**
 * \fn int G_debug (int level, char *msg,...)
 *
 * \brief Print debugging message.
 *
 * Print debugging message if environment variable GRASS_DEBUG_LEVEL
 * is set to level equal or greater  
 *
 * Levels: (recommended levels)<br>
 * 1 - message is printed once or few times per module<br>
 * 3 - each row (raster) or line (vector)<br>
 * 5 - each cell (raster) or point (vector) 
 *
 * \param[in] level
 * \param[in] msg
 * \return 0 on error
 * \return 1 on success
 */

int G_debug (int level, const char *msg,...)
{
#ifdef GDEBUG
    char    *lstr, *filen;
    va_list ap;
    FILE    *fd;
   
    if (grass_debug_level < 0) {
        lstr = G__getenv( "DEBUG" );

        if ( lstr != NULL )
            grass_debug_level = atoi ( lstr );
        else
            grass_debug_level = 0;
    }
	
    if ( grass_debug_level >= level ) {
        va_start(ap, msg);

	filen =  getenv("GRASS_DEBUG_FILE"); 
        if ( filen != NULL ) {
	    fd = fopen (filen,"a");
            if ( !fd ) {
		G_warning ( _("Cannot open debug file '%s'"), filen);
		return 0;
	    }
	} else {
	    fd = stderr;
	}
        
	fprintf (fd, "D%d/%d: ", level, grass_debug_level);
	vfprintf (fd, msg, ap);
	fprintf (fd, "\n");
        fflush (fd);
	
	if ( filen != NULL ) fclose ( fd );
	
	va_end(ap);
    }
#endif

    return 1;
}


/**
 * \fn int G_dump (int fd)
 *
 * \brief Dumps status of various GIS parameters.
 * 
 * Dumps status of various GIS parameters of a particular
 * file descriptor, <b>fd</b>.
 *
 * \param[in] fd
 * \return always returns 0
 *
*/

int G_dump(int fd)
{
    const struct fileinfo *fcb = &G__.fileinfo[fd];

    G_message("G_dump: memory allocated to G__");
    G_message("Size of cell in fp maps = %d", G__.fp_nbytes);
    G_message("type for writing floating maps = %d", G__.fp_type);
    G_message("current window = %p", &G__.window);
    G_message("Flag: window set? %d", G__.window_set);
    G_message("File descriptor for automatic mask %d", G__.mask_fd);
    G_message("Flag denoting automatic masking %d", G__.auto_mask); 
    G_message("CELL mask buffer %p", G__.mask_buf);
    G_message("buffer for reading null rows %p", G__.null_buf);
    G_message("Pre/post compressed data buffer %p", G__.compressed_buf);
    G_message("sizeof compressed_buf %d", G__.compressed_buf_size);
    G_message("work data buffer %p", G__.work_buf);
    G_message("sizeof work_buf %d", G__.work_buf_size);
    G_message("sizeof null_buf %d", G__.null_buf_size);
    G_message("sizeof mask_buf %d", G__.mask_buf_size);
    G_message("Histogram request %d", G__.want_histogram);

    G_message("G_dump: file #%d", fd);
    G_message("open mode = %d", fcb->open_mode);
    G_message("Cell header %p",&fcb->cellhd);
    G_message("Table reclass %p", &fcb->reclass);
    G_message("Cell stats %p", &fcb->statf);
    G_message("Range structure %p", &fcb->range);
    G_message("float Range structure %p", &fcb->fp_range);
    G_message("want histogram?  %d", fcb->want_histogram);
    G_message("Automatic reclass flag %d", fcb->reclass_flag);
    G_message("File row addresses %p", fcb->row_ptr);
    G_message("Data to window col mapping %p", fcb->col_map);
    G_message("Data to window row constants %f,%f", fcb->C1,fcb->C2);
    G_message("Current data row in memory %d", fcb->cur_row);
    G_message("Current null row in memory %d", fcb->null_cur_row);
    G_message("nbytes per cell for current row %d", fcb->cur_nbytes);
    G_message("Decompressed data buffer %s", fcb->data);
    G_message("bytes per cell %d", fcb->nbytes);
    G_message("type: int, float or double map %d", fcb->map_type);
    G_message("Temporary name for NEW files %s", fcb->temp_name);
    G_message("Temporary name for NEW NULL files %s", fcb->null_temp_name);
    G_message("for existing raster maps %d", fcb->null_file_exists);
    G_message("Name of open file %s", fcb->name);
    G_message("Mapset of open file %s", fcb->mapset);
    G_message("io error warning given %d", fcb->io_error);
    G_message("xdr stream for reading fp %p", &fcb->xdrstream);
    G_message("NULL_ROWS array[%d] = %p", NULL_ROWS_INMEM, fcb->NULL_ROWS);
    G_message("data buffer for reading null rows %p", fcb->null_work_buf);
    G_message("Minimum row null number in memory %d", fcb->min_null_row);
    G_message("Quant ptr = %p", &fcb->quant);
    G_message("G_dump: end");

    return 0;
}
