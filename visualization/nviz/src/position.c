/*      Alex Shevlakov sixote@yahoo.com 02/2000
 *      function added to handle postgres queries
 */
#include <stdlib.h>
#include <grass/gis.h>
#include "interface.h"

int Ninit_view_cmd(Nv_data * data, Tcl_Interp * interp,	/* Current interpreter. */
		   int argc,	/* Number of arguments. */
		   char **argv	/* Argument strings. */
    )
{
    GS_init_view();
    return (TCL_OK);
}

/* TODO: Need Nset_to_cmd or use viewdir */

int Nget_to_cmd(Nv_data * data, Tcl_Interp * interp,	/* Current interpreter. */
		int argc,	/* Number of arguments. */
		char **argv	/* Argument strings. */
    )
{
    float to[3];
    char x[32], y[32], z[32];
    char *list[4];

    GS_get_to(to);
    sprintf(x, "%f", to[0]);
    sprintf(y, "%f", to[1]);
    sprintf(z, "%f", to[2]);

    list[0] = x;
    list[1] = y;
    list[2] = z;
    list[3] = NULL;


    Tcl_SetResult(interp, Tcl_Merge(3, list), TCL_DYNAMIC);
    return (TCL_OK);
}

int Nget_from_cmd(Nv_data * data, Tcl_Interp * interp,	/* Current interpreter. */
		  int argc,	/* Number of arguments. */
		  char **argv	/* Argument strings. */
    )
{
    float from[3];
    char x[32], y[32], z[32];
    char *list[4];

    GS_get_from(from);
    sprintf(x, "%f", from[0]);
    sprintf(y, "%f", from[1]);
    sprintf(z, "%f", from[2]);

    list[0] = x;
    list[1] = y;
    list[2] = z;
    list[3] = NULL;


    Tcl_SetResult(interp, Tcl_Merge(3, list), TCL_DYNAMIC);


    return (TCL_OK);
}

int Nlook_here_cmd(Nv_data * data, Tcl_Interp * interp,	/* Current interpreter. */
		   int argc,	/* Number of arguments. */
		   char **argv	/* Argument strings. */
    )
{
    if (argc != 3)
	return (TCL_ERROR);
    GS_look_here(atoi(argv[1]), atoi(argv[2]));

    return (TCL_OK);
}

int Nhas_focus_cmd(Nv_data * data, Tcl_Interp * interp,	/* Current interpreter. */
		   int argc,	/* Number of arguments. */
		   char **argv	/* Argument strings. */
    )
{
    float realto[3];

    if (GS_get_focus(realto))
	Tcl_SetResult(interp, "1", TCL_VOLATILE);
    else
	Tcl_SetResult(interp, "0", TCL_VOLATILE);

    return (TCL_OK);

}

/***********************************/
int Nset_focus_gui_cmd(Nv_data * data, Tcl_Interp * interp,	/* Current interpreter. */
		       int argc,	/* Number of arguments. */
		       char **argv	/* Argument strings. */
    )
{
    float realto[3];
    float n, s, e, w;
    float ew_res, ns_res;
    int id;
    int *surf_list, num_surfs;
    int rows, cols;

    /* Get current center */
    GS_get_focus(realto);
    surf_list = GS_get_surf_list(&num_surfs);

    if (argc == 3 && surf_list != NULL) {
	id = surf_list[0];
	G_free(surf_list);
	GS_get_dims(id, &rows, &cols);
	/* get coordinates from gui (0->1) and convert to screen */
	GS_get_region(&n, &s, &w, &e);
	ew_res = (e - w) / cols;
	ns_res = (n - s) / rows;
	/* EAST TO WEST -- east=1, west=0 */
	realto[0] = (float)atof(argv[1]);
	realto[0] = (float)((e - w) * realto[0]) + w;
	realto[0] = (float)(realto[0] - w - (ew_res / 2.));
	/* NORTH to SOUTH -- north=0 south=1 */
	realto[1] = (float)atof(argv[2]);
	realto[1] = n - ((n - s) * realto[1]);
	realto[1] = (float)(realto[1] - s - (ns_res / 2.));

	GS_set_focus(realto);
	Nquick_draw_cmd(data, interp);
    }

    return (TCL_OK);

}


/***********************************/
int Nget_focus_gui_cmd(Nv_data * data, Tcl_Interp * interp,	/* Current interpreter. */
		       int argc,	/* Number of arguments. */
		       char **argv	/* Argument strings. */
    )
{
    float realto[3];
    float n, s, e, w;
    float ew_res, ns_res;
    int id;
    int *surf_list, num_surfs;
    int rows, cols;
    char *list[2], east[32], north[32];


    /* Get current center */
    GS_get_focus(realto);

    surf_list = GS_get_surf_list(&num_surfs);
    if (surf_list != NULL) {
	id = surf_list[0];
	G_free(surf_list);
	GS_get_dims(id, &rows, &cols);
	/* get coordinates from gui (0->1) and convert to screen */
	GS_get_region(&n, &s, &w, &e);
	ew_res = (e - w) / cols;
	ns_res = (n - s) / rows;
	/* EAST TO WEST -- east=1, west=0 */
	realto[0] = realto[0] + (ew_res / 2.);
	sprintf(east, "%f", (realto[0] / (e - w)));
	list[0] = east;

	/* NORTH to SOUTH -- north=0 south=1 */
	realto[1] = realto[1] + (ns_res / 2.);
	sprintf(north, "%f", (realto[1] / (n - s)));
	list[1] = north;

	Tcl_SetResult(interp, Tcl_Merge(2, list), TCL_VOLATILE);
    }

    return (TCL_OK);

}

/************************************************************/
int Nget_real_position_cmd(Nv_data * data, Tcl_Interp * interp,	/* Current interpreter. */
			   int argc,	/* Number of arguments. */
			   char **argv	/* Argument strings. */
    )
{
    float realto[3];
    int pos_flag;
    char *list[3], east[32], north[32], elev[32];

    if (argc != 2)
	return (TCL_ERROR);
    pos_flag = atoi(argv[1]);

    if (pos_flag == 1) {
	/* Get from position in real coords */
	GS_get_from_real(realto);

	sprintf(east, "%f", realto[0]);
	list[0] = east;
	sprintf(north, "%f", realto[1]);
	list[1] = north;
	sprintf(elev, "%f", realto[2]);
	list[2] = elev;

	Tcl_SetResult(interp, Tcl_Merge(3, list), TCL_VOLATILE);

    }
    else {
	/* Get to position in real coords */
	GS_get_to_real(realto);
	sprintf(east, "%f", realto[0]);
	list[0] = east;
	sprintf(north, "%f", realto[1]);
	list[1] = north;
	sprintf(elev, "%f", realto[2]);
	list[2] = elev;

	Tcl_SetResult(interp, Tcl_Merge(3, list), TCL_VOLATILE);
    }

    return (TCL_OK);

}

/***********************************/
int Nset_focus_cmd(Nv_data * data, Tcl_Interp * interp,	/* Current interpreter. */
		   int argc,	/* Number of arguments. */
		   char **argv	/* Argument strings. */
    )
{
    float realto[3];

    if (argc == 4) {
	realto[0] = (float)atof(argv[1]);
	realto[1] = (float)atof(argv[2]);
	realto[2] = (float)atof(argv[3]);
	GS_set_focus(realto);
    }

    return (TCL_OK);

}

/***********************************/
int Nset_focus_real_cmd(Nv_data * data, Tcl_Interp * interp,	/* Current interpreter. */
			int argc,	/* Number of arguments. */
			char **argv	/* Argument strings. */
    )
{
    float realto[3];

    if (argc == 4) {
	realto[0] = (float)atof(argv[1]);
	realto[1] = (float)atof(argv[2]);
	realto[2] = (float)atof(argv[3]);
	GS_set_focus_real(realto);
    }

    return (TCL_OK);

}

int Nset_focus_state_cmd(Nv_data * data, Tcl_Interp * interp,	/* Current interpreter. */
			 int argc,	/* Number of arguments. */
			 char **argv	/* Argument strings. */
    )
{
    int state_flag = 1;

    if (argc != 2)
	return (TCL_ERROR);

    state_flag = atoi(argv[1]);

    if (state_flag == 1)
	GS_set_infocus();	/* return center of view */
    else if (state_flag == 0)
	GS_set_nofocus();	/* no center of view -- use viewdir */
    else {
	Tcl_SetResult(interp, "Error: Flag must be either 0|1", TCL_VOLATILE);
	return (TCL_ERROR);
    }

    return (TCL_OK);
}

int Nset_focus_top_cmd(Nv_data * data, Tcl_Interp * interp,	/* Current interpreter. */
		       int argc,	/* Number of arguments. */
		       char **argv	/* Argument strings. */
    )
{
    float realto[3];
    float realto_new[3];
    float n, s, w, e, elev;
    float xres, yres, fudge;
    int id;
    int rows, cols;
    int *surf_list, num_surfs;

    if (argc != 2)
	return (TCL_ERROR);

    elev = atof(argv[1]);

    GS_get_focus(realto);
    GS_get_region(&n, &s, &w, &e);

    surf_list = GS_get_surf_list(&num_surfs);
    if (surf_list != NULL) {
	id = surf_list[0];
	G_free(surf_list);
	GS_get_dims(id, &rows, &cols);
    }

    xres = (e - w) / cols;
    yres = (n - s) / rows;
    /* calc fudge value to ensure north is up */
    fudge = rows * 0.1;

    realto_new[0] = (realto[0] + w + (xres / 2.));
    realto_new[1] = (realto[1] + s + (yres / 2.) - fudge);
    realto_new[2] = (elev);

    GS_moveto_real(realto_new);
    GS_alldraw_wire();



    return (TCL_OK);
}


int Nset_focus_map_cmd(Nv_data * data, Tcl_Interp * interp,	/* Current interpreter. */
		       int argc,	/* Number of arguments. */
		       char **argv	/* Argument strings. */
    )
{
    int id;

    if (!GS_num_surfs() && !(GVL_num_vols())) {
	GS_set_nofocus();
	return (TCL_OK);
    }

    if (argc == 1) {
	int *surf_list, num_surfs, *vol_list;

	if (GS_num_surfs() > 0) {
	    surf_list = GS_get_surf_list(&num_surfs);
	    id = surf_list[0];
	    G_free(surf_list);

	    GS_set_focus_center_map(id);
	    return (TCL_OK);
	}

	if (GVL_num_vols() > 0) {
	    vol_list = GVL_get_vol_list(&num_surfs);
	    id = vol_list[0];
	    G_free(vol_list);

	    GVL_set_focus_center_map(id);
	    return (TCL_OK);
	}
    }

    if (!strcmp(argv[1], "surf")) {
	id = atoi(argv[2]);

	GS_set_focus_center_map(id);
	return (TCL_OK);
    }

    if (!strcmp(argv[1], "vol")) {
	id = atoi(argv[2]);

	GVL_set_focus_center_map(id);
	return (TCL_OK);
    }

    return (TCL_ERROR);
}


int Nmove_to_real_cmd(Nv_data * data, Tcl_Interp * interp,	/* Current interpreter. */
		      int argc,	/* Number of arguments. */
		      char **argv	/* Argument strings. */
    )
{
    float ftmp[3];
    double atof();

    if (argc != 4)
	return (TCL_ERROR);

    ftmp[0] = (float)atof(argv[1]);
    ftmp[1] = (float)atof(argv[2]);
    ftmp[2] = (float)atof(argv[3]);
    GS_moveto_real(ftmp);

    return (TCL_OK);
}


int Nmove_to_cmd(Nv_data * data, Tcl_Interp * interp,	/* Current interpreter. */
		 int argc,	/* Number of arguments. */
		 char **argv	/* Argument strings. */
    )
{
    float ftmp[3];
    double atof();

    if (argc != 4)
	return (TCL_ERROR);

    ftmp[0] = (float)atof(argv[1]);
    ftmp[1] = (float)atof(argv[2]);
    ftmp[2] = (float)atof(argv[3]);
    GS_moveto(ftmp);

    return (TCL_OK);
}


int Nset_fov_cmd(Nv_data * data, Tcl_Interp * interp,	/* Current interpreter. */
		 int argc,	/* Number of arguments. */
		 char **argv	/* Argument strings. */
    )
{
    int fov;

    if (argc != 2)
	return (TCL_ERROR);
    fov = atoi(argv[1]);
    fov = (int)(fov * 10);
    GS_set_fov(fov);

    return (TCL_OK);
}

int Nget_fov_cmd(Nv_data * data, Tcl_Interp * interp,	/* Current interpreter. */
		 int argc,	/* Number of arguments. */
		 char **argv	/* Argument strings. */
    )
{
    char *list[2];
    char fov_name[32];
    int fov;

    fov = GS_get_fov();
    fov = (int)(fov / 10);

    sprintf(fov_name, "%d", fov);
    list[0] = fov_name;
    list[1] = NULL;
    Tcl_SetResult(interp, Tcl_Merge(1, list), TCL_DYNAMIC);


    return (TCL_OK);
}

int Nset_twist_cmd(Nv_data * data, Tcl_Interp * interp,	/* Current interpreter. */
		   int argc,	/* Number of arguments. */
		   char **argv	/* Argument strings. */
    )
{
    int twist;

    if (argc != 2)
	return (TCL_ERROR);

    twist = atoi(argv[1]);
    twist = (int)(twist * 10);
    GS_set_twist(twist);

    return (TCL_OK);
}

int Nget_twist_cmd(Nv_data * data, Tcl_Interp * interp,	/* Current interpreter. */
		   int argc,	/* Number of arguments. */
		   char **argv	/* Argument strings. */
    )
{
    char *list[2];
    char twist_name[32];
    int twist;

    twist = GS_get_twist();
    twist = (int)(twist / 10);

    sprintf(twist_name, "%d", twist);
    list[0] = twist_name;
    list[1] = NULL;
    Tcl_SetResult(interp, Tcl_Merge(1, list), TCL_DYNAMIC);

    return (TCL_OK);
}

int Nget_region_cmd(Nv_data * data, Tcl_Interp * interp,	/* Current interpreter. */
		    int argc,	/* Number of arguments. */
		    char **argv	/* Argument strings. */
    )
{
    float n, s, e, w;
    char *list[5];
    char north[32], east[32], south[32], west[32];

    GS_get_region(&n, &s, &e, &w);
    sprintf(north, "%f", n);
    sprintf(east, "%f", e);
    sprintf(south, "%f", s);
    sprintf(west, "%f", w);

    list[0] = north;
    list[1] = east;
    list[2] = south;
    list[3] = west;
    list[4] = NULL;

    Tcl_SetResult(interp, Tcl_Merge(4, list), TCL_DYNAMIC);

    return (TCL_OK);
}

int Nget_point_on_surf_cmd(Nv_data * data, Tcl_Interp * interp,	/* Current interpreter. */
			   int argc,	/* Number of arguments. */
			   char **argv	/* Argument strings. */
    )
{
    float x, y, z;
    int sx, sy, id;
    char cx[32], cy[32], cz[32], idname[128];
    char *list[5];

    if (argc != 3)
	return (TCL_ERROR);

    sx = atoi(argv[1]);
    sy = atoi(argv[2]);

    G_debug(3, "x= %d  :  y= %d\n", sx, sy);

    if (!GS_get_selected_point_on_surface(sx, sy, &id, &x, &y, &z)) {
	list[0] = NULL;
	Tcl_SetResult(interp, Tcl_Merge(0, list), TCL_DYNAMIC);

	return (TCL_OK);
    }

    sprintf(cx, "%f", x);
    sprintf(cy, "%f", y);
    sprintf(cz, "%f", z);
    sprintf(idname, "Nsurf%d", id);

    list[0] = cx;
    list[1] = cy;
    list[2] = cz;
    list[3] = idname;
    list[4] = NULL;

    Tcl_SetResult(interp, Tcl_Merge(4, list), TCL_DYNAMIC);

    return (TCL_OK);

}

int Nget_point_on_surf_vect(Nv_data * data, Tcl_Interp * interp, int argc,
			    char **argv)


	/* Current interpreter. */
			/* Number of arguments. */
		/* Argument strings. */
{
    float x, y, z;
    int sx, sy, id;
    char cx[32], cy[32], cz[32], idname[128];
    char *list[6];
    char *name;

    if (argc != 4)
	return (TCL_ERROR);

    sx = atoi(argv[1]);
    sy = atoi(argv[2]);
    name = argv[3];

    G_debug(3, "x= %d  :  y= %d\n", sx, sy);

    if (!GS_get_selected_point_on_surface(sx, sy, &id, &x, &y, &z)) {
	list[0] = NULL;
	Tcl_SetResult(interp, Tcl_Merge(0, list), TCL_DYNAMIC);

	return (TCL_OK);
    }

    sprintf(cx, "%f", x);
    sprintf(cy, "%f", y);
    sprintf(cz, "%f", z);
    sprintf(idname, "Nsurf%d", id);

    list[0] = cx;
    list[1] = cy;
    list[2] = cz;
    list[3] = idname;
    list[4] = (char *)query_vect(name, x, y);
    list[5] = NULL;

    Tcl_SetResult(interp, Tcl_Merge(5, list), TCL_DYNAMIC);

    return (TCL_OK);

}

int Nget_dist_along_surf_cmd(Nv_data * data, Tcl_Interp * interp,	/* Current interpreter. */
			     int argc,	/* Number of arguments. */
			     char **argv	/* Argument strings. */
    )
{
    float x, y, px, py, d;
    int id, exag;
    char dist[128];
    double atof();

    if (argc != 7)
	return (TCL_ERROR);

    id = get_idnum(argv[1]);
    x = (float)atof(argv[2]);
    y = (float)atof(argv[3]);
    px = (float)atof(argv[4]);
    py = (float)atof(argv[5]);
    exag = (float)atoi(argv[6]);

    if (!GS_get_distance_alongsurf(id, x, y, px, py, &d, exag))
	return (TCL_ERROR);
    sprintf(dist, "%f", d);
    Tcl_SetResult(interp, dist, TCL_VOLATILE);

    return (TCL_OK);

}

/*
   #define DO_TEST
 */

int Nget_cat_at_xy_cmd(Nv_data * data, Tcl_Interp * interp,	/* Current interpreter. */
		       int argc,	/* Number of arguments. */
		       char **argv	/* Argument strings. */
    )
{
    float x, y;
    int id, att;
    char catstr[1024];
    double atof();

    if (argc != 5)
	return (TCL_ERROR);

    id = get_idnum(argv[1]);
    att = att_atoi(argv[2]);
    x = (float)atof(argv[3]);
    y = (float)atof(argv[4]);

#ifdef DO_TEST
    GS_set_draw(GSD_FRONT);
    GS_ready_draw();
    GS_draw_flowline_at_xy(id, x, y);
    GS_done_draw();
#endif

    if (0 > GS_get_cat_at_xy(id, att, catstr, x, y)) {
	Tcl_SetResult(interp, "no category", TCL_VOLATILE);
	return (TCL_OK);
    }
    Tcl_SetResult(interp, catstr, TCL_VOLATILE);

    return (TCL_OK);

}


int Nget_val_at_xy_cmd(Nv_data * data, Tcl_Interp * interp,	/* Current interpreter. */
		       int argc,	/* Number of arguments. */
		       char **argv	/* Argument strings. */
    )
{
    float x, y;
    int id, att;
    char valstr[1024];
    double atof();

    if (argc != 5)
	return (TCL_ERROR);

    id = get_idnum(argv[1]);
    att = att_atoi(argv[2]);
    x = (float)atof(argv[3]);
    y = (float)atof(argv[4]);

    GS_get_val_at_xy(id, att, valstr, x, y);

    Tcl_SetResult(interp, valstr, TCL_VOLATILE);

    return (TCL_OK);

}

int Nget_focus_cmd(Nv_data * data, Tcl_Interp * interp,	/* Current interpreter. */
		   int argc,	/* Number of arguments. */
		   char **argv	/* Argument strings. */
    )
{
    float realto[3];
    char x[32], y[32], z[32];
    char *list[4];

    if (GS_get_focus(realto)) {
	sprintf(x, "%f", realto[0]);
	sprintf(y, "%f", realto[1]);
	sprintf(z, "%f", realto[2]);

	list[0] = x;
	list[1] = y;
	list[2] = z;
	list[3] = NULL;

	Tcl_SetResult(interp, Tcl_Merge(3, list), TCL_VOLATILE);
    }
    else {
	Tcl_SetResult(interp, "0", TCL_VOLATILE);
    }

    return (TCL_OK);
}


int Nget_longdim_cmd(Nv_data * data, Tcl_Interp * interp,	/* Current interpreter. */
		     int argc,	/* Number of arguments. */
		     char **argv	/* Argument strings. */
    )
{
    float dim;
    char buf[128];

    GS_get_longdim(&dim);
    sprintf(buf, "%f", dim);
    Tcl_SetResult(interp, buf, TCL_VOLATILE);

    return (TCL_OK);
}

int Nget_zrange_cmd(Nv_data * data, Tcl_Interp * interp,	/* Current interpreter. */
		    int argc,	/* Number of arguments. */
		    char **argv	/* Argument strings. */
    )
{
    float min, max;
    char *list[3], cmin[32], cmax[32];

    min = max = 0.0;
    if (argc > 2) {
	if (!strcmp(argv[2], "doexag"))
	    GS_get_zrange_nz(&min, &max);
	else if (!strcmp(argv[2], "nz"))
	    GS_get_zrange_nz(&min, &max);
	else
	    return (TCL_ERROR);
    }
    else
	GS_get_zrange_nz(&min, &max);

    sprintf(cmin, "%f", min);
    sprintf(cmax, "%f", max);
    list[0] = cmin;
    list[1] = cmax;
    list[2] = NULL;


    Tcl_SetResult(interp, Tcl_Merge(2, list), TCL_DYNAMIC);
    return (TCL_OK);
}

int
Nget_xyrange_cmd(Nv_data * data, Tcl_Interp * interp, int argc, char **argv)
{
    char temp[40];

    if (argc != 1) {
	Tcl_SetResult(interp, "Usage: Nget_xyrange", TCL_VOLATILE);
	return (TCL_ERROR);
    }

    sprintf(temp, "%f", data->XYrange);
    Tcl_SetResult(interp, temp, TCL_VOLATILE);
    return (TCL_OK);
}

int Nget_zextents_cmd(Nv_data * data, Tcl_Interp * interp,	/* Current interpreter. */
		      int argc,	/* Number of arguments. */
		      char **argv	/* Argument strings. */
    )
{
    int id;
    float min, max, mid;
    char *list[4], cmin[32], cmax[32], cmid[32];

    if (argc != 2)
	return (TCL_ERROR);
    id = get_idnum(argv[1]);

    GS_get_zextents(id, &min, &max, &mid);

    sprintf(cmin, "%f", min);
    sprintf(cmax, "%f", max);
    sprintf(cmid, "%f", mid);
    list[0] = cmin;
    list[1] = cmax;
    list[2] = cmid;
    list[3] = NULL;

    Tcl_SetResult(interp, Tcl_Merge(3, list), TCL_DYNAMIC);

    return (TCL_OK);
}

int Nget_exag_cmd(Nv_data * data, Tcl_Interp * interp,	/* Current interpreter. */
		  int argc,	/* Number of arguments. */
		  char **argv	/* Argument strings. */
    )
{
    char buf[128];
    float exag;

    exag = GS_global_exag();

    sprintf(buf, "%f", exag);
    Tcl_SetResult(interp, buf, TCL_VOLATILE);

    return (TCL_OK);
}

int Nset_exag_cmd(Nv_data * data, Tcl_Interp * interp,	/* Current interpreter. */
		  int argc,	/* Number of arguments. */
		  char **argv	/* Argument strings. */
    )
{
    double atof();

    if (argc != 2)
	return (TCL_ERROR);
    GS_set_global_exag((float)atof(argv[1]));
    return (TCL_OK);
}

/*
 * Nsave_3dview_cmd --
 *
 *      Syntax: Nsave_3dview file_name
 *      Saves the current orientation of Nviz camera position.
 *      Note that GRASS requires a surface ID to reference
 *      the view to.  By default we choose the first surface
 *      available or 0 if no surfaces have been loaded.
 */
int Nsave_3dview_cmd(Nv_data * data, Tcl_Interp * interp,	/* Current interpreter. */
		     int argc,	/* Number of arguments. */
		     char **argv	/* Argument strings. */
    )
{
    const char **list_space;
    int list_count, first_surf;

    /* Check for correct number of arguments */
    if (argc != 2) {
	Tcl_SetResult(interp, "Usage: Nsave_3dview file_name", TCL_VOLATILE);
	return (TCL_ERROR);
    }

    /* Try and find a surface to use as a reference */
    if ((Tcl_Eval(interp, "Nget_surf_list") != TCL_OK) ||
	(Tcl_SplitList(interp, Tcl_GetStringResult(interp), &list_count, &list_space) !=
	 TCL_OK)) {
	Tcl_SetResult(interp,
		      "Internal Error: Parse failure in Nsave_3dview_cmd",
		      TCL_VOLATILE);
	return (TCL_ERROR);
    }
    else {
	if (!list_count)
	    first_surf = 0;
	else
	    first_surf = (int)atoi(list_space[0]);

	Tcl_Free((char *)list_space);
    }

    /* Finally make the GSF library call */
    GS_save_3dview(argv[1], first_surf);

    return (TCL_OK);
}

/*
 * Nload_3dview_cmd --
 *
 *      Syntax: Nload_3dview file_name
 *      Loads a saved view.
 *      Note that GRASS requires a surface ID to reference
 *      the view to.  By default we choose the first surface
 *      available or 0 if no surfaces have been loaded.
 */
int Nload_3dview_cmd(Nv_data * data, Tcl_Interp * interp,	/* Current interpreter. */
		     int argc,	/* Number of arguments. */
		     char **argv	/* Argument strings. */
    )
{
    const char **list_space;
    int list_count, first_surf;

    /* Check for correct number of arguments */
    if (argc != 2) {
	Tcl_SetResult(interp, "Usage: Nload_3dview file_name", TCL_VOLATILE);
	return (TCL_ERROR);
    }

    /* Try and find a surface to use as a reference */
    if ((Tcl_Eval(interp, "Nget_surf_list") != TCL_OK) ||
	(Tcl_SplitList(interp, Tcl_GetStringResult(interp), &list_count, &list_space) !=
	 TCL_OK)) {
	Tcl_SetResult(interp,
		      "Internal Error: Parse failure in Nsave_3dview_cmd",
		      TCL_VOLATILE);
	return (TCL_ERROR);
    }
    else {
	if (!list_count)
	    first_surf = 0;
	else
	    first_surf = (int)atoi(list_space[0]);

	Tcl_Free((char *)list_space);
    }
    /* Finally make the GSF library call */
    GS_load_3dview(argv[1], first_surf);

    return (TCL_OK);
}
