#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <grass/gis.h>
#include "vector.h"


/* init vector structure */
void vector_init(void)
{
    vector.count = 0;
    vector.alloc = 0;
    vector.layer = NULL;
}

/* allocate at least one free space for layer */
void vector_alloc(void)
{
    if (vector.count == vector.alloc) {
	vector.alloc += 20;
	vector.layer =
	    (LAYER *) G_realloc(vector.layer, vector.alloc * sizeof(LAYER));
    }
}
