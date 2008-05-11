/*!
  \file Gv3.c
 
  \brief OGSF library - loading and manipulating vector sets
 
  GRASS OpenGL gsurf OGSF Library 
 
  (C) 1999-2008 by the GRASS Development Team
 
  This program is free software under the 
  GNU General Public License (>=v2). 
  Read the file COPYING that comes with GRASS
  for details.
  
  \author Bill Brown USACERL (December 1993)
*/

#include <stdlib.h>
#include <grass/gis.h>
#include <grass/Vect.h>

#include <grass/gstypes.h>

#define TRAK_MEM

#ifdef TRAK_MEM
static int Tot_mem = 0;
#endif

/* This loads to memory.  
The other alternative may be to load to a tmp file. */
geoline *Gv_load_vect(char *grassname, int *nlines)
{
    struct Map_info map;
    struct line_pnts *points;
    geoline *top, *gln, *prev;
    int np, i, n, nareas, nl=0, area, type, is3d;
    struct Cell_head  wind;
    float vect[2][3];

    /* TODO: handle error messages */

    Vect_set_open_level (2); 
    Vect_open_old (&map, grassname, "");
    
    if (NULL == (top=gln=(geoline *)malloc(sizeof(geoline)))) {
	fprintf(stderr,"Can't malloc.\n");
	return(NULL);
    }
    prev = top;
    
    #ifdef TRAK_MEM
    	Tot_mem+=sizeof(geoline);
    #endif

    points = Vect_new_line_struct ();

    G_get_set_window (&wind) ;
    Vect_set_constraint_region(&map,wind.north,wind.south,wind.east,wind.west,
	                       PORT_DOUBLE_MAX, -PORT_DOUBLE_MAX);

    is3d = Vect_is_3d ( &map );
    
    /* Read areas */
    n = Vect_get_num_areas (&map);
    nareas = 0;
    G_debug(3, "Reading vector areas (nareas = %d)", n);
    for ( area = 1; area <= n ; area++ ) {
	G_debug(3, " area %d", area);

	Vect_get_area_points ( &map, area, points ); 
	if ( points->n_points < 3 ) continue;
	gln->type = OGSF_POLYGON;
	gln->npts = np = points->n_points;
        G_debug(3, "  np = %d", np);
		
	if ( is3d ) { 
	    gln->dims = 3; 
	    if (NULL == (gln->p3=(Point3 *)calloc(np, sizeof(Point3)))) {
		fprintf(stderr,"Can't calloc.\n"); /* CLEAN UP */
		return(NULL);
	    }
	    #ifdef TRAK_MEM
		Tot_mem+=(np*sizeof(Point3));
	    #endif
	} else {
	    gln->dims = 2;
	    if (NULL == (gln->p2=(Point2 *)calloc(np, sizeof(Point2)))) {
		fprintf(stderr,"Can't calloc.\n"); /* CLEAN UP */
		return(NULL);
	    }
	    #ifdef TRAK_MEM
		Tot_mem+=(np*sizeof(Point2));
	    #endif
	}
    
	for (i=0; i < np; i++) {
	    if ( is3d ) {
		gln->p3[i][X] = points->x[i];
		gln->p3[i][Y] = points->y[i];
		gln->p3[i][Z] = points->z[i];
	    } else {
		gln->p2[i][X] = points->x[i];
		gln->p2[i][Y] = points->y[i];
	    }
	}
	/* Calc normal (should be average) */
	if ( is3d ) {
	    vect[0][X] = (float) ( gln->p3[0][X] - gln->p3[1][X] );
	    vect[0][Y] = (float) ( gln->p3[0][Y] - gln->p3[1][Y] ); 
            vect[0][Z] = (float) ( gln->p3[0][Z] - gln->p3[1][Z] );
	    vect[1][X] = (float) ( gln->p3[2][X] - gln->p3[1][X] );
	    vect[1][Y] = (float) ( gln->p3[2][Y] - gln->p3[1][Y] );
	    vect[1][Z] = (float) ( gln->p3[2][Z] - gln->p3[1][Z] );
	    GS_v3cross( vect[1], vect[0], gln->norm );
	    
	}
    
	if (NULL == (gln->next=(geoline *)malloc (sizeof(geoline)))) {
	    fprintf(stderr,"Can't malloc.\n"); /* CLEAN UP */
	    return(NULL);
	}

	#ifdef TRAK_MEM
	    Tot_mem+=sizeof(geoline);
	#endif
    
	prev = gln;
	gln = gln->next;
	nareas++;
    }
    G_debug(3, "%d areas loaded", nareas);

    /* Read all lines */
    G_debug(3, "Reading vector lines ...");
    while (-1 < (type = Vect_read_next_line(&map, points, NULL))) {
	G_debug(3, "line type = %d", type);

	if ( type & ( GV_LINES | GV_FACE ) ) { 
	    if ( type & ( GV_LINES ) ) {
		gln->type = OGSF_LINE;
	    } else {
		gln->type = OGSF_POLYGON;
                /* Vect_append_point ( points, points->x[0], points->y[0], points->z[0] ); */
	    }

	    gln->npts = np = points->n_points;
            G_debug(3, "  np = %d", np);
		
	    if ( is3d ) { 
		gln->dims = 3; 
		if (NULL == (gln->p3=(Point3 *)calloc(np, sizeof(Point3)))) {
		    fprintf(stderr,"Can't calloc.\n"); /* CLEAN UP */
		    return(NULL);
		}
		#ifdef TRAK_MEM
		    Tot_mem+=(np*sizeof(Point3));
		#endif
	    } else {
		gln->dims = 2;
		if (NULL == (gln->p2=(Point2 *)calloc(np, sizeof(Point2)))) {
		    fprintf(stderr,"Can't calloc.\n"); /* CLEAN UP */
		    return(NULL);
		}
		#ifdef TRAK_MEM
		    Tot_mem+=(np*sizeof(Point2));
		#endif
	    }

	
	    for (i=0; i < np; i++) {
		if ( is3d ) {
		    gln->p3[i][X] = points->x[i];
		    gln->p3[i][Y] = points->y[i];
		    gln->p3[i][Z] = points->z[i];
		} else {
		    gln->p2[i][X] = points->x[i];
		    gln->p2[i][Y] = points->y[i];
		}
	    }
	    /* Calc normal (should be average) */
	    if ( is3d && gln->type == OGSF_POLYGON ) {
		vect[0][X] = (float) ( gln->p3[0][X] - gln->p3[1][X] );
		vect[0][Y] = (float) ( gln->p3[0][Y] - gln->p3[1][Y] ); 
		vect[0][Z] = (float) ( gln->p3[0][Z] - gln->p3[1][Z] );
		vect[1][X] = (float) ( gln->p3[2][X] - gln->p3[1][X] );
		vect[1][Y] = (float) ( gln->p3[2][Y] - gln->p3[1][Y] );
		vect[1][Z] = (float) ( gln->p3[2][Z] - gln->p3[1][Z] );
		GS_v3cross( vect[1], vect[0], gln->norm );
                G_debug ( 3, "norm %f %f %f", gln->norm[0], gln->norm[1], gln->norm[2] );
	    }
	
	    if (NULL == (gln->next=(geoline *)malloc (sizeof(geoline)))) {
		fprintf(stderr,"Can't malloc.\n"); /* CLEAN UP */
		return(NULL);
	    }

	    #ifdef TRAK_MEM
		Tot_mem+=sizeof(geoline);
	    #endif
	
	    prev = gln;
	    gln = gln->next;
	    nl++;
	}
    }
    G_debug(3, "%d lines loaded", nl);

    nl += nareas;
    
    prev->next = NULL;
    free(gln);
    
    #ifdef TRAK_MEM
    	Tot_mem-=sizeof(geoline);
    #endif
    
    Vect_close (&map);

    fprintf(stderr,"Vector file %s loaded.\n",grassname);
    if (!nl) {
	    fprintf(stderr, "Error: No lines from %s fall within current region\n", grassname);
	    return (NULL);
    }
    *nlines = nl;

    #ifdef TRAK_MEM
    	fprintf(stderr,"Total vect memory = %d Kbytes\n", Tot_mem/1000);
    #endif

    return(top);
}

void add_Vectmem(int plus)
{
    #ifdef TRAK_MEM
    {
    	Tot_mem+=plus;
    }
    #endif
    
    return;
}

void sub_Vectmem(int minus)
{
    #ifdef TRAK_MEM
    {
    	Tot_mem-=minus;
    }
    #endif
    
    return;
}

void show_Vectmem(void)
{
    #ifdef TRAK_MEM
    {
    	fprintf(stderr,"Total vect memory = %d Kbytes\n", Tot_mem/1000);
    }
    #endif
    
    return;
}


