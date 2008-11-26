#include <grass/gis.h>
#include <grass/Vect.h>
#include <grass/glocale.h>

#ifdef MAIN
struct Cell_head Window;
BOUND_BOX Box;
#else
extern struct Cell_head Window;
extern BOUND_BOX Box;
#endif
