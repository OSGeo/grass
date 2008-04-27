/******************************************************************************
* hull.c <s.hull>
* Creates the convex hull surrounding a sites list

* @Copyright Andrea Aime <aaime@libero.it>
* 23 Sept. 2001
* Updated 19 Dec 2003, Markus Neteler to 5.7
* Last updated 16 jan 2007, Benjamin Ducke to support 3D hull creation

* This file is part of GRASS GIS. It is free software. You can
* redistribute it and/or modify it under the terms of
* the GNU General Public License as published by the Free Software
* Foundation; either version 2 of the License, or (at your option)
* any later version.

* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
******************************************************************************/


#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <grass/gis.h>
#include <grass/Vect.h>
#include <grass/glocale.h>

#include "chull.h"

struct Point {
   double x;
   double y;
   double z;
};

int rightTurn(struct Point *P, int i, int j, int k) {
    double a, b, c, d;
    a = P[i].x - P[j].x;
    b = P[i].y - P[j].y;
    c = P[k].x - P[j].x;
    d = P[k].y - P[j].y;
    return a*d - b*c < 0;	
}

int cmpPoints(const void* v1, const void* v2) {
    struct Point *p1, *p2;
    p1 = (struct Point*) v1;
    p2 = (struct Point*) v2;
    if( p1->x > p2->x )
        return 1;
    else if( p1->x < p2->x )
        return -1;
    else
        return 0;
}

int convexHull(struct Point* P, const int numPoints, int **hull) {
    int pointIdx, upPoints, loPoints;
    int *upHull, *loHull;

    /* sort points in ascending x order*/
    qsort(P, numPoints, sizeof(struct Point), cmpPoints);

    *hull = (int*) G_malloc(numPoints * 2 * sizeof(int));

    /* compute upper hull */
    upHull = *hull;
    upHull[0] = 0;
    upHull[1] = 1;
    upPoints = 1;
    for(pointIdx = 2; pointIdx < numPoints; pointIdx++) {
        upPoints++;
        upHull[upPoints] = pointIdx;
        while( upPoints > 1 &&
               !rightTurn(P, upHull[upPoints], upHull[upPoints-1],
                             upHull[upPoints-2])
             ) {
            upHull[upPoints-1] = upHull[upPoints];
            upPoints--;
        }
    }

    /*
    printf("upPoints: %d\n", upPoints);
    for(pointIdx = 0; pointIdx <= upPoints; pointIdx ++)
        printf("%d ", upHull[pointIdx]);
    printf("\n");
    */

    /* compute lower hull, overwrite last point of upper hull */
    loHull = &(upHull[upPoints]);
    loHull[0] = numPoints - 1;
    loHull[1] = numPoints - 2;
    loPoints = 1;
    for(pointIdx = numPoints - 3; pointIdx >= 0; pointIdx--) {
        loPoints++;
        loHull[loPoints] = pointIdx;
        while( loPoints > 1 &&
               !rightTurn(P, loHull[loPoints], loHull[loPoints-1],
                             loHull[loPoints-2])
             ) {
             loHull[loPoints-1] = loHull[loPoints];
             loPoints--;
        }
    }

    G_debug(3, "numPoints:%d loPoints:%d upPoints:%d",
                numPoints, loPoints, upPoints);
    /*
    printf("loPoints: %d\n", loPoints);
    for(pointIdx = 0; pointIdx <= loPoints; pointIdx ++)
        printf("%d ", loHull[pointIdx]);
    printf("\n");
    */

    /* reclaim uneeded memory */
    *hull = (int *) G_realloc(*hull, (loPoints + upPoints) * sizeof(int));
    return loPoints + upPoints;
}



void convexHull3d(struct Point* P, const int numPoints, struct Map_info *Map ) {

	int error;
	int i;
	double *px;
	double *py;
	double *pz;
	
	px = G_malloc ( sizeof (double) * numPoints );
	py = G_malloc ( sizeof (double) * numPoints );
	pz = G_malloc ( sizeof (double) * numPoints );
	
	for ( i=0; i < numPoints; i++ ) {
		px[i] = (P)[i].x;
		py[i] = (P)[i].y;
		pz[i] = (P)[i].z;
	}
	
	/* make 3D hull */
	error = make3DHull ( px, py, pz, numPoints, Map );
	if ( error < 0 ) {	
		G_fatal_error ("Simple planar hulls not implemented yet");
	}
	
	G_free ( px );
	G_free ( py );
	G_free ( pz );

}

#define ALLOC_CHUNK 256
int loadSiteCoordinates(struct Map_info *Map, struct Point **points, int all,
			struct Cell_head *window)
{
    int pointIdx = 0;
    struct line_pnts* sites;
    struct line_cats* cats;
    BOUND_BOX box;
    int cat, type;

    sites = Vect_new_line_struct();
    cats = Vect_new_cats_struct ();

    *points = NULL;

    /* copy window to box */
    Vect_region_box (window, &box);

    while ((type = Vect_read_next_line (Map, sites, cats)) > -1) {
	
	if (type != GV_POINT)
	    continue;

	Vect_cat_get (cats, 1, &cat);
	
	G_debug (4, "Point: %f|%f|%f|#%d", sites->x[0], sites->y[0], sites->z[0], cat);
	
	if (all || 
	    Vect_point_in_box(sites->x[0], sites->y[0], sites->z[0], &box)) {
	    
	    G_debug (4, "Point in the box");
	    
	    if ((pointIdx % ALLOC_CHUNK) == 0)
		*points = (struct Point *) G_realloc(*points, (pointIdx + ALLOC_CHUNK) * sizeof(struct Point));
	    
            (*points)[pointIdx].x = sites->x[0];
            (*points)[pointIdx].y = sites->y[0];
	    (*points)[pointIdx].z = sites->z[0];
            pointIdx++;
        }
    }

    if(pointIdx > 0)
        *points = (struct Point *) G_realloc(*points, (pointIdx + 1) * sizeof(struct Point));
    return pointIdx;
}

/*
 * Outputs the points that comprises the convex hull as a single closed line
 * and the hull baricenter as the label points (as it is a linear combination
 * of points on the hull is guaranteed to be inside the hull, follow from the
 * definition of convex polygon)
 */
int outputHull(struct Map_info *Map, struct Point* P, int *hull,
               int numPoints) {
    struct line_pnts *Points;
    struct line_cats *Cats;
    double *tmpx, *tmpy;
    int i, pointIdx;
    double xc, yc;
    
    tmpx = (double *) G_malloc((numPoints + 1) * sizeof(double));
    tmpy = (double *) G_malloc((numPoints + 1) * sizeof(double));

    xc = yc = 0;
    for(i = 0; i < numPoints; i++) {
        pointIdx = hull[i];
        tmpx[i] = P[pointIdx].x;
        tmpy[i] = P[pointIdx].y;
        /* average coordinates calculation... may introduce a little
           numerical error but guaratees that no overflow will occurr */
        xc = xc + tmpx[i] / numPoints;
        yc = yc + tmpy[i] / numPoints;
    }
    tmpx[numPoints] = P[hull[0]].x;
    tmpy[numPoints] = P[hull[0]].y;

    Points = Vect_new_line_struct ();
    Cats = Vect_new_cats_struct ();
    Vect_copy_xyz_to_pnts(Points, tmpx, tmpy, 0, numPoints+1);
    G_free(tmpx);
    G_free(tmpy);

    /* write out convex hull */
    Vect_write_line (Map, GV_BOUNDARY, Points, Cats);
    
    /* find and add centroid */
    Vect_reset_line (Points);
    Vect_append_point (Points,xc, yc, 0.0);
    Vect_cat_set ( Cats, 1, 1 );
    Vect_write_line (Map, GV_CENTROID, Points, Cats);
    Vect_destroy_line_struct (Points);

    return 0;
}

int main(int argc, char **argv) {
    struct GModule *module;
    struct Option *input, *output;
    struct Flag *all, *flat;
    struct Cell_head window;

    char *mapset;
    char *sitefile;

    struct Map_info Map;
    struct Point *points;  /* point loaded from site file */
    int *hull;   /* index of points located on the convex hull */
    int numSitePoints, numHullPoints;

    int MODE2D;
        

    G_gisinit (argv[0]);

    module = G_define_module();
    module->keywords = _("vector, geometry");
    module->description = _("Uses a GRASS vector points map to produce a convex hull vector map.");

    input = G_define_standard_option (G_OPT_V_INPUT);
    input->description = _("Name of input vector points map");

    output = G_define_standard_option (G_OPT_V_OUTPUT);
    output->description = _("Name of output vector area map");

    all = G_define_flag ();
    all->key = 'a';
    all->description = _("Use all vector points (do not limit to current region)");

    flat = G_define_flag ();
    flat->key = 'f';
    flat->description = _("Create a 'flat' 2D hull even if the input is 3D points");

    if (G_parser (argc, argv))
	exit (EXIT_FAILURE);

    sitefile = input->answer;

    mapset = G_find_vector2 (sitefile, "");
    if (mapset == NULL)
        G_fatal_error (_("Vector map <%s> not found"), sitefile);

    Vect_check_input_output_name (input->answer, output->answer,
				  GV_FATAL_EXIT);

    /* open site file */
    if (Vect_open_old(&Map, sitefile, mapset) < 0)
        G_fatal_error (_("Cannot open vector map <%s>"), sitefile);

    /* load site coordinates */
    G_get_window (&window);
    numSitePoints = loadSiteCoordinates(&Map, &points, all->answer, &window);
    if(numSitePoints < 0 )
        G_fatal_error (_("Error loading vector points map <%s>"), sitefile);

    if(numSitePoints < 3 )
        G_fatal_error (_("Convex hull calculation requires at least three points"));


    /* create a 2D or a 3D hull? */
    MODE2D = 1;
    if ( Vect_is_3d ( &Map ) ) {
    	MODE2D = 0;
    }
    if ( flat->answer ) {
    	MODE2D = 1;
    }


    /* create vector map */
    if ( MODE2D ) {
    	if (0 > Vect_open_new (&Map, output->answer, 0 ) ) {
        	G_fatal_error (_("Cannot open vector map <%s>"), output->answer);
	}
    } else {
    	if (0 > Vect_open_new (&Map, output->answer, 1 ) ) {
        	G_fatal_error (_("Cannot open vector map <%s>"), output->answer);
	}  
    }

    Vect_hist_command ( &Map );

    if ( MODE2D ) {
    
       /* compute convex hull */
       numHullPoints = convexHull(points, numSitePoints, &hull);
    
       /* output vector map */
       outputHull(&Map, points, hull, numHullPoints);

    } else {

	/* this does everything for the 3D hull including vector map creation */     
    	convexHull3d (points, numSitePoints, &Map );

    }
    

    /* clean up and bye bye */
    Vect_build (&Map, stderr);
    Vect_close (&Map);

    exit (EXIT_SUCCESS);
}
