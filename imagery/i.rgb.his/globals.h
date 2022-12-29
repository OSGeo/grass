#ifndef __GLOBALS_H__
#define __GLOBALS_H__

#include <grass/raster.h>

/* closefiles.c */
int closefiles(char *, char *, char *, int[3], CELL *[3]);

/* openfiles.c */
void openfiles(char *, char *, char *, char *, char *, char *,
	       int[3], int[3], CELL *[3]);
/* rgb2his.c */
void rgb2his(CELL *[3], int);

#endif /* __GLOBALS_H__ */
