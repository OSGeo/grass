#ifndef __GLOBALS_H__
#define __GLOBALS_H__

#include <grass/raster.h>

/* closefiles.c */
int closefiles(char *, char *, char *, int[3], CELL *[3]);

/* his2rgb.c */
void his2rgb(CELL *[3], int);

/* openfiles.c */
void openfiles(char *, char *, char *, char *, char *, char *,
	       int[3], int[3], CELL *[3]);

#endif /* __GLOBALS_H__ */
