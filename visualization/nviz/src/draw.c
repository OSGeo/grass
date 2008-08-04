#include <stdlib.h>
#include <grass/gis.h>
#include "togl.h"

#define FontBase_MAIN
#include "interface.h"

#define BG_COLOR 0xFF000000

/*
 *  Have to look into why we can't draw to BOTH in OpenGL/tcl
 *  (or if we still need to)
 *  Originally, needed to draw to both due to
 *  implementation of transparency
 */
#ifdef GSD_BOTH
#undef GSD_BOTH
#define GSD_BOTH GSD_FRONT
#endif

/* globals */
char *cancel_script = NULL;
Tcl_Interp *cancel_interp;
static GLuint legend_list = 0;

int auto_draw(Nv_data *, Tcl_Interp *);

/* this function is used as a hook to
 * call a particular script when the
 * cancel button is pressed during a draw
 */
void CancelFunc_Hook(void)
{
    if (cancel_script != NULL) {
	TkCopyAndGlobalEval(cancel_interp, cancel_script);
    }
}

/* Occasionally we want the cancel script ignored,
   this handles such a case.
 */
int Nunset_cancel_func_cmd(Nv_data * data, Tcl_Interp * interp,	/* Current interpreter. */
			   int argc,	/* Number of arguments. */
			   char **argv	/* Argument strings. */
    )
{
    if (cancel_script != NULL)
	G_free(cancel_script);

    cancel_script = NULL;

    return TCL_OK;
}

/* Set the cancel script to invoke when the cancel function
 * is called.
 */
int Nset_cancel_func_cmd(Nv_data * data, Tcl_Interp * interp,	/* Current interpreter. */
			 int argc,	/* Number of arguments. */
			 char **argv	/* Argument strings. */
    )
{
    if (argc != 2) {
	Tcl_SetResult(interp, "Usage: Nset_cancel_func script", TCL_VOLATILE);
	return (TCL_ERROR);
    }

    if (cancel_script != NULL)
	G_free(cancel_script);

    cancel_interp = interp;
    cancel_script = (char *)G_malloc(sizeof(char) * (strlen(argv[1]) + 1));
    strcpy(cancel_script, argv[1]);

    GS_set_cxl_func(CancelFunc_Hook);

    return (TCL_OK);
}

int Nset_draw_cmd(Nv_data * data, Tcl_Interp * interp,	/* Current interpreter. */
		  int argc,	/* Number of arguments. */
		  char **argv	/* Argument strings. */
    )
{
    int where;

    if (argc != 2) {
	return (TCL_ERROR);
    }

    if (!strcmp(argv[1], "front"))
	where = GSD_FRONT;
    else if (!strcmp(argv[1], "back"))
	where = GSD_BACK;
    else
	where = GSD_BOTH;

    GS_set_draw(where);
    return (TCL_OK);
}

int Ntransp_is_set_cmd(Nv_data * data, Tcl_Interp * interp,	/* Current interpreter. */
		       int argc,	/* Number of arguments. */
		       char **argv	/* Argument strings. */
    )
{
    char on[128];

    GS_transp_is_set()? sprintf(on, "1") : sprintf(on, "0");
    Tcl_SetResult(interp, on, TCL_VOLATILE);
    return (TCL_OK);
}

int Nis_masked_cmd(Nv_data * data, Tcl_Interp * interp,	/* Current interpreter. */
		   int argc,	/* Number of arguments. */
		   char **argv	/* Argument strings. */
    )
{
    char masked[128];
    int id, type;
    float pt[2];

    if (argc < 4)
	return (TCL_ERROR);
    id = get_idnum(argv[1]);
    type = get_type(argv[1]);
    pt[0] = (float)atof(argv[2]);
    pt[1] = (float)atof(argv[3]);

    GS_is_masked(id, pt) ? sprintf(masked, "1") : sprintf(masked, "0");
    Tcl_SetResult(interp, masked, TCL_VOLATILE);
    return (TCL_OK);
}

int Nhas_transparency_cmd(Nv_data * data, Tcl_Interp * interp,	/* Current interpreter. */
			  int argc,	/* Number of arguments. */
			  char **argv	/* Argument strings. */
    )
{

    if (GS_has_transparency())
	Tcl_SetResult(interp, "1", TCL_VOLATILE);
    else
	Tcl_SetResult(interp, "0", TCL_VOLATILE);
    return (TCL_OK);
}

int Nget_def_color_cmd(Nv_data * data, Tcl_Interp * interp,	/* Current interpreter. */
		       int argc,	/* Number of arguments. */
		       char **argv	/* Argument strings. */
    )
{
    char clr[128];

    sprintf(clr, "%d", GS_default_draw_color());
    Tcl_SetResult(interp, clr, TCL_VOLATILE);
    return (TCL_OK);
}

int Nclear_cmd(Nv_data * data, Tcl_Interp * interp,	/* Current interpreter. */
	       int argc,	/* Number of arguments. */
	       char **argv	/* Argument strings. */
    )
{
    int clr;

    clr = (argc == 2) ? tcl_color_to_int(argv[1]) : data->BGcolor;

    GS_clear(clr);
    return (TCL_OK);
}

int Ndraw_wire_cmd(Nv_data * data, Tcl_Interp * interp,	/* Current interpreter. */
		   int argc,	/* Number of arguments. */
		   char **argv	/* Argument strings. */
    )
{
    int id;

    if (argc != 2)
	return (TCL_ERROR);
    id = get_idnum(argv[1]);

    GS_draw_wire(id);

    return (TCL_OK);
}


int Ndraw_X_cmd(Nv_data * data, Tcl_Interp * interp,	/* Current interpreter. */
		int argc,	/* Number of arguments. */
		char **argv	/* Argument strings. */
    )
{
    int id;
    float pt[2];
    double atof();

    if (argc != 4)
	return (TCL_ERROR);
    id = get_idnum(argv[1]);
    pt[0] = (float)atof(argv[2]);
    pt[1] = (float)atof(argv[3]);
    GS_draw_X(id, pt);

    return (TCL_OK);
}


/*****************************************************
 * Nset_Narrow 
 * Sets the North position and return world coords
 *****************************************************/
int Nset_Narrow_cmd(Nv_data * data, Tcl_Interp * interp,	/* Current interpreter. */
		    int argc,	/* Number of arguments. */
		    char **argv	/* Argument strings. */
    )
{
    int id;
    int pt[2];
    float coords[3];
    float len;
    char x[32], y[32], z[32];
    char *list[4];

    if (argc != 5)
	return (TCL_ERROR);
    pt[0] = (int)atoi(argv[1]);
    pt[1] = (int)atoi(argv[2]);
    id = (int)atoi(argv[3]);
    len = (float)atof(argv[4]);

    GS_set_Narrow(pt, id, coords);

    sprintf(x, "%f", coords[0]);
    sprintf(y, "%f", coords[1]);
    sprintf(z, "%f", coords[2]);

    list[0] = x;
    list[1] = y;
    list[2] = z;
    list[3] = NULL;

    interp->result = Tcl_Merge(3, list);
    interp->freeProc = TCL_DYNAMIC;

    return (TCL_OK);
}


 /*****************************************************
 * Ndraw_Narrow 
 * Draws the North Arrow 
 *****************************************************/
int Ndraw_Narrow_cmd(Nv_data * data, Tcl_Interp * interp,	/* Current interpreter. */
		     int argc,	/* Number of arguments. */
		     char **argv	/* Argument strings. */
    )
{
    float coords[3];
    double len;
    int arrow_color, text_color;

    if (argc != 7)
	return (TCL_ERROR);
    coords[0] = (int)atof(argv[1]);	/* X */
    coords[1] = (int)atof(argv[2]);	/* Y */
    coords[2] = (int)atof(argv[3]);	/* Z */
    len = (atof(argv[4]));
    arrow_color = (int)tcl_color_to_int(argv[5]);
    text_color = (int)tcl_color_to_int(argv[6]);

    FontBase = load_font(TOGL_BITMAP_HELVETICA_18);
    if (FontBase)
	gsd_north_arrow(coords, (float)len, FontBase,
			arrow_color, text_color);
    else
	fprintf(stderr, "Unable to load font\n");

    return (TCL_OK);
}



/*****************************************************
 * Ndraw_ScaleBar
 * Draws the Scalebar. Adaped from Ndraw_Narrow_cmd()
 *****************************************************/
int Ndraw_ScaleBar_cmd(Nv_data * data, Tcl_Interp * interp,	/* Current interpreter. */
		       int argc,	/* Number of arguments. */
		       char **argv	/* Argument strings. */
    )
{
    float coords[3];
    double len;
    int bar_color, text_color;

    if (argc != 7)
	return (TCL_ERROR);
    coords[0] = (int)atof(argv[1]);	/* X */
    coords[1] = (int)atof(argv[2]);	/* Y */
    coords[2] = (int)atof(argv[3]);	/* Z */
    len = (atof(argv[4]));
    bar_color = (int)tcl_color_to_int(argv[5]);
    text_color = (int)tcl_color_to_int(argv[6]);

    FontBase = load_font(TOGL_BITMAP_HELVETICA_18);
    if (FontBase)
	gsd_scalebar(coords, (float)len, FontBase, bar_color, text_color);
    else
	fprintf(stderr, "Unable to load font\n");

    return (TCL_OK);
}



int Ndraw_line_on_surf_cmd(Nv_data * data, Tcl_Interp * interp,	/* Current interpreter. */
			   int argc,	/* Number of arguments. */
			   char **argv	/* Argument strings. */
    )
{
    int id;
    float x1, y1, x2, y2;
    double atof();

    if (argc != 6)
	return (TCL_ERROR);
    id = get_idnum(argv[1]);
    x1 = (float)atof(argv[2]);
    y1 = (float)atof(argv[3]);
    x2 = (float)atof(argv[4]);
    y2 = (float)atof(argv[5]);

    GS_draw_line_onsurf(id, x1, y1, x2, y2);

    return (TCL_OK);
}

int Ndraw_model_cmd(Nv_data * data, Tcl_Interp * interp,	/* Current interpreter. */
		    int argc,	/* Number of arguments. */
		    char **argv	/* Argument strings. */
    )
{
    GS_draw_lighting_model();
    return (TCL_OK);
}

int Nsurf_draw_one_cmd(Nv_data * data, Tcl_Interp * interp,	/* Current interpreter. */
		       int argc,	/* Number of arguments. */
		       char **argv	/* Argument strings. */
    )
{
    float x, y, z;
    int num, w;

    /* Get position for Light 1 */
    num = 1;
    x = data->light[num].x;
    y = data->light[num].y;
    z = data->light[num].z;
    w = data->light[num].z;


    if (argc != 2) {
	Tcl_SetResult(interp, "Usage: Nsurf_draw_one surf_id", TCL_VOLATILE);
	return (TCL_ERROR);
    }

    /* re-initialize lights */
    GS_setlight_position(num, x, y, z, w);
    num = 2;
    GS_setlight_position(num, 0., 0., 1., 0);

    if (atoi(argv[1])) {
	GS_set_cancel(0);
	GS_set_draw(GSD_FRONT);
	GS_ready_draw();
	GS_draw_surf(atoi(argv[1]));
	GS_done_draw();
	GS_set_draw(GSD_BACK);
	GS_set_cancel(0);
    }

    return (TCL_OK);
}

int Nvect_draw_one_cmd(Nv_data * data, Tcl_Interp * interp,	/* Current interpreter. */
		       int argc,	/* Number of arguments. */
		       char **argv	/* Argument strings. */
    )
{

    if (argc != 2) {
	Tcl_SetResult(interp, "Usage: Nvect_draw_one vect_id", TCL_VOLATILE);
	return (TCL_ERROR);
    }

    if (atoi(argv[1])) {
	GS_set_cancel(0);
	GS_set_draw(GSD_BOTH);
	GS_ready_draw();
	GV_draw_vect(atoi(argv[1]));
	GS_done_draw();
	GS_set_draw(GSD_BACK);
	GS_set_cancel(0);
    }

    return (TCL_OK);
}

int Nsite_draw_one_cmd(Nv_data * data, Tcl_Interp * interp,	/* Current interpreter. */
		       int argc,	/* Number of arguments. */
		       char **argv	/* Argument strings. */
    )
{
    if (argc != 2) {
	Tcl_SetResult(interp, "Usage: Nsite_draw_one site_id", TCL_VOLATILE);
	return (TCL_ERROR);
    }

    if (atoi(argv[1])) {
	GS_set_cancel(0);
	GS_set_draw(GSD_BOTH);
	GS_ready_draw();
	GP_draw_site(atoi(argv[1]));
	GS_done_draw();
	GS_set_draw(GSD_BACK);
	GS_set_cancel(0);
    }

    return (TCL_OK);
}

int Nvol_draw_one_cmd(Nv_data * data, Tcl_Interp * interp,	/* Current interpreter. */
		      int argc,	/* Number of arguments. */
		      char **argv	/* Argument strings. */
    )
{

    if (argc != 2) {
	Tcl_SetResult(interp, "Usage: Nvol_draw_one vol_id", TCL_VOLATILE);
	return (TCL_ERROR);
    }

    if (atoi(argv[1])) {
	GS_set_cancel(0);
	GS_set_draw(GSD_BOTH);
	GS_ready_draw();
	GVL_draw_vol(atoi(argv[1]));
	GS_done_draw();
	GS_set_draw(GSD_BACK);
	GS_set_cancel(0);
    }

    return (TCL_OK);
}

/**********************
* Routine to routine value of
* auto-redraw check-button
***********************/
int Nauto_draw_cmd(Nv_data * data, Tcl_Interp * interp,	/* Current interpreter. */
		   int argc,	/* Number of arguments. */
		   char **argv	/* Argument strings. */
    )
{
    GS_set_cancel(0);
    auto_draw(data, interp);
    GS_set_cancel(0);
    return (TCL_OK);
}

/**********************
* Routine to call all draw
* functions ... rast, vect
* and sites
**********************/
int auto_draw(Nv_data * dc, Tcl_Interp * interp)
{
    const char *buf;
    int autodraw;
    char abuf[128];

    buf = Tcl_GetVar(interp, "auto_draw", TCL_GLOBAL_ONLY);
    if (buf != NULL) {
	autodraw = atoi(buf);
	sprintf(abuf, "%d", autodraw);
	Tcl_SetResult(interp, abuf, TCL_VOLATILE);
    }

    return (TCL_OK);
}


int Nsurf_draw_all_cmd(Nv_data * data, Tcl_Interp * interp,	/* Current interpreter. */
		       int argc,	/* Number of arguments. */
		       char **argv	/* Argument strings. */
    )
{
    GS_set_cancel(0);
    surf_draw_all(data, interp);
    GS_set_cancel(0);
    return (TCL_OK);
}

/* Sorts surfaces by max elevation, lowest to highest.
   Puts ordered id numbers in id_sort, leaving id_orig unchanged.
   Puts ordered indices of surfaces from id_orig in indices.
 */
int sort_surfs_max(int *surf, int *id_sort, int *indices, int num)
{
    int i, j;
    float maxvals[MAX_SURFS];
    float tmp, max = 0., tmin, tmax, tmid;

    for (i = 0; i < num; i++) {
	GS_get_zextents(surf[i], &tmin, &tmax, &tmid);
	if (i == 0)
	    max = tmax;
	else
	    max = max < tmax ? tmax : max;
	maxvals[i] = tmax;
    }
    for (i = 0; i < num; i++) {
	tmp = maxvals[0];
	indices[i] = 0;
	for (j = 0; j < num; j++) {
	    if (maxvals[j] < tmp) {
		tmp = maxvals[j];
		indices[i] = j;
	    }
	}
	maxvals[indices[i]] = max + 1;
	id_sort[i] = surf[indices[i]];
    }

    return (TCL_OK);
}

int surf_draw_all(Nv_data * dc, Tcl_Interp * interp)
{
    int i, nsurfs;
    int sortSurfs[MAX_SURFS], sorti[MAX_SURFS];
    int doclear;
    int *surf_list;
    float x, y, z;
    int num, w;

    /* Get position for Light 1 */
    num = 1;
    x = dc->light[num].x;
    y = dc->light[num].y;
    z = dc->light[num].z;
    w = dc->light[num].z;

    doclear = atoi(Tcl_GetVar(interp, "autoc", TCL_GLOBAL_ONLY));
#ifdef INDY
    GS_set_draw(GSD_BACK);
#else
    if (GS_transp_is_set())
	GS_set_draw(GSD_BOTH);
    else
	GS_set_draw(GSD_FRONT);	/* faster */
#endif

    surf_list = GS_get_surf_list(&nsurfs);
    sort_surfs_max(surf_list, sortSurfs, sorti, nsurfs);
    G_free(surf_list);

    if (doclear == 1)
	GS_clear(dc->BGcolor);

    GS_ready_draw();

    /* re-initialize lights */
    GS_setlight_position(num, x, y, z, w);
    num = 2;
    GS_setlight_position(num, 0., 0., 1., 0);

    for (i = 0; i < nsurfs; i++) {
	if (check_blank(interp, sortSurfs[i]) == 0) {
	    GS_draw_surf(sortSurfs[i]);
	}
    }

    /* GS_draw_cplane_fence params will change - surfs aren't used anymore */
    for (i = 0; i < MAX_CPLANES; i++) {
	if (dc->Cp_on[i])
	    GS_draw_cplane_fence(sortSurfs[0], sortSurfs[1], i);
    }

    GS_done_draw();

    GS_set_draw(GSD_BACK);
    return (TCL_OK);
}

/* Set cancel mode for drawing */
int
Nset_cancel_cmd(Nv_data * data, Tcl_Interp * interp, int argc, char **argv)
{
    int c;

    if ((argc != 2) || (Tcl_GetInt(interp, argv[1], &c) != TCL_OK)) {
	Tcl_SetResult(interp, "Usage: Nset_cancel [0 | 1]", TCL_VOLATILE);
	return (TCL_ERROR);
    }

    GS_set_cancel(c);

    return TCL_OK;
}

int Nvect_draw_all_cmd(Nv_data * data, Tcl_Interp * interp,	/* Current interpreter. */
		       int argc,	/* Number of arguments. */
		       char **argv	/* Argument strings. */
    )
{
    int i, nvects;
    int *vect_list;

    GS_set_cancel(0);
    vect_list = GV_get_vect_list(&nvects);

    /* in case transparency is set */
    GS_set_draw(GSD_BOTH);

    GS_ready_draw();

    for (i = 0; i < nvects; i++) {
	if (check_blank(interp, vect_list[i]) == 0) {
	    GV_draw_vect(vect_list[i]);
	}
    }
    G_free(vect_list);

    GS_done_draw();

    GS_set_draw(GSD_BACK);
    /*
     */

    GS_set_cancel(0);
    return (TCL_OK);
}

int Nsite_draw_all_cmd(Nv_data * data, Tcl_Interp * interp,	/* Current interpreter. */
		       int argc,	/* Number of arguments. */
		       char **argv	/* Argument strings. */
    )
{
    int i, nsites;
    int *site_list;

    GS_set_cancel(0);

    site_list = GP_get_site_list(&nsites);
    G_debug(3, "SITES_DRAW_ALL: n = %d\n", nsites);

    GS_set_draw(GSD_BOTH);	/* in case transparency is set */
    GS_ready_draw();

    for (i = 0; i < nsites; i++) {
	if (check_blank(interp, site_list[i]) == 0) {
	    G_debug(3, "DRAWING: site: %d \n", site_list[i]);
	    GP_draw_site(site_list[i]);
	}
    }
    G_free(site_list);

    G_debug(3, "Done drawing\n");

    GS_done_draw();

    GS_set_draw(GSD_BACK);
    GS_set_cancel(0);
    return (TCL_OK);
}

int Nvol_draw_all_cmd(Nv_data * data, Tcl_Interp * interp,	/* Current interpreter. */
		      int argc,	/* Number of arguments. */
		      char **argv	/* Argument strings. */
    )
{
    int i, nvols;
    int *vol_list;

    GS_set_cancel(0);
    vol_list = GVL_get_vol_list(&nvols);

    /* in case transparency is set */
    GS_set_draw(GSD_BOTH);

    GS_ready_draw();

    for (i = 0; i < nvols; i++) {
	if (check_blank(interp, vol_list[i]) == 0) {
	    GVL_draw_vol(vol_list[i]);
	}
    }
    G_free(vol_list);

    GS_done_draw();

    GS_set_draw(GSD_BACK);

    GS_set_cancel(0);
    return (TCL_OK);
}

int Nready_draw_cmd(void)
{
    GS_ready_draw();
    return (TCL_OK);
}

int Ndone_draw_cmd(void)
{
    GS_done_draw();
    return (TCL_OK);
}


/*
   This function checks if a specific map object should be blanked for
   a draw.  This option is used by one of the script tools for
   blanking maps during specific frames.
 */
int check_blank(Tcl_Interp * interp, int map_id)
{
    const char *val, **listArgv;
    int listArgc, rval, i;

    /* The list of maps to blank is kept in the tcl variable */
    /* "NVIZ_BLANK_MAPS".  If this variable doesn't exist then return */
    /* false, otherwise check to see if we can find the given map_id in */
    /* the blank list. */

    /* Check to see if variable exists */
    val = Tcl_GetVar(interp, "NVIZ_BLANK_MAPS", TCL_GLOBAL_ONLY);
    if (val == NULL)
	return 0;

    /* Variable exists so check for current map_id */
    if (Tcl_SplitList(interp, val, &listArgc, &listArgv) != TCL_OK) {
	return 0;
    }

    rval = 0;
    for (i = 0; (i < listArgc) & (!rval); i++) {
	if (map_id == atoi(listArgv[i])) {
	    rval = 1;
	}
    }

    Tcl_Free((char *)listArgv);
    return rval;

}

int Ndraw_legend_cmd(Nv_data * data, Tcl_Interp * interp,	/* Current interpreter. */
		     int argc,	/* Number of arguments. */
		     char **argv	/* Argument strings. */
    )
{
    int pt[4];
    int flags[5];
    int size;
    float range[2];
    char name[100];
    char font[100];

    if (argc != 15)
	return (TCL_ERROR);

    sprintf(name, "%s", argv[1]);
    sprintf(font, "%s", argv[2]);

    size = (int)atoi(argv[3]);
    flags[0] = (int)atoi(argv[4]);	/* vals */
    flags[1] = (int)atoi(argv[5]);	/* labels */
    flags[2] = (int)atoi(argv[6]);	/* invert */
    flags[3] = (int)atoi(argv[7]);	/* discrete */
    flags[4] = (int)atoi(argv[8]);	/* range */
    range[0] = (float)atof(argv[9]);	/* low range */
    range[1] = (float)atof(argv[10]);	/* high range */

    pt[0] = (int)atoi(argv[11]);
    pt[1] = (int)atoi(argv[12]);
    pt[2] = (int)atoi(argv[13]);
    pt[3] = (int)atoi(argv[14]);

    /* destroy display list of previous legend */
    if (legend_list) {
	GS_delete_list(legend_list);
	legend_list = 0;
    }
    FontBase = load_font(font);

    if (FontBase)
	legend_list = GS_draw_legend(name, FontBase, size, flags, range, pt);
    else
	fprintf(stderr, "Failed to initialize font\n");

    return (TCL_OK);
}

int Ndelete_list_cmd(Nv_data * data, Tcl_Interp * interp,	/* Current interpreter. */
		     int argc,	/* Number of arguments. */
		     char **argv	/* Argument strings. */
    )
{
    char list_name[20];
    int flag;

    if (argc != 3)
	return (TCL_ERROR);

    sprintf(list_name, "%s", argv[1]);
    /* flag arg determines how list is removed */
    flag = atoi(argv[2]);

    if (strcmp(list_name, "legend") == 0) {
	if (legend_list) {
	    GS_delete_list(legend_list);
	}
    }
    if (strcmp(list_name, "label") == 0) {
	if (flag)
	    gsd_remove_curr();	/* remove one label */
	else
	    gsd_remove_all();	/* remove all labels */
    }


    return (TCL_OK);

}


/**************************************************************/
int Ndraw_fringe_cmd(Nv_data * data, Tcl_Interp * interp,	/* Current interpreter. */
		     int argc,	/* Number of arguments. */
		     char **argv	/* Argument strings. */
    )
{
    int id;
    int color;
    float elev;
    int flags[4];

    id = (int)atoi(argv[1]);
    color = (int)tcl_color_to_int(argv[2]);
    elev = (float)atof(argv[3]);
    flags[0] = (int)atoi(argv[4]);
    flags[1] = (int)atoi(argv[5]);
    flags[2] = (int)atoi(argv[6]);
    flags[3] = (int)atoi(argv[7]);

    GS_draw_fringe(id, color, elev, flags);

    return (TCL_OK);
}


/****************************************************
 * Manually set viewport to specified size
 * needed when manually changing canvas size
****************************************************/
int Nset_viewport_cmd(Nv_data * data, Tcl_Interp * interp,	/* Current interpreter. */
		      int argc,	/* Number of arguments. */
		      char **argv)
{				/* Argument strings. */
    int x, y;


    if (argc != 3) {
	Tcl_SetResult(interp, "Usage: Nset_viewport width, height",
		      TCL_VOLATILE);
	return (TCL_ERROR);
    }

    x = (int)atoi(argv[1]);
    y = (int)atoi(argv[2]);

    /* manually set viewport dimension */
    GS_set_viewport(0, x, 0, y);

    return (TCL_OK);

}
