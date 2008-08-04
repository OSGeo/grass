#ifndef __LOCAL_PROTO_H__
#define __LOCAL_PROTO_H__


#include <stdio.h>
#include <grass/gis.h>
#include <grass/dbmi.h>


#define FORMAT_POINT 0
#define FORMAT_ALL   1


int points_analyse(FILE *, FILE *, char *, int *, int *, int *, int *, int **,
		   int **, int, int, int, int);

int points_to_bin(FILE *, int, struct Map_info *, dbDriver *,
		  char *, char *, int, int, int *, int, int, int, int, int);

int read_head(FILE *, struct Map_info *);

int asc_to_bin(FILE *, struct Map_info *);


#endif /* __LOCAL_PROTO_H__ */
