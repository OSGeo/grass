#include <grass/gis.h>
#include <grass/raster.h>

#ifndef _LOCAL_PROTO_H
#define _LOCAL_PROTO_H

#define PI   M_PI
#define R2D  M_R2D
#define D2R  M_D2R

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
