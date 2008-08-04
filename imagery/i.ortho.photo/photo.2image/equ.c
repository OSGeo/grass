#include <grass/imagery.h>
#include "globals.h"


int Compute_equation(void)
{
    group.ref_equation_stat = I_compute_ref_equations(&group.photo_points,
						      group.E12, group.N12,
						      group.E21, group.N21);

    return 0;
}
