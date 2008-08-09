#include <grass/gis.h>

void *falloc(int nelem, int elsize)
{
    return G_calloc(nelem, elsize);
}

void *frealloc(void *oldptr, int nelem, int elsize)
{
    return G_realloc(oldptr, nelem * elsize);
}
