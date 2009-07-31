#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/glocale.h>

#ifdef MAIN
struct Cell_head Window;
struct bound_box Box;
#else
extern struct Cell_head Window;
extern struct bound_box Box;
#endif
