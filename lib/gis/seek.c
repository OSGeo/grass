/*!
 * \file lib/gis/seek.c
 *
 * \brief GIS Library - file seek routines
 *
 * (C) 2009-2010 by the GRASS Development Team
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author Glynn Clements
 */

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <grass/gis.h>
#include <grass/glocale.h>

/*!
  \brief Get the current file position of the stream.

  \param fp file descriptor

  \return file position
  \return -1 on failure
*/
off_t G_ftell(FILE *fp)
{
#ifdef HAVE_FSEEKO
    return ftello(fp);
#else
    return (off_t) ftell(fp);
#endif     
}

/*!
  \brief Change the file position of the stream.

  The value of <i>whence</i> must be one of the constants `SEEK_SET',
  `SEEK_CUR', or `SEEK_END', to indicate whether the <i>offset</i> is
  relative to the beginning of the file, the current file position, or
  the end of the file, respectively.

  \param fp file descriptor
  \param offset offset
  \param whence
*/
void G_fseek(FILE *fp, off_t offset, int whence)
{
#ifdef HAVE_FSEEKO
    if (fseeko(fp, offset, whence) != 0)
	G_fatal_error(_("Unable to seek: %s"), strerror(errno));
#else
    long loff = (long) offset;
    if ((off_t) loff != offset)
	G_fatal_error(_("Seek offset out of range"));
    if (fseek(fp, loff, whence) != 0)
	G_fatal_error(_("Unable to seek: %s"), strerror(errno));
#endif     
}
