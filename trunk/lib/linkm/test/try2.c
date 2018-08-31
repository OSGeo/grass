/*
 **  Written by David Gerdes  US Army Construction Engineering Research Lab
 **     April 1992
 **  Copyright 1992 USA-CERL   All rights reserved.
 **
 */


/*
 **  read from stdin and each line into a linked list of chars
 **  then print it back out.   if there is any argument specified
 **  the lines will be printed out reversed.
 */

#include <stdio.h>
#include <grass/linkm.h>

struct link
{
    char let;
    struct link *next;
};

int main(int argc, char *argv[])
{
    register int i;
    VOID_T *head;
    struct link List, *tmp, *p;
    int rev = 0;
    char buf[4096];

    if (argc == 2)
	rev = 1;


    List.next = NULL;
    List.let = ' ';


    link_set_chunk_size(1);
    head = (VOID_T *) link_init(sizeof(struct link));


    while (NULL != gets(buf)) {
	for (i = 0; buf[i] != '\0'; i++) {
	    tmp = (struct link *)link_new(head);
	    tmp->let = buf[i];
	    if (rev)
		add_link_rev(&List, tmp);
	    else
		add_link(&List, tmp);
	}

	dumplist(&List);

	p = List.next;

	while (p != NULL && p->next != NULL) {
	    tmp = p->next;
	    link_dispose(head, p);
	    p = tmp;
	}
	List.next = NULL;
    }

    link_cleanup(head);

    exit(0);
}

int add_link_rev(struct link *List, struct link *link)
{
    struct link *p;

    p = List->next;
    List->next = link;
    link->next = p;
}

int add_link(struct link *List, struct link *link)
{
    struct link *p;

    p = List;
    while (p->next != NULL)
	p = p->next;
    p->next = link;
    link->next = NULL;
}

int dumplist(struct link *List)
{
    struct link *p;

    p = List->next;
    while (p != NULL) {
	putchar(p->let);
	p = p->next;
    }
    putchar('\n');
}
