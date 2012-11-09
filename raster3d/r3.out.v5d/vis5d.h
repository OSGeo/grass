
/* Vis5D version 5.0 */

/*
   Vis5D system for visualizing five dimensional gridded data sets
   Copyright (C) 1990-1997 Bill Hibbard, Brian Paul, Dave Santek,
   and Andre Battaiola.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 1, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */


/*
 * This configuration file contains options which can be safely
 * changed by the user.
 */



#ifndef VIS5D_H
#define VIS5D_H


/*
 * Amount of physical RAM in megabytes:
 * vis5d normally uses a bounded amount of memory to avoid swapping.
 * When the limit is reached, the least-recently-viewed graphics will
 * be deallocated.  If MBS is set to 0, however, vis5d will use ordinary
 * malloc/free and not deallocate graphics (ok for systems with a lot
 * of memory (>=128MB)).
 */
#define MBS 32



/* Default topography file: */
#define TOPOFILE "EARTH.TOPO"


/* Default map lines files: */
#define WORLDFILE "OUTLSUPW"
#define USAFILE "OUTLUSAM"


/* Default directory to search for user functions: */
#define FUNCTION_PATH "userfuncs"


/* Default animation rate in milliseconds: */
#define ANIMRATE 100


/* Default scale and exponent values for logrithmic vertical coordinate system: */
#define DEFAULT_LOG_SCALE  1012.5
#define DEFAULT_LOG_EXP  -7.2



/**********************************************************************/

/**********************************************************************/

/***          USERS:  DON'T CHANGE ANYTHING BEYOND THIS POINT       ***/

/**********************************************************************/

/**********************************************************************/

/*
 * Define BIG_GFX to allow larger isosurfaces, contour slices, etc. if
 * there's enough memory.
 #if MBS==0 || MBS>=128
 #  define BIG_GFX
 #endif
 */

#define BIG_GFX


/*
 * Shared by code above and below API:
 */
#define MAX_LABEL   1000
#define MAX_FUNCS   100



#endif
