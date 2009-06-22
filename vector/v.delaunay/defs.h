#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/glocale.h>

#ifdef MAIN
struct Cell_head Window;
BOUND_BOX Box;
#else
extern struct Cell_head Window;
extern BOUND_BOX Box;
#endif
