#include <assert.h>

#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/dbmi.h>
#include <grass/raster.h>
#include <grass/glocale.h>
#include <grass/lidar.h>

struct lidar_cat
{
    int cat_edge;     /* category in layer F_EDGE_DETECTION_CLASS */
    int cat_class;    /* category in layer F_CLASSIFICATION */
    int cat_interp;   /* category in layer F_INTERPOLATION */
    int cat_obj;      /* category in layer F_COUNTER_OBJ */
};

void
P_Sparse_Correction(struct Map_info *, /**/
		    struct Map_info *, /**/
		    struct Map_info *, /**/
		    struct Cell_head *, /**/
		    struct bound_box, /**/
		    struct bound_box, /**/
		    double **, /**/
		    struct lidar_cat *,
		    double *, /**/
		    int *, /**/
		    double, /**/
		    double, /**/
		    double, /**/
		    double, /**/
		    double, /**/
		    int, /**/
		    int, /**/
		    int, /**/
		    dbDriver *, /**/
		    double, /**/
		    char *);

int Insert_Correction(double, /**/ int, /**/ dbDriver *, /**/ char *);

int UpDate_Correction(double, /**/ int, /**/ dbDriver *, /**/ char *);

int Select_Correction(double *, /**/ int, /**/ dbDriver *, /**/ char *);

struct Point *P_Read_Vector_Correction(struct Map_info *, /**/
				       struct Cell_head *, /**/
				       int *, /**/ int *, /**/ int /**/,
				       struct lidar_cat **);

int correction(int, double, double, double, double);
