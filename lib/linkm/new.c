/*
 **  Written by David Gerdes  US Army Construction Engineering Research Lab
 **     April 1992
 **  Copyright 1992 USA-CERL   All rights reserved.
 **
 */
#include <string.h>
#include <stdlib.h>
#include <grass/linkm.h>


VOID_T *link_new(struct link_head *Head)
{
    VOID_T *tmp;
    char *ctmp, *p;
    register int i;

    if (Head->Unused == NULL) {
	if (Head->max_ptr >= Head->alloced) {
	    /*DEBUG fprintf (stderr, "REALLOCING PTR_ARRAY (%d -> %d)\n", Head->alloced, Head->alloced * 2); */
	    if (NULL ==
		(tmp =
		 (VOID_T *) realloc(Head->ptr_array,
				    sizeof(VOID_T *) * Head->alloced * 2))) {
		if (Head->exit_flag)
		    link_out_of_memory();
		return NULL;
	    }
	    Head->ptr_array = (VOID_T **) tmp;
	    Head->alloced *= 2;
	}

	/*DEBUG fprintf (stderr, "Mallocing another chunk: %d\n", Head->max_ptr); */
	if (NULL == (tmp = (VOID_T *)
		     malloc(Head->chunk_size * Head->unit_size))) {
	    if (Head->exit_flag)
		link_out_of_memory();
	    return NULL;
	}

	Head->ptr_array[Head->max_ptr++] = (VOID_T *) tmp;
	Head->Unused = (VOID_T *) tmp;

	p = ctmp = (char *)tmp;
	for (i = 0; i < Head->chunk_size - 1; i++) {
	    link__set_next((VOID_T *) p,
			   (VOID_T *) & (ctmp[(i + 1) * Head->unit_size]));
	    /* p->next = p+1 */

	    p = &(ctmp[(i + 1) * Head->unit_size]);	/* p = p->next */
	}
	link__set_next((VOID_T *) p, NULL);	/* p->next = NULL */
    }

    tmp = Head->Unused;

    /* Unused = Unused->next */
    Head->Unused = link__get_next(Head->Unused);

    return tmp;
}
