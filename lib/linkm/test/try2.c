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
#include <stdlib.h>
#include <string.h>

#include <grass/linkm.h>

struct link {
    char let;
    struct link *next;
};

static void add_link_rev(struct link *, struct link *);
static void add_link(struct link *, struct link *);
static void dumplist(struct link *);

int main(int argc, char *argv[])
{
    int i;
    struct link_head *head;
    struct link List, *tmp, *p;
    int rev = 0;
    char buf[4096];

    if (argc == 2)
        rev = 1;

    List.next = NULL;
    List.let = ' ';

    link_set_chunk_size(1);
    head = link_init(sizeof(struct link));

    while (fgets(buf, sizeof(buf), stdin) != NULL) {
        buf[strcspn(buf, "\n")] = '\0';
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
            link_dispose(head, (VOID_T *)p);
            p = tmp;
        }
        List.next = NULL;
    }

    link_cleanup(head);

    exit(EXIT_SUCCESS);
}

static void add_link_rev(struct link *List, struct link *link)
{
    struct link *p;

    p = List->next;
    List->next = link;
    link->next = p;
}

static void add_link(struct link *List, struct link *link)
{
    struct link *p;

    p = List;
    while (p->next != NULL)
        p = p->next;
    p->next = link;
    link->next = NULL;
}

static void dumplist(struct link *List)
{
    struct link *p;

    p = List->next;
    while (p != NULL) {
        putchar(p->let);
        p = p->next;
    }
    putchar('\n');
}
