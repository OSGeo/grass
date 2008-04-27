#include <stdlib.h>
#include <string.h>
#include "global.h"

static void write_pnts(struct Map_info *, char *, int, int, int);

int add_polyline(struct dxf_file *dxf, struct Map_info *Map)
{
    int code;
    char layer[DXF_BUF_SIZE];
    int layer_flag = 0;		/* indicates if a layer name has been found */
    int polyline_flag = 0;	/* indicates the type of polyline */
    int warn_flag66 = 1;	/* indicates if error message printed once */
    int warn_flag70 = 1;	/* indicates if error message printed once */
    int vert_flag;		/* indicates that vertices are following */
    int xflag = 0;		/* indicates if a x value has been found */
    int yflag = 0;		/* indicates if a y value has been found */
    int zflag = 0;		/* indicates if a z value has been found */
    int arr_size = 0;
    /* variables to create arcs */
    double bulge = 0.0;		/* for arc curves */
    double prev_bulge = 0.0;	/* for arc curves */
    /* variables for polyface mesh */
    int vertex_flag = 0;
    int vertex_idx = 0;
    int polyface_mesh_started = 0;
    int mesh_pnts = 0;
    double *mesh_xpnts = NULL;
    double *mesh_ypnts = NULL;
    double *mesh_zpnts = NULL;

    strcpy(layer, UNIDENTIFIED_LAYER);

    /* read in lines and processes information until a 0 is read in */
    while ((code = dxf_get_code(dxf)) != 0) {
	if (code == -2)
	    return -1;

	switch (code) {
	case 66:		/* vertices follow flag */
	    vert_flag = atoi(dxf_buf);
	    if (vert_flag != 1)	/* flag must always be 1 */
		if (warn_flag66) {
		    G_warning(_("vertices following flag missing"));
		    warn_flag66 = 0;
		}
	    break;
	case 70:		/* polyline flag */
	    polyline_flag = atoi(dxf_buf);

	    /*******************************************************************
                  Bit        Meaning
                    1        This is a closed Polyline (or a polygon
                             mesh closed in the M direction)
                    2        Curve-fit vertices have been added
                    4        Spline-fit vertices have been added
                    8        This is a 3D Polyline
                   16        This is a 3D polygon mesh.  Group 75 indi-
                             cates the smooth surface type, as follows:

                               0 = no smooth surface fitted
                               5 = quadratic B-spline surface
                               6 = cubic B-spline surface
                               8 = Bezier surface

                   32        The polygon mesh is closed in the N direc-
                             tion
                   64        The polyline is a polyface mesh
	     ******************************************************************/
	    /* NOTE: code only exists for flag = 1 (closed polyline) or 0 */
	    G_debug(1, "polyline_flag: %d", polyline_flag);
	    if (polyline_flag & 8 || polyline_flag & 16 || polyline_flag & 17 ||
		polyline_flag & 32)
		if (warn_flag70) {
		    G_warning(_("3-d data in dxf file. Polyline_flag: %d"),
			      polyline_flag);
		    warn_flag70 = 0;
		}
	    break;
	}
    }

    zpnts[0] = 0.0;
    /* loop until SEQEND in the dxf file */
    while (strcmp(dxf_buf, "SEQEND") != 0) {
	if (feof(dxf->fp))	/* EOF */
	    return -1;

	if (strcmp(dxf_buf, "VERTEX") == 0) {
	    vertex_idx++;
	    xflag = 0;
	    yflag = 0;
	    zflag = 0;
	    while ((code = dxf_get_code(dxf)) != 0) {
		if (code == -2)	/* EOF */
		    return -1;

		switch (code) {
		case 8:	/* layer name */
		    /* if no layer previously assigned */
		    if (!layer_flag && *dxf_buf) {
			if (flag_list) {
			    if (!is_layer_in_list(dxf_buf)) {
				add_layer_to_list(dxf_buf);
				fprintf(stdout, _("Layer %d: %s\n"), num_layers,
					dxf_buf);
			    }
			    return 0;
			}
			/* skip if layers != NULL && (
			 * (flag_invert == 0 && is_layer_in_list == 0) ||
			 * (flag_invert == 1 && is_layer_in_list == 1)
			 * )
			 */
			if (layers && flag_invert == is_layer_in_list(dxf_buf))
			    return 0;
			strcpy(layer, dxf_buf);
			layer_flag = 1;
		    }
		    break;
		case 10:	/* x coordinate */
		    xpnts[arr_size] = atof(dxf_buf);
		    xflag = 1;
		    break;
		case 20:	/* y coordinate */
		    ypnts[arr_size] = atof(dxf_buf);
		    yflag = 1;
		    break;
		case 30:	/* Z coordinate */
		    zpnts[arr_size] = atof(dxf_buf);
		    zflag = 1;
		    break;
		case 40:	/* starting width */
		case 41:	/* ending width */
		    break;
		case 42:	/* bulge */
		    bulge = atof(dxf_buf);
		    break;
		case 50:	/* curve fit tangent direction */
		    break;
		case 70:	/* vertex flag */
		    vertex_flag = atoi(dxf_buf);

	    /*******************************************************************
                 Bit         Meaning
                   1         Extra vertex created by curve fitting
                   2         Curve fit tangent defined for this vertex.
                             A curve fit tangent direction of 0 may be
                             omitted from the DXF output, but is signif-
                             icant if this bit is set.
                   4         Unused (never set in DXF files)
                   8         Spline vertex created by spline fitting
                  16         Spline frame control point
                  32         3D Polyline vertex
                  64         3D polygon mesh vertex
                 128         Polyface mesh vertex
	     ******************************************************************/
		    if (vertex_flag == 16) {
			/* spline frame control point: don't draw it! */
			xflag = 0;
			yflag = 0;
			zflag = 0;
		    }
		    break;
		    /* NOTE: there are more cases possible */
		case 71:
		case 72:
		case 73:
		case 74:
		    if (!((vertex_flag & 128) && !(vertex_flag & 64)))
			break;
		    if (!polyface_mesh_started) {
			/* save vertex coordinates to mesh_?pnts */
			mesh_xpnts =
			    (double *)G_malloc(arr_size * sizeof(double));
			mesh_ypnts =
			    (double *)G_malloc(arr_size * sizeof(double));
			mesh_zpnts =
			    (double *)G_malloc(arr_size * sizeof(double));
			memcpy(mesh_xpnts, xpnts, arr_size * sizeof(double));
			memcpy(mesh_ypnts, ypnts, arr_size * sizeof(double));
			memcpy(mesh_zpnts, zpnts, arr_size * sizeof(double));

			/*
			 * if (flag_frame)
			 * write_pnts(Map, layer, polyline_flag, zflag,
			 * arr_size);
			 */
			polyface_mesh_started = 1;
			arr_size = 0;
			mesh_pnts = 0;
		    }
		    vertex_idx = atoi(dxf_buf);
		    if (vertex_idx > 0) {
			xpnts[arr_size] = mesh_xpnts[vertex_idx - 1];
			ypnts[arr_size] = mesh_ypnts[vertex_idx - 1];
			zpnts[arr_size] = mesh_zpnts[vertex_idx - 1];
			arr_size++;
		    }
		    if (++mesh_pnts == 4 || vertex_idx == 0) {
			/* check if there are any vertices when vertex_idx is 0
			 */
			if (arr_size > 0) {
			    /* close a mesh */
			    xpnts[arr_size] = xpnts[0];
			    ypnts[arr_size] = ypnts[0];
			    zpnts[arr_size] = zpnts[0];
			    arr_size++;
			    if (flag_frame) {
				set_entity("POLYFACE FRAME");
				write_line(Map, layer, arr_size);
			    }
			    else {
				set_entity("POLYFACE");
				write_face(Map, layer, arr_size);
			    }
			    arr_size = 0;
			}
			mesh_pnts = 0;
		    }
		    break;
		}
	    }
	}

	if (polyface_mesh_started) {
	    /* discard a premature mesh */
	    arr_size = 0;
	    mesh_pnts = 0;
	    continue;
	}

	if (xflag && yflag) {
	    arr_size = make_arc_from_polyline(arr_size, bulge, prev_bulge);
	    prev_bulge = bulge;
	    bulge = 0.0;
	}			/* processing polyline vertex */
    }				/* vertex loop */

    if (polyface_mesh_started) {
	G_free(mesh_xpnts);
	G_free(mesh_ypnts);
	G_free(mesh_zpnts);
    }
    else
	write_pnts(Map, layer, polyline_flag, zflag, arr_size);

    return 0;
}

static void write_pnts(struct Map_info *Map, char *layer, int polyline_flag,
		       int zflag, int arr_size)
{
    /* done reading vertices */
    if (polyline_flag & 1) {	/* only dealing with polyline_flag = 1 */
	/* check to make sure vertex points describe a closed polyline */
	if (xpnts[0] != xpnts[arr_size - 1] || ypnts[0] != ypnts[arr_size - 1]) {
	    /* add on the vertex point to complete closed polyline */
	    xpnts[arr_size] = xpnts[0];
	    ypnts[arr_size] = ypnts[0];
	    zpnts[arr_size] = zpnts[0];

	    /* arr_size incremented to be consistent with polyline_flag != 1 */
	    if (arr_size >= ARR_MAX - 1) {
		ARR_MAX += ARR_INCR;
		xpnts = (double *)G_realloc(xpnts, ARR_MAX * sizeof(double));
		ypnts = (double *)G_realloc(ypnts, ARR_MAX * sizeof(double));
		zpnts = (double *)G_realloc(zpnts, ARR_MAX * sizeof(double));
	    }
	    arr_size++;
	}
    }

    if (!zflag) {
	int i;

	for (i = 0; i < arr_size; i++)
	    zpnts[i] = 0.0;
    }

    write_line(Map, layer, arr_size);

    return;
}
