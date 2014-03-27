#include "io.h"
#include "local_vars.h"

/* snap.c */
int create_distance_mask(int);
int read_points(char *, SEGMENT *, SEGMENT *);
int snap_point(OUTLET *, int, SEGMENT *, SEGMENT *, double);

/* points_io.c */
int write_points(char *, int);
