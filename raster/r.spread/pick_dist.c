
/******************************************************************************
 *
 *	pick_dist.c  inverse square distance distributed probability generator
 *	
 * Usage: pick_dist (maxnumber)
 *
 * Notes: prob_invsqr() generates a decreasing probability distribution 
 * outward in an inverse square rate. It use three consecutive random number
 * generator: 
 *	applying it once gets an UNIFORM distribution in the range of 0-max_num;
 *	doing it twice gets a SIMPLE INVERSE distribuion in that range;
 *	doing three times gets a INVERSE SQUARE distribution.
 * 
 * Author: Jianping Xu, Rutgers University
 * Date: 06/11/1994
 ******************************************************************************/
#include <stdlib.h>
#include <limits.h>
#include <math.h>
#include "local_proto.h"

int pick_dist(int u)
{
    int v;

    v = (int)((u + 0.99999999999) * rand() / INT_MAX);
    u = (int)((v + 0.99999999999) * rand() / INT_MAX);
    return ((int)((u + 0.99999999999) * rand() / INT_MAX));	/*4th for a test */
}
