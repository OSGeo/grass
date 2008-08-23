
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


    /*
       List.next = NULL;
       List.let = ' ';
     */


    head = (VOID_T *) link_init(sizeof(struct link));


    for (i = 0; i < 2000000; i++) {
	/*
	   p = (struct link *) malloc (sizeof (struct link));
	   free (p);
	 */
	p = (struct link *)link_new(head);
	link_destroy(head, p);
    }

    link_cleanup(head);

    exit(0);
}
