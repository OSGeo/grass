#include <grass/gis.h>
#include <grass/imagery.h>
#include <grass/glocale.h>
#include "defs.h"

extern func interpolate;	/* interpolation routine */

extern int seg_mb_img;

struct Image_Group
{
    char name[GNAME_MAX];
    struct Ref ref;
    struct Control_Points control_points;
    int equation_stat;
    /* georef coefficients */
    double E12[10], N12[10], E21[10], N21[10];
    /* TPS coefficients */
    double *E12_t, *N12_t, *E21_t, *N21_t;
};

extern struct Cell_head target_window;

#include "local_proto.h"
