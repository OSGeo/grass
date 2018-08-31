/*
 **  Written by David Gerdes  US Army Construction Engineering Research Lab
 **     April 1992
 **  Copyright 1992 USA-CERL   All rights reserved.
 **
 */

/*
 **  takes 1st command line argument and stuffs each letter of it into
 **   a linked list.  then prints it back out to stdout.
 **  If a second argument is specified, the first argument is put in the
 **   list backwards.
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

    if (argc < 2)
	fprintf(stderr, "Usage: %s str [rev]\n", argv[0]), exit(1);

    if (argc > 2)
	rev = 1;


    List.next = NULL;
    List.let = ' ';


    head = (VOID_T *) link_init(sizeof(struct link));

    for (i = 0; argv[1][i]; i++) {
	tmp = (struct link *)link_new(head);
	tmp->let = argv[1][i];
	if (rev)
	    add_link_rev(&List, tmp);
	else
	    add_link(&List, tmp);
    }

    dumplist(&List);

    p = List.next;
    while (p->next != NULL) {
	tmp = p->next;
	link_dispose(head, p);
	p = tmp;
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
