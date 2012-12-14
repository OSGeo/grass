#include <grass/gis.h>
#include <grass/imagery.h>
#include <grass/ortholib.h>
#include <grass/glocale.h>
#include "orthophoto.h"
#include "defs.h"

extern func interpolate;	/* interpolation routine */

extern int seg_mb_img, seg_mb_elev;
extern char *extension;
extern int *ref_list;
extern struct Ortho_Image_Group group;
extern char *elev_name;
extern char *elev_mapset;
extern struct Cell_head target_window;

#include "local_proto.h"
