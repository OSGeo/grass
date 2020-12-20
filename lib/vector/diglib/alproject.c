/*
 ****************************************************************************
 *
 * MODULE:       Vector library 
 *              
 * AUTHOR(S):    Original author CERL, probably Dave Gerdes.
 *               Update to GRASS 5.7 Radim Blazek.
 *
 * PURPOSE:      Lower level functions for reading/writing/manipulating vectors.
 *
 * COPYRIGHT:    (C) 2001 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *              License (>=v2). Read the file COPYING that comes with GRASS
 *              for details.
 *
 *****************************************************************************/
#include <unistd.h>
#include <stdlib.h>
#include <grass/vector.h>

/*  functions - alloc_space(), falloc(), frealloc() _falloc() _frealloc() */


/*   alloc_space ()    allocates space if needed.
 *    All allocated space is created by calloc (2).
 *
 *   args: number of elements wanted, pointer to number of currently allocated
 *   elements, size of chunks to allocate,  pointer to current array, sizeof
 *   an element.
 */

void *dig_alloc_space(int n_wanted,
		      int *n_elements,
		      int chunk_size, void *ptr, int element_size)
{
    char *p;

    p = dig__alloc_space(n_wanted, n_elements, chunk_size, ptr, element_size);

    if (p == NULL) {
	fprintf(stderr, "\nERROR: out of memory.  memory asked for: %d\n",
		n_wanted);
	exit(EXIT_FAILURE);
    }

    return (p);
}

void *dig__alloc_space(int n_wanted, int *n_elements, int chunk_size, void *ptr,	/* changed char -> void instead of casting.  WBH 8/16/1998  */
		       int element_size)
{
    int to_alloc;

    to_alloc = *n_elements;

    /*  do we need to allocate more space  */
    if (n_wanted < to_alloc)
	return (ptr);

    /*  calculate the number needed by chunk size */
    /*  ORIGINAL
       while (n_wanted >= to_alloc)
       to_alloc += chunk_size;
     */
    /*
     **  This was changed as a test on Aug 21, 1990
     **  Build.vect was taking outrageous amounts of
     **  memory to run, so instead of blaming my
     **  code, I decided that it could be the realloc/malloc
     **  stuff not making efficient use of the space.
     **  So the fix is to instead of asking for many small
     **  increments, ask for twice as much space as we are currently
     **  using, each time we need more space.
     */
    while (n_wanted >= to_alloc)
	to_alloc += *n_elements ? *n_elements : chunk_size;

    /*  first time called allocate initial storage  */
    if (*n_elements == 0)
	ptr = G_calloc(to_alloc, element_size);
    else
	ptr = dig__frealloc((char *)ptr, to_alloc, element_size, *n_elements);

    *n_elements = to_alloc;

    return (ptr);
}


void *dig_falloc(int nelem, int elsize)
{
    void *ret;

    if ((ret = dig__falloc(nelem, elsize)) == NULL) {
	fprintf(stderr, "Out of Memory.\n");
	G_sleep(2);
	exit(EXIT_FAILURE);
    }
    return (ret);
}

void *dig_frealloc(void *oldptr, int nelem, int elsize, int oldnelem)
{
    char *ret;

    if ((ret = dig__frealloc(oldptr, nelem, elsize, oldnelem)) == NULL) {
	fprintf(stderr, "\nOut of Memory on realloc.\n");
	G_sleep(2);
	exit(EXIT_FAILURE);
    }
    return (ret);
}

/*  these functions don't exit on "no more memory",  calling function should
   check the return value  */

void *dig__falloc(int nelem, int elsize)
{
    char *ptr;

    if (elsize == 0) {
	elsize = 4;
    }
    if (nelem == 0) {
	nelem = 1;
    }

    ptr = G_calloc(nelem, elsize);
    return (ptr);
}

void *dig__frealloc(void *oldptr, int nelem, int elsize, int oldnelem)
{
    char *ptr;

    if (elsize == 0) {
	elsize = 4;
    }
    if (nelem == 0) {
	nelem = 1;
    }

    ptr = G_calloc(nelem, elsize);

    /*  out of memory  */
    if (!ptr)
	return (ptr);

    {
	register char *a;
	register char *b;
	register size_t n;

	n = oldnelem * elsize;
	a = ptr;
	b = oldptr;
	while (n--)
	    *a++ = *b++;
    }

    G_free(oldptr);
    return (ptr);
}
