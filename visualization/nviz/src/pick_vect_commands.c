
/***************************************************************
 *
 * MODULE:       pick_vect_commands.c 1.0
 *
 * AUTHOR(S):    ACS - Massimo Cuomo - m.cuomo at acsys.it
 *
 * PURPOSE:		 Picking of site elements for showing associated
 *                  DB content. Site map must be enabled with
 *                  pick_add_map (and removed with (pick_remove_map)
 *
 *               These functions are derived from files query_vect.c
 *                  position.c and the original Tcl command
 *                  Nget_point_on_surf_vect.
 *                  Have been added in order to set dinamically
 *                  "maxdist" for picking and to optimize
 *                  picking procedure, not going through formatting
 *                  strings back and forth
 *
 *
 * REQUIREMENTS: site_attr_commands.c and optionally site_highlight_commands.c
 *
 * COPYRIGHT:    (C) 2005 by the ACS / GRASS Development Team
 *
 *               This program is free software under the
 *               GNU General Public License (>=v2).
 *               Read the file COPYING that comes with GRASS
 *               for details.
 *
 **************************************************************/

#if 0

/*******************************************************************************
*** pick_vect "README" *********************************************************
********************************************************************************

 The Tcl command "Npick_vect" returns a list (usually only
    one element) of found categories for the picked vector.

 Here we are neglecting the field the category is associated to.

 Function query_vect_cats retrieves also these fields, but if we look at
    the other functions we use to manage sites db entries,
    we notice that (starting from G_sites_open_old) only field 1
    is considered.

 My understanding at the moment is that there could be more categories
    "Cats->cat[i]" associated to a single vect/site element and each category
    is contained in a field "Cats->field[i]".

 These parameters are used to find the field_info from a related db
    via the call: "field_info *fi = Vect_get_field(Map, Cats->field[i]);"
    In this "fi" we can access our record via the specific cat value Cats->cat[i]

 But as G_sites_open_old uses only Cats->field[i]=1 when calls
    field_info *fi = Vect_get_field(Map, 1);
    in order to retrieve the db info, here we should never find
    fields different from 1 and hence more than a single category.

 So in the panel_pick.tcl functions we will use only the first element
    of the returned list, because, until things don't change,
    there will be always only one cat and it should correspond to field 1.



*** REQUIREMENTS

     site_attr_commands.c and optionally site_highlight_commands.c
       are used by panel_pick.tcl.
     site_attr_commands.c is used to retrieve the fields names, values
	   and cats of records with a not empty specific field
     site_highlight_commands.c is optionally used to highlight picked
	   objects. It is optionally used because "catch" is used in the Tcl
	   code, so if it is not defined, the program doesn't fail.



*** FILE INVOLVED

+++ visualization/nviz/scripts

(new) visualization/nviz/scripts/panel_pick.tcl
	all tcl code

			to panelIndex file add:
				"pick"
			to tclIndex file add:
				set auto_index(mkpickPanel) "source $dir/panel_pick.tcl"


+++ visualization/nviz/src

(new) visualization/nviz/src/pick_vect_commands.c
	C functions for picking

(mod) visualization/nviz/src/nviz_init.c
	creates tcl commands and variables by calling
		pick_init_tcl(interp, &data);
		in Ninit function
		added commands:	Npick_vect,


WARNING: 	remember to add pick_vect_commands.o
			into the visualization/nviz/src/Makefile


********************************************************************************
********************************************************************************
*******************************************************************************/
#endif

#include <stdio.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/form.h>
#include <grass/dbmi.h>
#include <grass/display.h>
#include <grass/Vect.h>

#include <stdlib.h>
#include "interface.h"

int Npick_vect_cmd(Nv_data * data, Tcl_Interp * interp, int argc,
		   char **argv);
int query_vect_cats(char *name, double x, double y, double maxdist,
		    int **found_cats);

void pick_init_tcl(Tcl_Interp * interp, Nv_data * data)
{
    Tcl_CreateCommand(interp, "Npick_vect", (Tcl_CmdProc *) Npick_vect_cmd,
		      data, NULL);
}


/* Tcl command Npick_vect */

int Npick_vect_cmd(data, interp, argc, argv)
     Nv_data *data;
     Tcl_Interp *interp;	/* Current interpreter. */
     int argc;			/* Number of arguments. */
     char **argv;		/* Argument strings. */
{
    float x, y, z, maxdist;
    int sx, sy, id;
    char *name;

    int *found_cats = NULL;
    int i, n_cats;
    char buf[2000];

    if (argc != 5)
	return (TCL_ERROR);

    sx = atoi(argv[1]);
    sy = atoi(argv[2]);
    name = argv[3];
    maxdist = atof(argv[4]);

#ifdef DEBUG_MSG
    fprintf(stderr, "x= %d  :  y= %d\n", sx, sy);
#endif

    if (!GS_get_selected_point_on_surface(sx, sy, &id, &x, &y, &z)) {
	sprintf(buf, "%s", "");
	Tcl_AppendElement(interp, buf);
	return (TCL_OK);
    }

    n_cats = query_vect_cats(name, x, y, maxdist, &found_cats);

    for (i = 0; i < n_cats; i++) {
	sprintf(buf, "%d", found_cats[i]);
	Tcl_AppendElement(interp, buf);
    }

    if (found_cats)
	free(found_cats);
    return (TCL_OK);
}


int query_vect_cats(char *name, double x, double y, double maxdist,
		    int **found_cats)
{
    struct Map_info Map;
    int line, area;
    static struct line_cats *Cats = NULL;
    char *mapset;

    if (!Cats)
	Cats = Vect_new_cats_struct();
    else
	Vect_reset_cats(Cats);

    if ((mapset = G_find_vector2(name, "")) == NULL)
	return (-1);

    Vect_open_old(&Map, name, mapset);

    line =
	Vect_find_line(&Map, x, y, 0.0,
		       GV_POINT | GV_LINE | GV_BOUNDARY | GV_CENTROID,
		       maxdist, 0, 0);
    area = Vect_find_area(&Map, x, y);

    if (line + area == 0)
	goto error;

    if (line > 0)
	Vect_read_line(&Map, NULL, Cats, line);
    else if (area > 0)
	Vect_get_area_cats(&Map, area, Cats);

    if (Cats->n_cats > 0) {
	int i;

	*found_cats = (int *)G_malloc(Cats->n_cats * sizeof(int));

	for (i = 0; i < Cats->n_cats; i++) {
	    (*found_cats)[i] = Cats->cat[i];
	    G_debug(3, "##################### field: %d category: %d\n",
		    Cats->field[i], Cats->cat[i]);
	}
    }
    else {
	goto error;
    }

    Vect_close(&Map);
    return (Cats->n_cats);

  error:
    Vect_close(&Map);
    return (-1);
}
