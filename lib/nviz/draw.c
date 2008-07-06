/*!
  \file draw.c
 
  \brief Nviz library -- Draw map objects to GLX context
  
  COPYRIGHT: (C) 2008 by the GRASS Development Team

  This program is free software under the GNU General Public
  License (>=v2). Read the file COPYING that comes with GRASS
  for details.

  Based on visualization/nviz/src/draw.c and
  visualization/nviz/src/togl_flythrough.c

  \author Updated/modified by Martin Landa <landa.martin gmail.com> (Google SoC 2008)

  \date 2008
*/

#include <grass/nviz.h>

static int sort_surfs_max(int *, int *, int *, int);

/*!
  \brief Draw all loaded surfaces

  \param dc nviz data

  \return 1
*/
int Nviz_draw_all_surf(nv_data *dc)
{
    int i, nsurfs;
    int sortSurfs[MAX_SURFS], sorti[MAX_SURFS];
    int *surf_list;
    float x, y, z;
    int num, w;

/* Get position for Light 1 */
    num = 1;
    x = dc->light[num].x;
    y = dc->light[num].y;
    z = dc->light[num].z;
    w = dc->light[num].z;

    surf_list = GS_get_surf_list(&nsurfs);

    sort_surfs_max(surf_list, sortSurfs, sorti, nsurfs);

    G_free (surf_list);

    /* re-initialize lights */
    GS_setlight_position(num, x, y, z, w);
    num = 2;
    GS_setlight_position(num, 0., 0., 1., 0);

    for (i = 0; i < nsurfs; i++) {
	GS_draw_surf(sortSurfs[i]);
    }
    
    /* GS_draw_cplane_fence params will change - surfs aren't used anymore */
    for (i = 0; i < MAX_CPLANES; i++) {
	if (dc->cp_on[i])
	    GS_draw_cplane_fence(sortSurfs[0], sortSurfs[1], i);
    }

    return 1;
}

/*!
  \brief Sorts surfaces by max elevation, lowest to highest.

  Puts ordered id numbers in id_sort, leaving id_orig unchanged.
  Puts ordered indices of surfaces from id_orig in indices.

  \param surf pointer to surface array
  \param id_sort
  \param indices
  \param num

  \return 1
*/
int sort_surfs_max(int *surf, int *id_sort, int *indices, int num)
{
    int i, j;
    float maxvals[MAX_SURFS];
    float tmp, max=0., tmin, tmax, tmid;

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

    return 1;
}

/*!
  \brief Draw all loaded vector sets

  \param dc nviz data

  \return 1
*/
int Nviz_draw_all_vect(nv_data *dc)
{
    // GS_set_cancel(0);

    /* in case transparency is set */
    GS_set_draw(GSD_BOTH);

    GS_ready_draw();

    GV_alldraw_vect();

    GS_done_draw();
    
    GS_set_draw(GSD_BACK);

    // GS_set_cancel(0);

    return 1;
}

/*!
  \brief Draw all map objects (in full resolution) and decorations

  \param data nviz data
*/
int Nviz_draw_all(nv_data *data)
{
    int draw_surf, draw_vect, draw_site, draw_vol;
    int draw_north_arrow, arrow_x, draw_label, draw_legend;
    int draw_fringe, draw_scalebar, draw_bar_x;
    // const char* draw_is_drawing = Tcl_GetVar(interp, "is_drawing", TCL_GLOBAL_ONLY);
    // const char* EMPTYSTRING = "";

    draw_surf = 1;
    draw_vect = 1;
    draw_site = 0;
    draw_vol = 0;
    draw_north_arrow = 0;
    arrow_x = 0;
    draw_label = 0;
    draw_legend = 0;
    draw_fringe = 0;
    draw_scalebar = 0;
    draw_bar_x = 0;

    /*
    if (buf_is_drawing && atoi(buf_is_drawing))
	return (TCL_OK);
    */

    // Tcl_SetVar(interp, "is_drawing", "1", TCL_GLOBAL_ONLY);
    
    GS_set_draw(GSD_BACK); /* needs to be BACK to avoid flickering */

    GS_ready_draw();

    GS_clear(data->bgcolor);

/*
    buf_surf     = Tcl_GetVar(interp, "surface", TCL_GLOBAL_ONLY);
    buf_vect     = Tcl_GetVar(interp, "vector", TCL_GLOBAL_ONLY);
    buf_site     = Tcl_GetVar(interp, "sites", TCL_GLOBAL_ONLY);
    buf_vol      = Tcl_GetVar(interp, "volume", TCL_GLOBAL_ONLY);
    buf_north_arrow = Tcl_GetVar(interp, "n_arrow", TCL_GLOBAL_ONLY);
    arrow_x      = Tcl_GetVar(interp, "n_arrow_x", TCL_GLOBAL_ONLY);
    buf_label    = Tcl_GetVar(interp, "labels", TCL_GLOBAL_ONLY);
    buf_legend   = Tcl_GetVar(interp, "legend", TCL_GLOBAL_ONLY);
    buf_fringe   = Tcl_GetVar(interp, "fringe", TCL_GLOBAL_ONLY);
    buf_scalebar = Tcl_GetVar(interp, "scalebar", TCL_GLOBAL_ONLY);
    bar_x        = Tcl_GetVar(interp, "scalebar_x", TCL_GLOBAL_ONLY);
*/  
    if (draw_surf)
	Nviz_draw_all_surf(data);

    if (draw_vect)
	Nviz_draw_all_vect (data);

    /*
    if (draw_site)
	site_draw_all_together(data, interp);

    if (draw_vol)
	vol_draw_all_cmd(data, interp, argc, argv);
    */
    GS_done_draw();
    GS_set_draw(GSD_BACK);
    
    /*
    if (!draw_north_arrow)
	draw_north_arrow = EMPTYSTRING; 
    
    if (!arrow_x)
	arrow_x = EMPTYSTRING; 
    
    if (!draw_scalebar)
	draw_scalebar = EMPTYSTRING; 
    
    if (!draw_bar_x)
	bar_x = EMPTYSTRING; 
    
    if (!draw_fringe)
	draw_fringe = EMPTYSTRING; 
    
    if (!draw_label)
	draw_label = EMPTYSTRING; 
    
    if (!draw_legend)
	draw_legend = EMPTYSTRING; 
    */
    /* Draw decorations */
    
    /* North Arrow
    if (atoi(draw_north_arrow) == 1 && atoi(arrow_x) != 999 ) {
	const char *arrow_y, *arrow_z, *arrow_len;
	float coords[3], len;
	int arrow_clr, text_clr;
	
	arrow_y = Tcl_GetVar(interp, "n_arrow_y", TCL_GLOBAL_ONLY);
	arrow_z = Tcl_GetVar(interp, "n_arrow_z", TCL_GLOBAL_ONLY);
	arrow_len = Tcl_GetVar(interp, "n_arrow_size", TCL_GLOBAL_ONLY);
	arrow_clr = (int) tcl_color_to_int(Tcl_GetVar(interp, "arw_clr", TCL_GLOBAL_ONLY));
	text_clr = (int) tcl_color_to_int(Tcl_GetVar(interp, "arw_text_clr", TCL_GLOBAL_ONLY));
	coords[0] = atoi(arrow_x);
	coords[1] = atoi(arrow_y);
	coords[2] = atoi(arrow_z);
	len = atof(arrow_len);
	
	FontBase = load_font(TOGL_BITMAP_HELVETICA_18);
	gsd_north_arrow(coords, len, FontBase, arrow_clr, text_clr);
    }
    */

    /* Scale Bar
    if (atoi(draw_scalebar) == 1 && atoi(bar_x) != 999 ) {
	const char *bar_y, *bar_z, *bar_len;
	float coords[3], len;
	int bar_clr, text_clr;
	
	bar_y = Tcl_GetVar(interp, "scalebar_y", TCL_GLOBAL_ONLY);
	bar_z = Tcl_GetVar(interp, "scalebar_z", TCL_GLOBAL_ONLY);
	bar_len = Tcl_GetVar(interp, "scalebar_size", TCL_GLOBAL_ONLY);
	bar_clr = (int) tcl_color_to_int(Tcl_GetVar(interp, "bar_clr", TCL_GLOBAL_ONLY));
	text_clr = (int) tcl_color_to_int(Tcl_GetVar(interp, "bar_text_clr", TCL_GLOBAL_ONLY));
	coords[0] = atoi(bar_x);
	coords[1] = atoi(bar_y);
	coords[2] = atoi(bar_z);
	len = atof(bar_len);
	
	FontBase = load_font(TOGL_BITMAP_HELVETICA_18);
	gsd_scalebar(coords, len, FontBase, bar_clr, bar_clr);
    }
    */

    /* fringe
    if (atoi(draw_fringe) == 1) {
	const char *fringe_ne, *fringe_nw, *fringe_se, *fringe_sw;
	const char *surf_id;
	int flags[4], id;
	int fringe_clr;
	float fringe_elev;
	
	fringe_clr = (int) tcl_color_to_int(Tcl_GetVar(interp, "fringe_color", TCL_GLOBAL_ONLY));
	fringe_elev = (float) atof(Tcl_GetVar(interp, "fringe_elev", TCL_GLOBAL_ONLY));
	fringe_ne = Tcl_GetVar(interp, "fringe_ne", TCL_GLOBAL_ONLY);
	fringe_nw = Tcl_GetVar(interp, "fringe_nw", TCL_GLOBAL_ONLY);
	fringe_se = Tcl_GetVar(interp, "fringe_se", TCL_GLOBAL_ONLY);
	fringe_sw = Tcl_GetVar(interp, "fringe_sw", TCL_GLOBAL_ONLY);
	flags[0] = atoi(fringe_nw);
	flags[1] = atoi(fringe_ne);
	flags[2] = atoi(fringe_sw);
	flags[3] = atoi(fringe_se);
	surf_id = Tcl_GetVar2(interp, "Nv_", "CurrSurf", TCL_GLOBAL_ONLY);
	id = atoi(surf_id);
	
	GS_draw_fringe(id, fringe_clr, fringe_elev, flags);
    }
    */

    /* Legend and/or labels
    if (atoi(draw_label) == 1 || atoi(draw_legend) == 1)
	GS_draw_all_list();
    */

    // Tcl_SetVar(interp, "is_drawing", "0", TCL_GLOBAL_ONLY);
    // flythrough_postdraw_cb();
    
    return 1;
}

/*!
  \brief Draw all surfaces in wireframe

  \param dc nviz data

  \return 1
*/
int Nviz_draw_quick(nv_data *data)
{
    GS_set_draw(GSD_BACK);
    
    GS_ready_draw();

    GS_clear(data->bgcolor);

    /* draw surfaces */
    GS_alldraw_wire();

    /* draw vector */
    /* GV_alldraw_fastvect(); is broken */
    /* GV_alldraw_vect(); */

    /*
    vol_list = GVL_get_vol_list(&max);
    max = GVL_num_vols();
    for (i = 0; i < max; i++) {
	if (check_blank(interp, vol_list[i]) == 0) {
	    GVL_draw_wire(vol_list[i]);
	}
    }
    */

    GS_done_draw();

    // flythrough_postdraw_cb();

    return 1;
}

