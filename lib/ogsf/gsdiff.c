/*!
  \file gsdiff.c
 
  \brief OGSF library - loading and manipulating surfaces
 
  GRASS OpenGL gsurf OGSF Library 
 
  Routines to set up automatic on-the-fly recalculation
  of surface elevations, doing a "scaled difference" using another
  surface for reference
  
  Note that we're using a true difference here, between data set values,
  no translations, etc.
  
  \todo generalize this concept to allow transform functions which are
  dependent on surfaces that are dependent on other surfaces, etc., as long
  as the dependency doesn't loop back.

  (C) 1999-2008 by the GRASS Development Team
 
  This program is free software under the 
  GNU General Public License (>=v2). 
  Read the file COPYING that comes with GRASS
  for details.
  
  \author Bill Brown USACERL, GMSL/University of Illinois (November 1994)
*/

#include <stdio.h>

#include <grass/gstypes.h>
#include "gsget.h"

static geosurf *Refsurf = NULL;
static typbuff *Refbuff = NULL;
static float Refscale = 1.0;

/***********************************************************************/
void gsdiff_set_SDscale(float scale)
{
    Refscale = scale;

    return;
}

/***********************************************************************/
float gsdiff_get_SDscale(void)
{
    return (Refscale);
}

/***********************************************************************/
void gsdiff_set_SDref(geosurf * gsref)
{
    Refsurf = gsref;
    Refbuff = gs_get_att_typbuff(gsref, ATT_TOPO, 0);

    return;
}

/***********************************************************************/
geosurf *gsdiff_get_SDref(void)
{
    if (Refsurf && Refbuff) {
	return (Refsurf);
    }

    return (NULL);
}

/***********************************************************************/
float gsdiff_do_SD(float val, int offset)
{
    float ref;

    if (Refbuff) {
	GET_MAPATT(Refbuff, offset, ref);
	return (ref + (val - ref) * Refscale);
    }

    return (val);
}
