#include <grass/raster.h>

#ifndef _LOCAL_PROTO_H
#define _LOCAL_PROTO_H

#define PI   3.1415926535897932384626433832795
#define R2D 57.295779513082320877
#define D2R  0.017453292519943295769

typedef struct
{
    int fd;
    char name[GNAME_MAX];
    void *rast;
    RASTER_MAP_TYPE type;
} Gfile;

#define LAMBERTIAN		 	 0
#define COSINE			 	 1
#define PERCENT				 2
#define NON_LAMBERTIAN			10
#define MINNAERT			11
#define C_CORRECT			12

void eval_cosi(Gfile *, Gfile *, double, double);
void eval_tcor(int, Gfile *, Gfile *, Gfile *, double, int);

#endif
