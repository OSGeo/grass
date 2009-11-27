#include <grass/vector.h>

struct Point
{
    double x;
    double y;
    double z;
};

#define ALLOC_CHUNK 256

int loadSiteCoordinates(struct Map_info *, struct Point **, int,
			struct Cell_head *, int);
  
int convexHull(struct Point *, int, int **);

int outputHull(struct Map_info *, struct Point *, int *,
	       int);

int make3DHull(double *, double *, double *, int,
	       struct Map_info *);

void convexHull3d(struct Point *, int, struct Map_info *);
