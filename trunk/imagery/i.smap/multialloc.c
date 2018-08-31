#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <grass/gis.h>
#include <grass/glocale.h>


/* modified from dynamem.c on 4/29/91 C. Bouman */

/*
 * multialloc( s, d,  dn, dn ....) allocates a d dimensional array, whose 
 * dimensions are stored in a list starting at d1. Each array element is 
 * of size s. 
 */

char *multialloc(size_t s,	/* individual array element size */
		 int d,		/* number of dimensions */
		 ...)
{
    va_list ap;			/* varargs list traverser */
    int max,			/* size of array to be declared */
     *q;			/* pointer to dimension list */
    char **r,			/* pointer to beginning of the array of the
				 * pointers for a dimension */
    **s1, *t, *tree;		/* base pointer to beginning of first array */
    int i, j;			/* loop counters */
    int *d1;			/* dimension list */

    va_start(ap, d);
    d1 = (int *)G_malloc(d * sizeof(int));

    for (i = 0; i < d; i++)
	d1[i] = va_arg(ap, int);

    r = &tree;
    q = d1;			/* first dimension */
    max = 1;
    for (i = 0; i < d - 1; i++, q++) {	/* for each of the dimensions
					 * but the last */
	max *= (*q);
	r[0] = (char *)G_malloc(max * sizeof(char **));
	r = (char **)r[0];	/* step through to beginning of next
				 * dimension array */
    }
    max *= s * (*q);		/* grab actual array memory */
    r[0] = (char *)G_malloc(max * sizeof(char));

    /*
     * r is now set to point to the beginning of each array so that we can
     * use it to scan down each array rather than having to go across and
     * then down 
     */
    r = (char **)tree;		/* back to the beginning of list of arrays */
    q = d1;			/* back to the first dimension */
    max = 1;
    for (i = 0; i < d - 2; i++, q++) {	/* we deal with the last
					 * array of pointers later on */
	max *= (*q);		/* number of elements in this dimension */
	for (j = 1, s1 = r + 1, t = r[0]; j < max; j++) {	/* scans down array for
								 * first and subsequent
								 * elements */

	    /*  modify each of the pointers so that it points to
	     * the correct position (sub-array) of the next
	     * dimension array. s1 is the current position in the
	     * current array. t is the current position in the
	     * next array. t is incremented before s1 is, but it
	     * starts off one behind. *(q+1) is the dimension of
	     * the next array. */

	    *s1 = (t += sizeof(char **) * *(q + 1));
	    s1++;
	}
	r = (char **)r[0];	/* step through to beginning of next
				 * dimension array */
    }
    max *= (*q);		/* max is total number of elements in the
				 * last pointer array */

    /* same as previous loop, but different size factor */
    for (j = 1, s1 = r + 1, t = r[0]; j < max; j++)
	*s1++ = (t += s * *(q + 1));

    va_end(ap);
    G_free((char *)d1);

    return tree;		/* return base pointer */
}


/*
 * multifree releases all memory that we have already declared analogous to
 * G_free () when using G_malloc () 
 */
void multifree(char *r, int d)
{
    char **p;
    char *next = NULL;
    int i;

    for (p = (char **)r, i = 0; i < d; p = (char **)next, i++) {
	if (p != NULL) {
	    next = *p;
	    G_free((char *)p);
	}
    }
}


unsigned char **get_img(int wd, int ht, size_t size)
{
    char *pt;

    if ((pt = multialloc(size, 2, ht, wd)) == NULL)
	G_fatal_error(_("Out of memory"));

    return ((unsigned char **)pt);
}


void free_img(unsigned char **pt)
{
    multifree((char *)pt, 2);
}
