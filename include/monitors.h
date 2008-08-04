#ifndef _GRASS_MONITORS_H
#define _GRASS_MONITORS_H

struct MON_CAP
{
    char *name;			/* name of monitor */
    char *path;			/* full path name to execute monitor */
    char *comment;		/* like this */
    char *link;			/* were monitor interfaces to gis */
    char *tty;			/* where monitor must be run from */
    char *where;		/* version of above for user */
};

#define MON_NEXT 1
#define MON_NAME 2
#define MON_PATH 3
#define MON_LINK 4
#define MON_CLOSE 5

#endif
