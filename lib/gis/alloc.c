
/**
 * \file alloc.c
 *
 * \brief GIS Library - Memory allocation routines.
 *
 * (C) 2001-2008 by the GRASS Development Team
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author GRASS GIS Development Team
 *
 * \date 1999-2008
 */

#include <stdlib.h>
#include <grass/gis.h>
#include <grass/glocale.h>


/**
 * \brief Memory allocation.
 *
 * Allocates a block of
 * memory at least <b>n</b> bytes which is aligned properly for all data
 * types. A pointer to the aligned block is returned.<br>
 * Dies with error message on memory allocation fail.
 *
 * \param[in] n
 * \return void * 
 */

void *G__malloc(const char *file, int line, size_t n)
{
    void *buf;
	
    if (n <= 0)
	n = 1;			/* make sure we get a valid request */
	
    buf = malloc(n);
    if (!buf)
	G_fatal_error(_("G_malloc: unable to allocate %lu bytes at %s:%d"),
		      (unsigned long) n, file, line);

    return buf;
}

/**
 * \brief Memory allocation.
 *
 * Allocates a
 * properly aligned block of memory <b>n</b>*<b>m</b> bytes in length,
 * initializes the allocated memory to zero, and returns a pointer to the
 * allocated block of memory.<br>
 * Dies with error message on memory allocation fail.<br>
 * <b>Note:</b> Allocating memory for reading and writing raster maps is
 * discussed in Allocating_Raster_I_O_Buffers.
 *
 * \param[in] n number of elements
 * \param[in] m element size
 * \return void * 
 */

void *G__calloc(const char *file, int line, size_t m, size_t n)
{
    void *buf;

    if (m <= 0)
	m = 1;			/* make sure we get a valid requests */
    if (n <= 0)
	n = 1;

    buf = calloc(m, n);
    if (!buf)
	G_fatal_error(_("G_calloc: unable to allocate %lu * %lu bytes at %s:%d"),
		      (unsigned long) m, (unsigned long) n, file, line);

    return buf;
}


/**
 * \brief Memory reallocation.
 *
 * Changes the
 * <b>size</b> of a previously allocated block of memory at <b>ptr</b> and
 * returns a pointer to the new block of memory. The <b>size</b> may be larger
 * or smaller than the original size. If the original block cannot be extended
 * "in place", then a new block is allocated and the original block copied to the
 * new block.<br>
 * <b>Note:</b> If <b>buf</b> is NULL, then this routine simply allocates a
 * block of <b>n</b> bytes else <b>buf</b> must point to memory that has been dynamically
 * allocated by <i>G_malloc()</i>, <i>G_calloc()</i>, <i>G_realloc()</i>,
 * malloc(3), alloc(3), or realloc(3).. This routine works around broken realloc( )
 * routines, which do not handle a NULL <b>buf</b>.
 *
 * \param[in,out] buf buffer holding original data
 * \param[in] n array size
 * \return void * 
 */

void *G__realloc(const char *file, int line, void *buf, size_t n)
{
    if (n <= 0)
	n = 1;			/* make sure we get a valid request */

    if (!buf)
	buf = malloc(n);
    else
	buf = realloc(buf, n);

    if (!buf)
	G_fatal_error(_("G_realloc: unable to allocate %lu bytes at %s:%d"),
		      (unsigned long) n, file, line);

    return buf;
}


/**
 * \brief Free allocated memory.
 *
 * \param[in,out] buf buffer holding original data
 */

void G_free(void *buf)
{
    free(buf);
}
