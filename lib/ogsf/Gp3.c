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
#include <grass/site.h>
#include <grass/Vect.h>
#include <grass/glocale.h>
#include <grass/gstypes.h>

/*!
   \brief Set color for point set

   used when site attribute mode is ST_ATT_COLOR

   Gets color structure for grass file, goes through points and
   uses fattr as CAT, putting rgb color in iattr.

   \param grassname raster map name
   \param gp pointer to geopoint struct

   \return 1 on success
   \return 0 on failure
 */
int Gp_set_color(const char *grassname, geopoint * gp)
{
    const char *col_map;
    struct Colors sc;
    CELL cat;
    geopoint *tp;
    int r, g, b, color;

    /* TODO: handle error messages */

    if (grassname) {
	col_map = G_find_cell2(grassname, "");
	if (!col_map) {
	    G_warning(_("Raster map <%s> not found"), grassname);
	    return 0;
	}

	G_read_colors(grassname, col_map, &sc);

	for (tp = gp; tp; tp = tp->next) {
	    cat = (int)tp->fattr;
	    color = NULL_COLOR;

	    if (G_get_color(cat, &r, &g, &b, &sc)) {
		color = (r & 0xff) | ((g & 0xff) << 8) | ((b & 0xff) << 16);
	    }

	    tp->iattr = color;
	}

	return (1);
    }

    return (0);
}

/*!
   \brief Load to points to memory

   The other alternative may be to load to a tmp file.

   \param grassname vector point map 
   \param nsites
   \param has_z 2D or 3D points?
   \param has_att attributes included

   \return pointer to geopoint struct
   \return NULL on failure
 */
geopoint *Gp_load_sites(const char *grassname, int *nsites, int *has_z,
			int *has_att)
{
    struct Map_info map;
    static struct line_pnts *Points = NULL;
    static struct line_cats *Cats = NULL;
    geopoint *top, *gpt, *prev;
    int np, ltype, eof;
    struct Cell_head wind;
    RASTER_MAP_TYPE rtype;
    int ndim;
    const char *mapset;

    np = 0;
    eof = 0;
    *has_z = *has_att = 0;

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

	    if (Cats->n_cats > 0) {
		*has_att = 1;
		gpt->fattr = Cats->field[0];	/* Is this correct? */
		/* gpt->cat = ; ??** */
		gpt->highlight_color = gpt->highlight_size =
		    gpt->highlight_marker = FALSE;
	    }
	    else {
		gpt->fattr = 0;
		*has_att = 0;
	    }

	    gpt->iattr = gpt->fattr;
	    gpt->cattr = NULL;

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
