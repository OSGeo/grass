/*!
   \file diglib/file.c

   \brief Vector library - file management (lower level functions)

   Lower level functions for reading/writing/manipulating vectors.

   Note: seems that the time is almost the same for both cases: 
    - reading from file 
    - load whole file to memory and read from memory
   
   (C) 2001-2009 by the GRASS Development Team

   This program is free software under the GNU General Public License
   (>=v2).  Read the file COPYING that comes with GRASS for details.

   \author Original author CERL, probably Dave Gerdes
   \author Update to GRASS 5.7 Radim Blazek
 */

#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <grass/vector.h>
#include <grass/glocale.h>

/*!
  \brief Get struct gvfile position.

  \param file pointer to struct gvfile structure

  \return current file position
*/
off_t dig_ftell(struct gvfile *file)
{
    if (file->loaded) /* using memory */
	return (file->current - file->start);

    return (G_ftell(file->file));
}

/*!
  \brief Set struct gvfile position.
 
  Start positions:
  
   - SEEK_SET (start)
   - SEEK_CUR (current position)
   - SEEK_END (end)
  
  \param file pointer to struct gvfile structure
  \param offset offset position
  \param whence start position

  \return 0 OK
  \return -1 error
*/
int dig_fseek(struct gvfile * file, off_t offset, int whence)
{
    if (file->loaded) {	 /* using memory */
	switch (whence) {
	case SEEK_SET:
	    file->current = file->start + offset;
	    break;
	case SEEK_CUR:
	    file->current += offset;
	    break;
	case SEEK_END:
	    file->current = file->start + file->size + offset;
	    break;
	}
	return 0;
    }

    G_fseek(file->file, offset, whence);

    return  0;
}

/*!
  \brief Rewind file position.
 
  \param file pointer to gvfile structure
*/
void dig_rewind(struct gvfile * file)
{
    if (file->loaded) {	/* using memory */
	file->current = file->start;
    }
    else {
	rewind(file->file);
    }
}

/*!
  \brief Flush struct gvfile.
 
  \param file pointer to struct gvfile structure

  \return 0
*/
int dig_fflush(struct gvfile * file)
{
    if (file->loaded) {	/* using memory */
	return 0;
    }
    else {
	return (fflush(file->file));
    }
}

/*!
  \brief Read struct gvfile.
 
  \param[out] ptr data buffer
  \param size buffer size
  \param nmemb number of members
  \param file pointer to struct gvfile structure

  \return number of read members
 */
size_t dig_fread(void *ptr, size_t size, size_t nmemb, struct gvfile *file)
{
    long tot;
    size_t cnt;

    if (file->loaded) {	/* using memory */
	if (file->current >= file->end) { /* EOF */
	    return 0;
	}
	tot = size * nmemb;
	cnt = nmemb;
	if (file->current + tot > file->end) {
	    tot = file->end - file->current;
	    cnt = (int)tot / size;
	}
	memcpy(ptr, file->current, tot);
	file->current += tot;
	return (cnt);
    }
    return (fread(ptr, size, nmemb, file->file));
}

/*!
  \brief Write struct gvfile.

  \param ptr data buffer
  \param size buffer size
  \param nmemb number of members
  \param[out] file pointer to struct gvfile structure

  \return number of items written
 */
size_t dig_fwrite(const void *ptr, size_t size, size_t nmemb, struct gvfile *file)
{
    if (file->loaded) {	/* using memory */
	G_fatal_error(_("Writing to file loaded to memory not supported"));
    }

    return fwrite(ptr, size, nmemb, file->file);
}

/*!
  \brief Initialize gvfile strcuture
  
  \param[in,out] file pointer to gvfile structure
*/
void dig_file_init(struct gvfile *file)
{
    G_zero(file, sizeof(struct gvfile));
}

/*!
  \brief Load opened struct gvfile to memory.
 
  Warning: position in file is set to the beginning.
 
  \param file pointer to struct gvfile structure

  \return 1 loaded
  \return 0 not loaded
  \return -1 error
*/
/* unused, coor file is never loaded to memory. Remove ? MM 2010 */
int dig_file_load(struct gvfile * file)
{
    int ret, mode, load;
    const char *cmode;
    size_t size;
    struct stat sbuf;

    G_debug(2, "dig_file_load ()");

    if (file->file == NULL) {
	G_warning(_("Unable to load file to memory, file not open"));
	return -1;
    }

    /* Get mode */
    mode = GV_MEMORY_NEVER;
    cmode = G__getenv("GV_MEMORY");
    if (cmode != NULL) {
	if (G_strcasecmp(cmode, "ALWAYS") == 0)
	    mode = GV_MEMORY_ALWAYS;
	else if (G_strcasecmp(cmode, "NEVER") == 0)
	    mode = GV_MEMORY_NEVER;
	else if (G_strcasecmp(cmode, "AUTO") == 0)
	    mode = GV_MEMORY_AUTO;
	else
	    G_warning(_("Vector memory mode not supported, using 'AUTO'"));
    }
    G_debug(2, "  requested mode = %d", mode);


    fstat(fileno(file->file), &sbuf);
    size = sbuf.st_size;

    G_debug(2, "  size = %lu", (long unsigned int) size);

    /* Decide if the file should be loaded */
    /* TODO: I don't know how to get size of free memory (portability) to decide if load or not for auto */
    if (mode == GV_MEMORY_AUTO)
	mode = GV_MEMORY_NEVER;
    if (mode == GV_MEMORY_ALWAYS)
	load = 1;
    else
	load = 0;

    if (load) {
	file->start = G_malloc(size);
	if (file->start == NULL)
	    return -1;

	G_fseek(file->file, 0L, 0);
	ret = fread(file->start, size, 1, file->file);	/* Better to read in smaller portions? */
	G_fseek(file->file, 0L, 0);	/* reset to the beginning */

	if (ret <= 0) {
	    G_free(file->start);
	    return -1;
	}

	file->alloc = size;
	file->size = size;
	file->current = file->start;
	file->end = file->start + size;

	file->loaded = 1;
	G_debug(2, "  file was loaded to the memory");
	return 1;
    }
    else {
	G_debug(2, "  file was not loaded to the memory");
    }

    return 0;
}

/*!
  \brief Free struct gvfile.

  \param file pointer to struct gvfile structure
*/
void dig_file_free(struct gvfile * file)
{
    if (file->loaded) {
	G_free(file->start);
	file->loaded = 0;
	file->alloc = 0;
    }
}
