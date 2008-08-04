/*
   volume.c --

   This file contains routines to manipulate with volumes
 */

/* Nvision includes */
#include <stdlib.h>
#include <grass/gis.h>
#include "interface.h"

/* set polygon resolution for isosurfaces */
int isosurf_set_res(int id, Tcl_Interp * interp, int argc, char *argv[])
{
    int xres, yres, zres;

    if (argc < 6) {
	Tcl_SetResult(interp,
		      "Usage: <map_obj> isosurf set_res xres yres zres",
		      TCL_VOLATILE);
	return (TCL_ERROR);
    }

    xres = atoi(argv[3]);
    yres = atoi(argv[4]);
    zres = atoi(argv[5]);

    GVL_isosurf_set_drawres(id, xres, yres, zres);

    return (TCL_OK);
}

/* get polygon resolution for isosurfaces */
int isosurf_get_res(int id, Tcl_Interp * interp, int argc, char *argv[])
{
    int xres, yres, zres;
    char x[32], y[32], z[32];
    char *list[4];

    GVL_isosurf_get_drawres(id, &xres, &yres, &zres);

    sprintf(x, "%d", xres);
    sprintf(y, "%d", yres);
    sprintf(z, "%d", zres);

    list[0] = x;
    list[1] = y;
    list[2] = z;
    list[3] = NULL;

    interp->result = Tcl_Merge(3, list);
    interp->freeProc = TCL_DYNAMIC;

    return (TCL_OK);
}

/* set drawmode for isosurfaces */
int isosurf_set_drawmode(int id, Tcl_Interp * interp, int argc, char *argv[])
{
    int mode;

    /* Set the mode flags by parsing the surface style and shading arguments */
    mode = 0;

    if (!strcmp(argv[3], "gouraud"))
	mode |= DM_GOURAUD;
    else if (!strcmp(argv[3], "flat"))
	mode |= DM_FLAT;
    else {
	Tcl_SetResult(interp,
		      "Usage: <map_obj> isosurf set_drawmode [ gouraud | flat ]",
		      TCL_VOLATILE);
	return (TCL_ERROR);
    }

    /* Set the appropriate mode in the gsf library */
    GVL_isosurf_set_drawmode(id, mode);

    return (TCL_OK);
}

/* get drawmode for isosurfaces */
int isosurf_get_drawmode(int id, Tcl_Interp * interp, int argc, char *argv[])
{
    int mode;
    char shade[32];

    if (GVL_isosurf_get_drawmode(id, &mode) == -1) {
	Tcl_SetResult(interp,
		      "Error: id in GVL_isosurf_get_drawmode is invalid.",
		      TCL_VOLATILE);
	return (TCL_ERROR);
    }

    /* Parse mode returned for shade style */
    G_debug(3, "isosurf_get_drawmode: mode %d", mode);
    if (mode & DM_GOURAUD)
	strcpy(shade, "gouraud");
    else if (mode & DM_FLAT)
	strcpy(shade, "flat");
    else {
	Tcl_SetResult(interp,
		      "Internal Error: unknown shade style returned in GVL_isosurf_get_drawmode",
		      TCL_VOLATILE);
	return (TCL_ERROR);
    }

    Tcl_SetResult(interp, shade, TCL_VOLATILE);

    return (TCL_OK);
}

/* num of isosurface */
int isosurf_num_isosurfs(int id, Tcl_Interp * interp, int argc, char *argv[])
{
    int num;
    char ret[32];

    num = GVL_isosurf_num_isosurfs(id);
    sprintf(ret, "%d", num);

    Tcl_SetResult(interp, ret, TCL_VOLATILE);
    return (TCL_OK);
}

/* add isosurface */
int isosurf_add(int id, Tcl_Interp * interp, int argc, char *argv[])
{
    if (GVL_isosurf_add(id) == -1) {
	Tcl_SetResult(interp, "Error: unable to add isosurface.",
		      TCL_VOLATILE);
	return (TCL_ERROR);
    }

    return (TCL_OK);
}

/* delete isosurface */
int isosurf_del(int id, Tcl_Interp * interp, int argc, char *argv[])
{
    int isosurf_id;

    if (argc != 4) {
	Tcl_SetResult(interp,
		      "Usage: <map_obj> isosurf del isosurf_id",
		      TCL_VOLATILE);
	return (TCL_ERROR);
    }

    isosurf_id = atoi(argv[3]);

    if (GVL_isosurf_del(id, isosurf_id) == -1) {
	Tcl_SetResult(interp, "Error: unable to delete isosurface.",
		      TCL_VOLATILE);
	return (TCL_ERROR);
    }

    return (TCL_OK);
}

/* move isosurf up in draw order */
int isosurf_move_up(int id, Tcl_Interp * interp, int argc, char *argv[])
{
    int isosurf_id;

    if (argc != 4) {
	Tcl_SetResult(interp,
		      "Usage: <map_obj> isosurf move_up isosurf_id",
		      TCL_VOLATILE);
	return (TCL_ERROR);
    }

    isosurf_id = atoi(argv[3]);

    if (GVL_isosurf_move_up(id, isosurf_id) == -1) {
	Tcl_SetResult(interp, "Error: unable change isosurf draw order",
		      TCL_VOLATILE);
	return (TCL_ERROR);
    }

    return (TCL_OK);
}

/* move isosurf down in draw order */
int isosurf_move_down(int id, Tcl_Interp * interp, int argc, char *argv[])
{
    int isosurf_id;

    if (argc != 4) {
	Tcl_SetResult(interp,
		      "Usage: <map_obj> isosurf move_down isosurf_id",
		      TCL_VOLATILE);
	return (TCL_ERROR);
    }

    isosurf_id = atoi(argv[3]);

    if (GVL_isosurf_move_down(id, isosurf_id) == -1) {
	Tcl_SetResult(interp, "Error: unable change isosurf draw order",
		      TCL_VOLATILE);
	return (TCL_ERROR);
    }

    return (TCL_OK);
}

/* get isosurface attribute */
int isosurf_get_att(int id, Tcl_Interp * interp, int argc, char *argv[])
{
    int set, isosurf_id;
    float c;
    char mapname[100], temp[100];

    if (argc != 5) {
	Tcl_SetResult(interp,
		      "Usage: <map_obj> isosurf get_att isosurf_id [threshold | color | mask | transp | shin | emi]",
		      TCL_VOLATILE);
	return (TCL_ERROR);
    }

    isosurf_id = atoi(argv[3]);

    GVL_isosurf_get_att(id, isosurf_id, isosurf_att_atoi(argv[4]), &set, &c,
			mapname);

    switch (set) {
    case NOTSET_ATT:
	Tcl_AppendElement(interp, "unset");
	break;
    case MAP_ATT:
	Tcl_AppendElement(interp, "map");
	Tcl_AppendElement(interp, mapname);
	break;
    case CONST_ATT:
	Tcl_AppendElement(interp, "const");
	sprintf(temp, "%f", c);
	Tcl_AppendElement(interp, temp);
	break;
    case FUNC_ATT:
	/* not implemented yet */
	break;
    }

    return (TCL_OK);
}

/* set isosurface attribute */
int isosurf_set_att(int id, Tcl_Interp * interp, int argc, char *argv[])
{
    int att, ret, isosurf_id;
    float temp;
    double atof();

    if (argc < 6) {
	Tcl_SetResult(interp,
		      "Usage: <map_obj> isosurf set_att isosurf_id [threshold | color | mask | transp | shin | emi] [file_name | constant value]",
		      TCL_VOLATILE);
	return (TCL_ERROR);
    }

    isosurf_id = atoi(argv[3]);
    att = isosurf_att_atoi(argv[4]);

    if (att < 0) {
	Tcl_SetResult(interp,
		      "Internal Error: unknown attribute name in set_att",
		      TCL_VOLATILE);
	return (TCL_ERROR);
    }

    if (!strcmp(argv[5], "constant")) {
	temp = (float)atof(argv[6]);

	if (att == ATT_COLOR) {
	    int r, g, b;

	    r = (((int)temp) & 0xff0000) >> 16;
	    g = (((int)temp) & 0x00ff00) >> 8;
	    b = (((int)temp) & 0x0000ff);
	    temp = r + (g << 8) + (b << 16);
	}

	ret = GVL_isosurf_set_att_const(id, isosurf_id, att, temp);
    }
    else {
	ret = GVL_isosurf_set_att_map(id, isosurf_id, att, argv[5]);
    }

    return (ret < 0) ? (TCL_ERROR) : (TCL_OK);
}

/* unset isosurface attribute */
int isosurf_unset_att(int id, Tcl_Interp * interp, int argc, char *argv[])
{
    int att, isosurf_id;

    if (argc != 5) {
	Tcl_SetResult(interp,
		      "Usage: <map_obj> isosurf unset_att isosurf_id [threshold | color | mask | transp | shin | emi]",
		      TCL_VOLATILE);
	return (TCL_ERROR);
    }

    isosurf_id = atoi(argv[3]);
    att = isosurf_att_atoi(argv[4]);

    return ((0 >
	     GVL_isosurf_unset_att(id, isosurf_id,
				   att)) ? TCL_ERROR : TCL_OK);
}

/* map textual attribute name to internal code. */
int isosurf_att_atoi(char *attname)
{
    if (!strncmp(attname, "threshold", 4))
	return (ATT_TOPO);
    else if (!strncmp(attname, "color", 5))
	return (ATT_COLOR);
    else if (!strncmp(attname, "mask", 4))
	return (ATT_MASK);
    else if (!strncmp(attname, "transp", 6))
	return (ATT_TRANSP);
    else if (!strncmp(attname, "shin", 4))
	return (ATT_SHINE);
    else if (!strncmp(attname, "emi", 3))
	return (ATT_EMIT);
    else
	return (-1);
}

/* get isosurface mask mode */
int isosurf_get_mask_mode(int id, Tcl_Interp * interp, int argc, char *argv[])
{
    int mode, isosurf_id;

    if (argc != 4) {
	Tcl_SetResult(interp,
		      "Usage: <map_obj> isosurf get_mask_mode isosurf_id",
		      TCL_VOLATILE);
	return (TCL_ERROR);
    }

    isosurf_id = atoi(argv[3]);

    GVL_isosurf_get_maskmode(id, isosurf_id, &mode);
    sprintf(interp->result, "%d", mode);

    return (TCL_OK);
}

/* set isosurface mask mode */
int isosurf_set_mask_mode(int id, Tcl_Interp * interp, int argc, char *argv[])
{
    int mode, isosurf_id;

    if (argc != 5) {
	Tcl_SetResult(interp,
		      "Usage: <map_obj> isosurf set_mask_mode isosurf_id [0 | 1]",
		      TCL_VOLATILE);
	return (TCL_ERROR);
    }

    isosurf_id = atoi(argv[3]);

    if (Tcl_GetBoolean(interp, argv[4], &mode) != TCL_OK) {
	Tcl_SetResult(interp, "Error: must be BOOLEAN", TCL_VOLATILE);
	return (TCL_ERROR);
    }
    else {
	GVL_isosurf_set_maskmode(id, isosurf_id, mode);
    }

    return (TCL_OK);
}

/* get isosurface flags */
int isosurf_get_flags(int id, Tcl_Interp * interp, int argc, char *argv[])
{
    int inout, isosurf_id;
    char tmp[32];

    if (argc != 4) {
	Tcl_SetResult(interp,
		      "Usage: <map_obj> isosurf get_flags isosurf_id",
		      TCL_VOLATILE);
	return (TCL_ERROR);
    }

    isosurf_id = atoi(argv[3]);

    GVL_isosurf_get_flags(id, isosurf_id, &inout);

    sprintf(tmp, "%d", inout);
    Tcl_AppendElement(interp, tmp);

    return (TCL_OK);
}

/* set isosurface flags */
int isosurf_set_flags(int id, Tcl_Interp * interp, int argc, char *argv[])
{
    int isosurf_id;

    isosurf_id = atoi(argv[3]);

    if (argc != 5) {
	Tcl_SetResult(interp,
		      "Usage: <map_obj> isosurf set_flags isosurf_id inout",
		      TCL_VOLATILE);
	return (TCL_ERROR);
    }

    GVL_isosurf_set_flags(id, isosurf_id, atoi(argv[4]));

    return (TCL_OK);
}

/* set polygon resolution for slices */
int slice_set_res(int id, Tcl_Interp * interp, int argc, char *argv[])
{
    int xres, yres, zres;

    if (argc < 6) {
	Tcl_SetResult(interp,
		      "Usage: <map_obj> slice set_res xres yres zres",
		      TCL_VOLATILE);
	return (TCL_ERROR);
    }

    xres = atoi(argv[3]);
    yres = atoi(argv[4]);
    zres = atoi(argv[5]);

    GVL_slice_set_drawres(id, xres, yres, zres);

    return (TCL_OK);
}

/* get polygon resolution for slices */
int slice_get_res(int id, Tcl_Interp * interp, int argc, char *argv[])
{
    int xres, yres, zres;
    char x[32], y[32], z[32];
    char *list[4];

    GVL_slice_get_drawres(id, &xres, &yres, &zres);

    sprintf(x, "%d", xres);
    sprintf(y, "%d", yres);
    sprintf(z, "%d", zres);

    list[0] = x;
    list[1] = y;
    list[2] = z;
    list[3] = NULL;

    interp->result = Tcl_Merge(3, list);
    interp->freeProc = TCL_DYNAMIC;

    return (TCL_OK);
}

/* set drawmode for slices */
int slice_set_drawmode(int id, Tcl_Interp * interp, int argc, char *argv[])
{
    int mode;

    /* Set the mode flags by parsing the surface style and shading arguments */
    mode = 0;

    if (argc != 4) {
	Tcl_SetResult(interp,
		      "Usage: <map_obj> slice set_drawmode [ gouraud | flat ]",
		      TCL_VOLATILE);
	return (TCL_ERROR);
    }

    if (!strcmp(argv[3], "gouraud"))
	mode |= DM_GOURAUD;
    else if (!strcmp(argv[3], "flat"))
	mode |= DM_FLAT;
    else {
	Tcl_SetResult(interp,
		      "Usage: <map_obj> slice set_drawmode [ gouraud | flat ]",
		      TCL_VOLATILE);
	return (TCL_ERROR);
    }

    /* Set the appropriate mode in the gsf library */
    GVL_slice_set_drawmode(id, mode);

    return (TCL_OK);
}

/* get drawmode for slices */
int slice_get_drawmode(int id, Tcl_Interp * interp, int argc, char *argv[])
{
    int mode;
    char shade[32];

    if (GVL_slice_get_drawmode(id, &mode) == -1) {
	Tcl_SetResult(interp,
		      "Error: id in GVL_slice_get_drawmode() is invalid.",
		      TCL_VOLATILE);
	return (TCL_ERROR);
    }

    /* Parse mode returned for shade style */
    G_debug(3, "slice_get_drawmode: mode %d", mode);
    if (mode & DM_GOURAUD)
	strcpy(shade, "gouraud");
    else if (mode & DM_FLAT)
	strcpy(shade, "flat");
    else {
	Tcl_SetResult(interp,
		      "Internal Error: unknown shade style returned in GVL_slice_get_get_drawmode",
		      TCL_VOLATILE);
	return (TCL_ERROR);
    }

    Tcl_SetResult(interp, shade, TCL_VOLATILE);

    return (TCL_OK);
}

/* num of slices */
int slice_num_slices(int id, Tcl_Interp * interp, int argc, char *argv[])
{
    int num;
    char ret[32];

    num = GVL_slice_num_slices(id);
    sprintf(ret, "%d", num);

    Tcl_SetResult(interp, ret, TCL_VOLATILE);
    return (TCL_OK);
}

/* get slice position */
int slice_get_pos(int id, Tcl_Interp * interp, int argc, char *argv[])
{
    int slice_id, dir;
    char rx1[32], rx2[32], ry1[32], ry2[32], rz1[32], rz2[32], rdir[32];
    float x1, x2, y1, y2, z1, z2;
    char *list[8];

    if (argc != 4) {
	Tcl_SetResult(interp,
		      "Usage: <map_obj> slice get_pos slice_id",
		      TCL_VOLATILE);
	return (TCL_ERROR);
    }

    slice_id = atoi(argv[3]);

    if (GVL_slice_get_pos(id, slice_id, &x1, &x2, &y1, &y2, &z1, &z2, &dir) ==
	-1) {
	Tcl_SetResult(interp, "Error: unable to get slice position",
		      TCL_VOLATILE);
	return (TCL_ERROR);
    }

    sprintf(rx1, "%.3f", x1);
    sprintf(rx2, "%.3f", x2);
    sprintf(ry1, "%.3f", y1);
    sprintf(ry2, "%.3f", y2);
    sprintf(rz1, "%.3f", z1);
    sprintf(rz2, "%.3f", z2);
    sprintf(rdir, "%d", dir);

    list[0] = rx1;
    list[1] = rx2;
    list[2] = ry1;
    list[3] = ry2;
    list[4] = rz1;
    list[5] = rz2;
    list[6] = rdir;
    list[7] = NULL;

    interp->result = Tcl_Merge(7, list);
    interp->freeProc = TCL_DYNAMIC;

    return (TCL_OK);
}

/* set slice position */
int slice_set_pos(int id, Tcl_Interp * interp, int argc, char *argv[])
{
    int slice_id, dir;
    float x1, x2, y1, y2, z1, z2;
    double atof();

    if (argc != 11) {
	Tcl_SetResult(interp,
		      "Usage: <map_obj> slice set_pos slice_id x1 y1 x2 y2 z1 z2 direction",
		      TCL_VOLATILE);
	return (TCL_ERROR);
    }

    slice_id = atoi(argv[3]);

    x1 = (float)atof(argv[4]);
    x2 = (float)atof(argv[6]);
    y1 = (float)atof(argv[5]);
    y2 = (float)atof(argv[7]);
    z1 = (float)atof(argv[8]);
    z2 = (float)atof(argv[9]);

    dir = atoi(argv[10]);

    if (GVL_slice_set_pos(id, slice_id, x1, y1, x2, y2, z1, z2, dir) == -1) {
	Tcl_SetResult(interp, "Error: unable to set slice position",
		      TCL_VOLATILE);
	return (TCL_ERROR);
    }

    return (TCL_OK);
}

/* add slice */
int slice_add(int id, Tcl_Interp * interp, int argc, char *argv[])
{
    if (GVL_slice_add(id) == -1) {
	Tcl_SetResult(interp, "Error: unable to add slice", TCL_VOLATILE);
	return (TCL_ERROR);
    }

    return (TCL_OK);
}

/* delete slice */
int slice_del(int id, Tcl_Interp * interp, int argc, char *argv[])
{
    int slice_id;

    if (argc != 4) {
	Tcl_SetResult(interp,
		      "Usage: <map_obj> slice del slice_id", TCL_VOLATILE);
	return (TCL_ERROR);
    }

    slice_id = atoi(argv[3]);

    if (GVL_slice_del(id, slice_id) == -1) {
	Tcl_SetResult(interp, "Error: unable to delete slice", TCL_VOLATILE);
	return (TCL_ERROR);
    }

    return (TCL_OK);
}

/* move slice up in draw order */
int slice_move_up(int id, Tcl_Interp * interp, int argc, char *argv[])
{
    int slice_id;

    if (argc != 4) {
	Tcl_SetResult(interp,
		      "Usage: <map_obj> slice move_up slice_id",
		      TCL_VOLATILE);
	return (TCL_ERROR);
    }

    slice_id = atoi(argv[3]);

    if (GVL_slice_move_up(id, slice_id) == -1) {
	Tcl_SetResult(interp, "Error: unable change slice draw order",
		      TCL_VOLATILE);
	return (TCL_ERROR);
    }

    return (TCL_OK);
}

/* move slice down in draw order */
int slice_move_down(int id, Tcl_Interp * interp, int argc, char *argv[])
{
    int slice_id;

    if (argc != 4) {
	Tcl_SetResult(interp,
		      "Usage: <map_obj> slice move_down slice_id",
		      TCL_VOLATILE);
	return (TCL_ERROR);
    }

    slice_id = atoi(argv[3]);

    if (GVL_slice_move_down(id, slice_id) == -1) {
	Tcl_SetResult(interp, "Error: unable change slice draw order",
		      TCL_VOLATILE);
	return (TCL_ERROR);
    }

    return (TCL_OK);
}

/* set slice transparency */
int slice_get_transp(int id, Tcl_Interp * interp, int argc, char *argv[])
{
    int slice_id, transp;

    if (argc != 4) {
	Tcl_SetResult(interp,
		      "Usage: <map_obj> slice get_transp slice_id",
		      TCL_VOLATILE);
	return (TCL_ERROR);
    }

    slice_id = atoi(argv[3]);

    GVL_slice_get_transp(id, slice_id, &transp);

    sprintf(interp->result, "%d", transp);

    return (TCL_OK);
}

/* set slice transparency */
int slice_set_transp(int id, Tcl_Interp * interp, int argc, char *argv[])
{
    int slice_id, transp;

    if (argc != 5) {
	Tcl_SetResult(interp,
		      "Usage: <map_obj> slice set_transp slice_id value",
		      TCL_VOLATILE);
	return (TCL_ERROR);
    }

    slice_id = atoi(argv[3]);
    transp = atoi(argv[4]);

    /* set the appropriate tranparency in the gsf library */
    GVL_slice_set_transp(id, slice_id, transp);

    return (TCL_OK);
}
