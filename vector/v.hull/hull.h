#include <grass/vector.h>

struct Point
{
    double x;
    double y;
    double z;
};

#define ALLOC_CHUNK 256

struct cat_list *parse_filter_options(struct Map_info *, int , char *,
                                    char *);

int loadSiteCoordinates(struct Map_info *, struct Point **, int,
			struct Cell_head *, int, struct cat_list *);
  
int convexHull(struct Point *, int, int **);

int outputHull(struct Map_info *, struct Point *, int *,
	       int);

int make3DHull(double *, double *, double *, int,
	       struct Map_info *);

void convexHull3d(struct Point *, int, struct Map_info *);
