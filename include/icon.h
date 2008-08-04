#ifndef GRASS_ICON_H
#ifndef _STDIO_H
#include <stdio.h>
#endif
#define GRASS_ICON_H

#define ICON struct _icon
struct _icon
{
    int nrows;
    int ncols;
    int xref;
    int yref;
    char **map;
};

char *ask_icon_any(char *, char *, int);
char *ask_icon_new(char *, char *);
char *ask_icon_old(char *, char *);
int get_icon(char *, char *, ICON *);
int get_default_icon(ICON *);
int put_icon(char *, ICON *);
int read_icon(FILE *, ICON *);
int release_icon(ICON *);
int scale_icon(ICON *, ICON *, float);
int write_icon(FILE *, ICON *, int);
#endif
