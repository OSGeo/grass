#ifndef GRASS_MANAGE_H
#define GRASS_MANAGE_H

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

#define REMOVE 1
#define RENAME 2
#define COPY   3
#define LIST   4

#include <grass/gis.h>

#include <grass/defs/manage.h>

#endif
