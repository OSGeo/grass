#include <grass/gis.h>

char *falloc(int nelem, int elsize)
{
    char *ptr;

    ptr = G_calloc(nelem, elsize);

    if (!ptr)
	G_fatal_error("ERROR: no more memory available");

    return (ptr);
}

char *frealloc(char *oldptr, int nelem, int elsize, int oldnelem)
{
    char *ptr;

    ptr = G_calloc(nelem, elsize);
    if (!ptr)
	G_fatal_error("ERROR: no more memory available");

    {
	register char *a;
	register char *b;
	register int n;

	n = oldnelem * elsize;
	a = ptr;
	b = oldptr;
	while (n--)
	    *a++ = *b++;
    }

    G_free(oldptr);
    return (ptr);
}
