/*!
   \file Gp3.c

   \brief OGSF library - loading point sets (lower level functions)

   GRASS OpenGL gsurf OGSF Library 

   (C) 1999-2008 by the GRASS Development Team

   This program is free software under the 
   GNU General Public License (>=v2). 
   Read the file COPYING that comes with GRASS
   for details.

   \author Bill Brown USACERL, GMSL/University of Illinois (January 1994)
   \author Doxygenized by Martin Landa <landa.martin gmail.com> (May 2008)
 */

#include <stdlib.h>

#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/site.h>
#include <grass/vector.h>
#include <grass/glocale.h>
#include <grass/gstypes.h>

/*!
   \brief Load to points to memory

   The other alternative may be to load to a tmp file.

   \param grassname vector point map 
   \param nsites
   \param has_z 2D or 3D points?

   \return pointer to geopoint struct
   \return NULL on failure
 */
geopoint *Gp_load_sites(const char *grassname, int *nsites, int *has_z)
{
    struct Map_info map;
    static struct line_pnts *Points = NULL;
    struct line_cats *Cats = NULL;
    geopoint *top, *gpt, *prev;
    int np, ltype, eof;
    struct Cell_head wind;
    RASTER_MAP_TYPE rtype;
    int ndim;
    const char *mapset;

    np = 0;
    eof = 0;
    *has_z = 0;

    mapset = G_find_vector2(grassname, "");
    if (!mapset) {
	G_warning(_("Vector map <%s> not found"), grassname);
	return NULL;
    }

    Vect_set_open_level(2);
    if (Vect_open_old(&map, grassname, "") == -1) {
	G_fatal_error(_("Unable to open vector map <%s>"),
		      G_fully_qualified_name(grassname, mapset));
    }

    Points = Vect_new_line_struct();
    Cats = Vect_new_cats_struct();

    top = gpt = (geopoint *) G_malloc(sizeof(geopoint));
    if (!top) {
	return (NULL);
    }

    G_get_set_window(&wind);

    /* get ndim */
    ndim = 2;
    if (Vect_is_3d(&map)) {
	ndim = 3;
    }

    /* set rtype */
    rtype = CELL_TYPE;

    while (eof == 0) {
	ltype = Vect_read_next_line(&map, Points, Cats);
	switch (ltype) {
	case -1:
	    {
		G_warning(_("Unable to read vector map <%s>"),
			  G_fully_qualified_name(grassname, mapset));
		return (NULL);
	    }
	case -2:		/* EOF */
	    {
		eof = 1;
		continue;
	    }
	}
	if ((ltype & GV_POINTS)) {
	    np++;
	    gpt->p3[X] = Points->x[0];
	    gpt->p3[Y] = Points->y[0];

	    if (ndim > 2) {
		*has_z = 1;
		gpt->dims = 3;
		gpt->p3[Z] = Points->z[0];
	    }
	    else {
		gpt->dims = 2;
		*has_z = 0;
	    }

	    /* Store category info for thematic display */
	    if (Cats->n_cats > 0) {
		gpt->cats = Cats;
		Cats = Vect_new_cats_struct();
	    }
	    else {
		gpt->cats = NULL;
		Vect_reset_cats(Cats);
	    }
	    gpt->highlighted = 0;


	    G_debug(3, "loading vector point %d %f %f -- %d",
		    np, Points->x[0], Points->y[0], Cats->n_cats);

	    gpt->next = (geopoint *) G_malloc(sizeof(geopoint));	/* G_fatal_error */
	    if (!gpt->next) {
		return (NULL);
	    }

	    prev = gpt;
	    gpt = gpt->next;
	}

    }
    if (np > 0) {
	prev->next = NULL;
	G_free(gpt);
    }

    Vect_close(&map);

    if (!np) {
	G_warning(_("No points from vector map <%s> fall within current region"),
		  G_fully_qualified_name(grassname, mapset));
	return (NULL);
    }
    else {
	G_message(_("Vector map <%s> loaded (%d points)"),
		  G_fully_qualified_name(grassname, mapset), np);
    }

    *nsites = np;

    return (top);
}
