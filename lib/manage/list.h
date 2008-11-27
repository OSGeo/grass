
#ifndef GRASS_LIST_H
#define GRASS_LIST_H

#include <grass/gis.h>
#include <grass/glocale.h>

struct list
{
    const char **element;	/* list of related elements */
    char *alias;		/* element alias */
    const char **desc;		/* description of elements */
    char *text;			/* menu text */
    int nelem;			/* number of elements */
    char status;
    char *mainelem;		/* main element */
    char *maindesc;		/* main element description */
};

extern int nlist;
extern struct list *list;

#define REMOVE 1
#define RENAME 2
#define COPY   3
#define LIST   4

/* add_elem.c */
int add_element(const char *, const char *);

/* do_copy.c */
int do_copy(int, const char *, const char *, const char *);

/* do_list.c */
void do_list(int, const char *);

/* do_remove.c */
int do_remove(int, const char *);

/* do_rename.c */
int do_rename(int, const char *, const char *);

/* empty.c */
int empty(char *);

/* find.c */
const char *find(int, char *, const char *);

/* get_len.c */
int get_description_len(int);

/* read_list.c */
int read_list(int);

/* show_elem.c */
int show_elements(void);

/* sighold.c */
int hold_signals(int);

#endif

