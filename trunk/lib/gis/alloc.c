/*!
 * \file lib/gis/alloc.c
 *
 * \brief GIS Library - Memory allocation routines.
 *
 * (C) 1999-2009 by the GRASS Development Team
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author Original author CERL
 */

#include <stdlib.h>
#include <grass/gis.h>
#include <grass/glocale.h>

/*!
 * \brief Memory allocation.
 *
 * Allocates a block of memory at least <i>n</i> bytes which is
 * aligned properly for all data types. A pointer to the aligned block
 * is returned.
 *
 * Dies with error message on memory allocation fail.
 *
 * \param file file name
 * \param line line number
 * \param n number of elements
 */

void *G__malloc(const char *file, int line, size_t n)
{
    void *buf;
	
    if (n <= 0)
	n = 1;			/* make sure we get a valid request */
	
    buf = malloc(n);
    if (!buf) {
	struct Cell_head window;
	
	G_get_window(&window);
	G_important_message(_("Current region rows: %d, cols: %d"), 
		            window.rows, window.cols);

	G_fatal_error(_("G_malloc: unable to allocate %lu bytes of memory at %s:%d"),
		      (unsigned long) n, file, line);
    }

    return buf;
}

/*!
 * \brief Memory allocation.
 *
 * Allocates a properly aligned block of memory <i>n</i>*<i>m</i>
 * bytes in length, initializes the allocated memory to zero, and
 * returns a pointer to the allocated block of memory.
 *
 * Dies with error message on memory allocation fail.
 *
 * <b>Note:</b> Allocating memory for reading and writing raster maps
 * is discussed in \ref Allocating_Raster_I_O_Buffers.
 *
 * \param file fine name
 * \param line line number
 * \param m element size
 * \param n number of elements
 */

void *G__calloc(const char *file, int line, size_t m, size_t n)
{
    void *buf;

    if (m <= 0)
	m = 1;			/* make sure we get a valid requests */
    if (n <= 0)
	n = 1;

    buf = calloc(m, n);
    if (!buf) {
	struct Cell_head window;
	
	G_get_window(&window);
	G_important_message(_("Current region rows: %d, cols: %d"), 
		            window.rows, window.cols);

	G_fatal_error(_("G_calloc: unable to allocate %lu * %lu bytes of memory at %s:%d"),
		      (unsigned long) m, (unsigned long) n, file, line);
    }

    return buf;
}


/*!
 * \brief Memory reallocation.
 *
 * Changes the <i>size</i> of a previously allocated block of memory
 * at <i>ptr</i> and returns a pointer to the new block of memory. The
 * <i>size</i> may be larger or smaller than the original size. If the
 * original block cannot be extended "in place", then a new block is
 * allocated and the original block copied to the new block.
 *
 * <b>Note:</b> If <i>buf</i> is NULL, then this routine simply
 * allocates a block of <i>n</i> bytes else <i>buf</i> must point to
 * memory that has been dynamically allocated by G_malloc(),
 * G_calloc(), G_realloc(), malloc(3), alloc(3), or realloc(3).. This
 * routine works around broken realloc() routines, which do not
 * handle a NULL <i>buf</i>.
 *
 * \param file file name
 * \param line line number
 * \param[in,out] buf buffer holding original data
 * \param[in] n array size
 */
void *G__realloc(const char *file, int line, void *buf, size_t n)
{
    if (n <= 0)
	n = 1;			/* make sure we get a valid request */

    if (!buf)
	buf = malloc(n);
    else
	buf = realloc(buf, n);

    if (!buf) {
	struct Cell_head window;
	
	G_get_window(&window);
	G_important_message(_("Current region rows: %d, cols: %d"), 
		            window.rows, window.cols);

	G_fatal_error(_("G_realloc: unable to allocate %lu bytes of memory at %s:%d"),
		      (unsigned long) n, file, line);
    }

    return buf;
}


/*!
 * \brief Free allocated memory.
 *
 * \param[in,out] buf buffer holding original data
 */

void G_free(void *buf)
{
    free(buf);
}

/*!
 * \brief Advance void pointer
 *
 * Advances void pointer by <i>size</i> bytes. Returns new pointer
 * value.
 *
 * Useful in raster row processing loops, substitutes
 *
 \code
 CELL *cell; 
 cell += n;
 \endcode
 *
 * Now 
 \code
 rast = G_incr_void_ptr(rast, Rast_cell_size(data_type))
 \endcode
 *
 * (where rast is void* and <i>data_type</i> is RASTER_MAP_TYPE can be
 * used instead of rast++.)
 *
 * Very useful to generalize the row processing - loop i.e.
 * \code
 *   void * buf_ptr += Rast_cell_size(data_type)
 * \endcode
 *
 * \param ptr pointer
 * \param size buffer size
 *
 * \return pointer to the data
 */
#ifndef G_incr_void_ptr
void *G_incr_void_ptr(const void *ptr, size_t size)
{
    /* assuming that the size of unsigned char is 1 */
    return (void *)((const unsigned char *)ptr + size);
}
#endif
