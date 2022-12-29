#ifndef __LOCAL_PROTO_H__
#define __LOCAL_PROTO_H__

#include <stdio.h>
#include <grass/gis.h>
#include <grass/dbmi.h>

int points_analyse(FILE *, FILE *, char *, char *, int *, int *, int *, int *, int **,
		   char ***, int **, int, int, int, int, int, int, int);

int points_to_bin(FILE *, int, struct Map_info *, dbDriver *,
		  char *, char *, char *, int, int *, int, int, int, int, int);

#endif /* __LOCAL_PROTO_H__ */
