/*!
   \file Gv3.c

   \brief OGSF library - loading vector sets (lower level functions)

   GRASS OpenGL gsurf OGSF Library 

   (C) 1999-2008 by the GRASS Development Team

   This program is free software under the 
   GNU General Public License (>=v2). 
   Read the file COPYING that comes with GRASS
   for details.

   \author Bill Brown USACERL (December 1993)
   \author Doxygenized by Martin Landa <landa.martin gmail.com> (May 2008)
 */

#include <stdlib.h>

#include <grass/gis.h>
#include <grass/Vect.h>
#include <grass/glocale.h>
#include <grass/gstypes.h>

/*
   #define TRAK_MEM
 */

#ifdef TRAK_MEM
static int Tot_mem = 0;
#endif

/*!
   \brief Load vector map to memory

   The other alternative may be to load to a tmp file

   \param grassname vector map name
   \param[out] number of loaded features

   \return pointer to geoline struct
   \return NULL on failure
 */
geoline *Gv_load_vect(const char *grassname, int *nlines)
{
    struct Map_info map;
    struct line_pnts *points;
    geoline *top, *gln, *prev;
    int np, i, n, nareas, nl = 0, area, type, is3d;
    struct Cell_head wind;
    float vect[2][3];
    const char *mapset;

    mapset = G_find_vector2(grassname, "");
    if (!mapset) {
	G_warning(_("Vector map <%s> not found"), grassname);
	return NULL;
    }

    Vect_set_open_level(2);
    if (Vect_open_old(&map, grassname, "") == -1) {
	G_warning(_("Unable to open vector map <%s>"),
		  G_fully_qualified_name(grassname, mapset));
	return NULL;
    }

    top = gln = (geoline *) G_malloc(sizeof(geoline));	/* G_fatal_error */
    if (!top) {
	return NULL;
    }

    prev = top;

#ifdef TRAK_MEM
    Tot_mem += sizeof(geoline);
#endif

    points = Vect_new_line_struct();

    G_get_set_window(&wind);
    Vect_set_constraint_region(&map, wind.north, wind.south, wind.east,
			       wind.west, PORT_DOUBLE_MAX, -PORT_DOUBLE_MAX);

    is3d = Vect_is_3d(&map);

    /* Read areas */
    n = Vect_get_num_areas(&map);
    nareas = 0;
    G_debug(3, "Reading vector areas (nareas = %d)", n);
    for (area = 1; area <= n; area++) {
	G_debug(3, " area %d", area);

	Vect_get_area_points(&map, area, points);
	if (points->n_points < 3)
	    continue;
	gln->type = OGSF_POLYGON;
	gln->npts = np = points->n_points;
	G_debug(3, "  np = %d", np);

	if (is3d) {
	    gln->dims = 3;
	    gln->p3 = (Point3 *) G_calloc(np, sizeof(Point3));	/* G_fatal_error */
	    if (!gln->p3) {
		return (NULL);
	    }
#ifdef TRAK_MEM
	    Tot_mem += (np * sizeof(Point3));
#endif
	}
	else {
	    gln->dims = 2;
	    gln->p2 = (Point2 *) G_calloc(np, sizeof(Point2));	/* G_fatal_error */
	    if (!gln->p2) {
		return (NULL);
	    }
#ifdef TRAK_MEM
	    Tot_mem += (np * sizeof(Point2));
#endif
	}

	for (i = 0; i < np; i++) {
	    if (is3d) {
		gln->p3[i][X] = points->x[i];
		gln->p3[i][Y] = points->y[i];
		gln->p3[i][Z] = points->z[i];
	    }
	    else {
		gln->p2[i][X] = points->x[i];
		gln->p2[i][Y] = points->y[i];
	    }
	}
	/* Calc normal (should be average) */
	if (is3d) {
	    vect[0][X] = (float)(gln->p3[0][X] - gln->p3[1][X]);
	    vect[0][Y] = (float)(gln->p3[0][Y] - gln->p3[1][Y]);
	    vect[0][Z] = (float)(gln->p3[0][Z] - gln->p3[1][Z]);
	    vect[1][X] = (float)(gln->p3[2][X] - gln->p3[1][X]);
	    vect[1][Y] = (float)(gln->p3[2][Y] - gln->p3[1][Y]);
	    vect[1][Z] = (float)(gln->p3[2][Z] - gln->p3[1][Z]);
	    GS_v3cross(vect[1], vect[0], gln->norm);

	}

	gln->next = (geoline *) G_malloc(sizeof(geoline));	/* G_fatal_error */
	if (!gln->next) {
	    return (NULL);
	}

#ifdef TRAK_MEM
	Tot_mem += sizeof(geoline);
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

	if (type & (GV_LINES | GV_FACE)) {
	    if (type & (GV_LINES)) {
		gln->type = OGSF_LINE;
	    }
	    else {
		gln->type = OGSF_POLYGON;
		/* Vect_append_point ( points, points->x[0], points->y[0], points->z[0] ); */
	    }

	    gln->npts = np = points->n_points;
	    G_debug(3, "  np = %d", np);

	    if (is3d) {
		gln->dims = 3;
		gln->p3 = (Point3 *) G_calloc(np, sizeof(Point3));	/* G_fatal_error */
		if (!gln->p3) {
		    return (NULL);
		}
#ifdef TRAK_MEM
		Tot_mem += (np * sizeof(Point3));
#endif
	    }
	    else {
		gln->dims = 2;
		gln->p2 = (Point2 *) G_calloc(np, sizeof(Point2));	/* G_fatal_error */
		if (!gln->p2) {
		    return (NULL);
		}
#ifdef TRAK_MEM
		Tot_mem += (np * sizeof(Point2));
#endif
	    }

	    for (i = 0; i < np; i++) {
		if (is3d) {
		    gln->p3[i][X] = points->x[i];
		    gln->p3[i][Y] = points->y[i];
		    gln->p3[i][Z] = points->z[i];
		}
		else {
		    gln->p2[i][X] = points->x[i];
		    gln->p2[i][Y] = points->y[i];
		}
	    }
	    /* Calc normal (should be average) */
	    if (is3d && gln->type == OGSF_POLYGON) {
		vect[0][X] = (float)(gln->p3[0][X] - gln->p3[1][X]);
		vect[0][Y] = (float)(gln->p3[0][Y] - gln->p3[1][Y]);
		vect[0][Z] = (float)(gln->p3[0][Z] - gln->p3[1][Z]);
		vect[1][X] = (float)(gln->p3[2][X] - gln->p3[1][X]);
		vect[1][Y] = (float)(gln->p3[2][Y] - gln->p3[1][Y]);
		vect[1][Z] = (float)(gln->p3[2][Z] - gln->p3[1][Z]);
		GS_v3cross(vect[1], vect[0], gln->norm);
		G_debug(3, "norm %f %f %f", gln->norm[0], gln->norm[1],
			gln->norm[2]);
	    }

	    gln->next = (geoline *) G_malloc(sizeof(geoline));	/* G_fatal_error */
	    if (!gln->next) {
		return (NULL);
	    }
#ifdef TRAK_MEM
	    Tot_mem += sizeof(geoline);
#endif

	    prev = gln;
	    gln = gln->next;
	    nl++;
	}
    }
    G_debug(3, "%d lines loaded", nl);

    nl += nareas;

    prev->next = NULL;
    G_free(gln);

#ifdef TRAK_MEM
    Tot_mem -= sizeof(geoline);
#endif

    Vect_close(&map);

    if (!nl) {
	G_warning(_("No features from vector map <%s> fall within current region"),
		  G_fully_qualified_name(grassname, mapset));
	return (NULL);
    }
    else {
	G_message(_("Vector map <%s> loaded (%d features)"),
		  G_fully_qualified_name(grassname, mapset), nl);
    }

    *nlines = nl;

#ifdef TRAK_MEM
    G_debug(3, "Total vect memory = %d Kbytes", Tot_mem / 1000);
#endif

    return (top);
}

/*! 
   \brief Tracking memory 

   \param minus mimus number 
 */
void sub_Vectmem(int minus)
{
#ifdef TRAK_MEM
    {
	Tot_mem -= minus;
    }
#endif

    return;
}
