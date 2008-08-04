#include "globals.h"

/*
   Compute_equation()
   {
   group.equation_stat = CRS_compute_georef_equations(&group.points,
   group.E12, group.N12, group.E21, group.N21);

   return 0;
   }
 */

int CRS_Compute_equation(int order)
{
    group.equation_stat = CRS_compute_georef_equations(&group.points,
						       group.E12, group.N12,
						       group.E21, group.N21,
						       order);

    return 0;
}
