

/*
 **  Written by David Gerdes  US Army Construction Engineering Research Lab
 **     April 1992
 **  Copyright 1992 USA-CERL   All rights reserved.
 **
 */

/*
 **  This is a simple worst case performance comparison between linkm and malloc
 */
#include <stdio.h>
#include <grass/linkm.h>

struct link
{
    char let;
    struct link *next;
};

/*
   #define LINKM
 */

int main(int argc, char *argv[])
{
    register int i;
    VOID_T *head;
    struct link List, *tmp, *p;
    int rev = 0;


    tmp = &List;

#ifdef LINKM
    /* link_set_chunk_size (2000); */
    head = (VOID_T *) link_init(sizeof(struct link));
#endif


    for (i = 0; i < 2000000; i++) {
#ifdef LINKM
	p = (struct link *)link_new(head);
#else
	p = (struct link *)malloc(sizeof(struct link));
#endif
	tmp->next = p;
	tmp = p;
	tmp->next = NULL;
    }

    for (p = List.next; p != NULL;) {
	tmp = p->next;
#ifdef LINKM
	link_dispose(head, p);
#else
	free(p);
#endif
	p = tmp;
    }


#ifdef LINKM
    link_cleanup(head);
#endif

    exit(0);
}
