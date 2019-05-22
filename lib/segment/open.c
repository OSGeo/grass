
/**
 * \file lib/segment/open.c
 *
 * \brief Segment creation routine.
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author GRASS GIS Development Team
 *
 * \date 2018
 */

#include <unistd.h>
#include <fcntl.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include "local_proto.h"

/**
 * \brief Initialize segment structure and open segment file.
 *
 * Initializes the <b>seg</b> structure and prepares a temporary file. 
 * This fn is a wrapper for Segment_format() and Segment_init()
 *
 * <b>Note:</b> The file with name fname will be created anew.
 *
 * \param[in,out] SEG segment
 * \param[in] fname file name
 * \param[in] nrows number of non-segmented rows
 * \param[in] ncols number of non-segmented columns
 * \param[in] srows segment rows
 * \param[in] scols segment columns
 * \param[in] len length of data type
 * \param[in] nseg number of segments to remain in memory
 * \return 1 if successful
 * \return -1 if file name is invalid
 * \return -2 if file write error
 * \return -3 if illegal parameters are passed
 * \return -4 if file could not be re-opened
 * \return -5 if prepared file could not be read
 * \return -6 if out of memory
 */

int
Segment_open(SEGMENT *SEG, char *fname, off_t nrows, off_t ncols,
             int srows, int scols, int len, int nseg)
{
    int ret;
    int nseg_total;

    nseg_total = ((nrows + srows - 1) / srows) * 
                 ((ncols + scols - 1) / scols);

    if (nseg >= nseg_total) {
	G_verbose_message(_("Using memory cache"));

	SEG->nrows = nrows;
	SEG->ncols = ncols;
	SEG->len = len;
	SEG->nseg = nseg;
	SEG->cache = G_calloc(sizeof(char) * SEG->nrows * SEG->ncols, SEG->len);
	SEG->scb = NULL;
	SEG->open = 1;
	
	return 1;
    }

    G_verbose_message(_("Using disk cache"));

    if (!fname) {
	G_warning(_("Segment file name is NULL"));
	return -1;
    }
    /* file exists? */
    if (access(fname, F_OK) == 0) {
	G_warning(_("Segment file exists already"));
	return -1;
    }
    
    SEG->fname = G_store(fname);
    SEG->fd = -1;

    if (-1 == (SEG->fd = creat(SEG->fname, 0666))) {
	G_warning(_("Unable to create segment file"));
	return -1;
    }
    if (0 > (ret = Segment_format_nofill(SEG->fd, nrows, ncols, srows,
							scols, len))) {
	close(SEG->fd);
	unlink(SEG->fname);
	if (ret == -1) {
	    G_warning(_("Could not write segment file"));
	    return -2;
	}
	else { /* ret = -3 */
	    G_warning(_("Illegal segment configuration parameter(s)"));
	    return ret;
	}
    }
    /* re-open for read and write */
    close(SEG->fd);
    SEG->fd = -1;
    if (-1 == (SEG->fd = open(SEG->fname, 2))) {
	unlink(SEG->fname);
	G_warning(_("Unable to re-open segment file"));
	return -4;
    }
    if (0 > (ret = Segment_init(SEG, SEG->fd, nseg))) {
	close(SEG->fd);
	unlink(SEG->fname);
	if (ret == -1) {
	    G_warning(_("Could not read segment file"));
	    return -5;
	}
	else {
	    G_warning(_("Out of memory"));
	    return -6;
	}
    }

    return 1;
}
