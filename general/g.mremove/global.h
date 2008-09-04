#include <grass/gis.h>
#include <grass/glocale.h>

struct list
{
    char **element;		/* list of related elements */
    char *alias;		/* element alias */
    char **desc;		/* description of elements */
    char *text;			/* menu text */
    int nelem;			/* number of elements */
    char status;
    char *mainelem;		/* main element */
    char *maindesc;		/* main element description */
};

/* read_list.c */
int read_list(int);

/* wc2regex.c */
char *wc2regex(const char *);

/* check_reclass.c */
int check_reclass(const char *, const char *, int);

/* do_remove.c */
int do_remove(int, char *);

extern int nlist;
extern struct list *list;
