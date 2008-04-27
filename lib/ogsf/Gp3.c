/*
* $Id$
*/

/*  Gp.c 
    Bill Brown, USACERL  
    January 1994
    Uses GRASS routines!
*/

#include <stdlib.h>
#include <grass/gis.h>
#include <grass/site.h>
#include <grass/Vect.h>

#include <grass/gstypes.h>

/* used when site attribute mode is ST_ATT_COLOR */
/* Gets color structure for grass file, goes through points and
   uses fattr as CAT, putting rgb color in iattr. */
int Gp_set_color(char *grassname, geopoint * gp)
{
    char *col_map;
    struct Colors sc;
    CELL cat;
    geopoint *tp;
    int r, g, b, color;

    /* TODO: handle error messages */

    if (grassname) {
	col_map = G_find_file2("cell", grassname, "");

	if (col_map == NULL) {
	    fprintf(stderr, "Could not find file '%s'", grassname);
	    return (0);
	}
	else {
	    G_read_colors(grassname, col_map, &sc);
	}

	for (tp = gp; tp; tp = tp->next) {
	    cat = (int) tp->fattr;
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

/*##############################################################*/
/* This loads to memory.  
The other alternative may be to load to a tmp file. */
geopoint *Gp_load_sites(char *grassname, int *nsites, int *has_z,
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


    /* TODO: handle error messages */

    np = 0;
    eof = 0;
    *has_z = *has_att = 0;

    Vect_set_open_level (2);
    Vect_open_old (&map, grassname, "");

    Points = Vect_new_line_struct ();
    Cats = Vect_new_cats_struct ();

    if (NULL == (top = gpt = (geopoint *) malloc(sizeof(geopoint)))) {
	fprintf(stderr, "Can't malloc.\n");
	return (NULL);
    }

    G_get_set_window(&wind);

    /* get ndim */
    ndim = 2;
    if ( Vect_is_3d(&map) ) {
	ndim = 3;
    }
    
    /* set rtype */
    rtype = CELL_TYPE;

    while(eof == 0)
    {
	ltype =  Vect_read_next_line (&map, Points, Cats);
	switch (ltype)
	{
		case -1:
		{
			fprintf(stderr, "Can't read vector file");
			return (NULL);
		}
		case -2: /* EOF */
		{
			eof = 1;
			continue;
		}
	}
	if ( (ltype & GV_POINTS))
	{
		np++;
		gpt->p3[X] = Points->x[0]; 
		gpt->p3[Y] = Points->y[0];

		if (ndim > 2) {
			*has_z = 1;
			gpt->dims = 3;
			gpt->p3[Z] = Points->z[0];
		} else {
			gpt->dims = 2;
			*has_z = 0;
		}

		if (Cats->n_cats > 0) {
			*has_att = 1;
			gpt->fattr = Cats->field[0]; /* Is this correct? */
			/* gpt->cat = ; ??***/
			gpt->highlight_color = gpt->highlight_size = gpt->highlight_marker = FALSE;	
		} else {
			gpt->fattr = 0;
			*has_att = 0;
		}

		gpt->iattr = gpt->fattr;
		gpt->cattr = NULL;

		G_debug(3, "loading vector point %d %f %f -- %d", 
			np, Points->x[0], Points->y[0], Cats->n_cats);
		if (NULL == 
			(gpt->next = (geopoint *) malloc(sizeof(geopoint)))) {
                		fprintf(stderr, "Can't malloc.\n");/*CLEAN UP*/
                		return (NULL);
            	}

		prev = gpt;
		gpt = gpt->next;
	}

    }
    if (np > 0)
    {
	prev->next = NULL;
	free(gpt);
    }

    Vect_close (&map);

    if (!np) {
	    fprintf(stderr, "Error: No points from %s fall within current region\n", grassname);
	    return(NULL);
    } else {
            fprintf(stderr, "Vector file %s loaded with %d points.\n",
		grassname, np);
    }

    *nsites = np;

    return (top);
}
