
 /***************************************************************************
 *
 * MODULE:     v.out.vtk  
 * AUTHOR(S):  Soeren Gebbert
 *
 * PURPOSE:    v.out.vtk: writes ASCII VTK file
 *             this module is based on v.out.ascii
 * COPYRIGHT:  (C) 2000 by the GRASS Development Team
 *
 *             This program is free software under the GNU General Public
 *              License (>=v2). Read the file COPYING that comes with GRASS
 *              for details.
 *
 ****************************************************************************/

#include <stdlib.h>
#include <grass/vector.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include "writeVTK.h"
#include "local_proto.h"


/*Prototype */
/*Formated coordinates output */
static void write_point_coordinates(struct line_pnts *Points, int dp,
				    double scale, FILE * ascii);


/* ************************************************************************* */
/* This function writes the vtk points and coordinates ********************* */
/* ************************************************************************* */
int write_vtk_points(FILE * ascii, struct Map_info *Map, VTKInfo * info,
		     int *types, int typenum, int dp, double scale)
{
    int type, cur, i, k, centroid;
    int pointoffset = 0;
    int lineoffset = 0;
    int polygonoffset = 0;
    static struct line_pnts *Points;
    struct line_cats *Cats;

    Points = Vect_new_line_struct();	/* init line_pnts struct */
    Cats = Vect_new_cats_struct();

    G_message("Writing the coordinates");

    /*For every available vector type */
    for (k = 0; k < typenum; k++) {
	/*POINT KERNEL CENTROID */
	if (types[k] == GV_POINT || types[k] == GV_KERNEL ||
	    types[k] == GV_CENTROID) {

	    /*Get the number of the points to generate */
	    info->typeinfo[types[k]]->pointoffset = pointoffset;
	    info->typeinfo[types[k]]->numpoints =
		Vect_get_num_primitives(Map, types[k]);
	    pointoffset += info->typeinfo[types[k]]->numpoints;

	    info->typeinfo[types[k]]->numvertices =
		info->typeinfo[types[k]]->numpoints;
	    info->maxnumvertices += info->typeinfo[types[k]]->numpoints;

	    info->maxnumpoints += info->typeinfo[types[k]]->numpoints;
	    /*
	     * printf("Points Type %i Number %i offset %i\n", types[k],
	     * info->typeinfo[types[k]]->numpoints,
	     * info->typeinfo[types[k]]->pointoffset);
	     */
	}
    }

    for (k = 0; k < typenum; k++) {
	/*LINE BOUNDARY */
	if (types[k] == GV_LINE || types[k] == GV_BOUNDARY) {

	    info->typeinfo[types[k]]->pointoffset = pointoffset;
	    info->typeinfo[types[k]]->lineoffset = lineoffset;

	    /*count the number of line_nodes and lines */
	    Vect_rewind(Map);
	    while (1) {
		if (-1 == (type = Vect_read_next_line(Map, Points, Cats)))
		    break;
		if (type == -2)	/* EOF */
		    break;
		if (type == types[k]) {
		    info->typeinfo[types[k]]->numpoints += Points->n_points;
		    info->typeinfo[types[k]]->numlines++;
		}
	    }
	    pointoffset += info->typeinfo[types[k]]->numpoints;
	    lineoffset += info->typeinfo[types[k]]->lineoffset;

	    info->maxnumpoints += info->typeinfo[types[k]]->numpoints;
	    info->maxnumlinepoints += info->typeinfo[types[k]]->numpoints;
	    info->maxnumlines += info->typeinfo[types[k]]->numlines;
	    /*
	     * printf("Lines  Type %i Number %i offset %i\n", types[k],
	     * info->typeinfo[types[k]]->numlines,
	     * info->typeinfo[types[k]]->lineoffset);
	     */
	}

    }

    for (k = 0; k < typenum; k++) {
	/*FACE */
	if (types[k] == GV_FACE) {

	    info->typeinfo[types[k]]->pointoffset = pointoffset;
	    info->typeinfo[types[k]]->polygonoffset = polygonoffset;

	    /*count the number of line_nodes and lines */
	    Vect_rewind(Map);
	    while (1) {
		if (-1 == (type = Vect_read_next_line(Map, Points, Cats)))
		    break;
		if (type == -2)	/* EOF */
		    break;
		if (type == types[k]) {
		    info->typeinfo[types[k]]->numpoints += Points->n_points;
		    info->typeinfo[types[k]]->numpolygons++;
		}
	    }

	    pointoffset += info->typeinfo[types[k]]->numpoints;
	    polygonoffset += info->typeinfo[types[k]]->numpolygons;

	    info->maxnumpoints += info->typeinfo[types[k]]->numpoints;
	    info->maxnumpolygonpoints += info->typeinfo[types[k]]->numpoints;
	    info->maxnumpolygons += info->typeinfo[types[k]]->numpolygons;
	    /*
	     * printf("Polygons  Type %i Number %i offset %i\n", types[k],
	     * info->typeinfo[types[k]]->numpolygons,
	     * info->typeinfo[types[k]]->polygonoffset);
	     */
	}

    }

    for (k = 0; k < typenum; k++) {
	/*AREA */
	if (types[k] == GV_AREA) {

	    info->typeinfo[types[k]]->numpolygons = Vect_get_num_areas(Map);
	    info->typeinfo[types[k]]->pointoffset = pointoffset;
	    info->typeinfo[types[k]]->polygonoffset = polygonoffset;

	    /*Count the coordinate points */
	    Vect_rewind(Map);
	    for (i = 1; i <= info->typeinfo[types[k]]->numpolygons; i++) {
		centroid = Vect_get_area_centroid(Map, i);
		if (centroid > 0) {
		    Vect_read_line(Map, NULL, Cats, centroid);
		}
		Vect_get_area_points(Map, i, Points);
		info->typeinfo[types[k]]->numpoints += Points->n_points;
	    }

	    pointoffset += info->typeinfo[types[k]]->numpoints;
	    polygonoffset += info->typeinfo[types[k]]->numpolygons;

	    info->maxnumpoints += info->typeinfo[types[k]]->numpoints;
	    info->maxnumpolygonpoints += info->typeinfo[types[k]]->numpoints;
	    info->maxnumpolygons += info->typeinfo[types[k]]->numpolygons;
	    /*
	     * printf("Polygons  Type %i Number %i offset %i\n", types[k],
	     * info->typeinfo[types[k]]->numpolygons,
	     * info->typeinfo[types[k]]->polygonoffset);
	     */
	}
    }

    /*
     * printf("Maxnum points %i \n", info->maxnumpoints);
     * printf("Maxnum vertices %i \n", info->maxnumvertices);
     * printf("Maxnum lines %i \n", info->maxnumlines);
     * printf("Maxnum line points %i \n", info->maxnumlinepoints);
     * printf("Maxnum polygons %i \n", info->maxnumpolygons);
     * printf("Maxnum polygon points %i \n", info->maxnumpolygonpoints);
     */
    /*break if nothing to generate */
    if (info->maxnumpoints == 0)
	G_fatal_error(_("No coordinates to generate the output! Maybe an empty vector type chosen?"));


    /************************************************/
    /*Write the coordinates into the vtk ascii file */

    /************************************************/

    fprintf(ascii, "POINTS %i float\n", info->maxnumpoints);

    /*For every available vector type */
    for (k = 0; k < typenum; k++) {
	/*POINT KERNEL CENTROID */
	if (types[k] == GV_POINT || types[k] == GV_KERNEL ||
	    types[k] == GV_CENTROID) {
	    Vect_rewind(Map);

	    /*Write the coordinates */
	    cur = 0;
	    while (1) {
		if (cur <= info->typeinfo[types[k]]->numpoints)
		    G_percent(cur, info->typeinfo[types[k]]->numpoints, 2);
		if (-1 == (type = Vect_read_next_line(Map, Points, Cats)))
		    break;
		if (type == -2)	/* EOF */
		    break;
		if (type == types[k]) {
		    write_point_coordinates(Points, dp, scale, ascii);

		    if (Cats->n_cats == 0)
			info->typeinfo[types[k]]->generatedata = 0;	/*No data generation */
		}
		cur++;
	    }

	}
    }

    for (k = 0; k < typenum; k++) {
	/*LINE BOUNDARY */
	if (types[k] == GV_LINE || types[k] == GV_BOUNDARY) {
	    Vect_rewind(Map);
	    cur = 0;
	    while (1) {
		if (cur <= info->typeinfo[types[k]]->numlines)
		    G_percent(cur, info->typeinfo[types[k]]->numlines, 2);
		if (-1 == (type = Vect_read_next_line(Map, Points, Cats)))
		    break;
		if (type == -2)	/* EOF */
		    break;
		if (type == types[k]) {
		    write_point_coordinates(Points, dp, scale, ascii);
		}
		cur++;
	    }

	}
    }

    for (k = 0; k < typenum; k++) {
	/* FACE */
	if (types[k] == GV_FACE) {
	    Vect_rewind(Map);
	    cur = 0;
	    while (1) {
		if (cur <= info->typeinfo[types[k]]->numpolygons)
		    G_percent(cur, info->typeinfo[types[k]]->numpolygons, 2);
		if (-1 == (type = Vect_read_next_line(Map, Points, Cats)))
		    break;
		if (type == -2)	/* EOF */
		    break;
		if (type == types[k]) {
		    write_point_coordinates(Points, dp, scale, ascii);
		}
		cur++;
	    }

	}
    }

    for (k = 0; k < typenum; k++) {
	/* AREA */
	if (types[k] == GV_AREA) {
	    Vect_rewind(Map);
	    for (i = 1; i <= info->typeinfo[types[k]]->numpolygons; i++) {
		centroid = Vect_get_area_centroid(Map, i);
		if (centroid > 0) {
		    Vect_read_line(Map, NULL, Cats, centroid);
		}
		Vect_get_area_points(Map, i, Points);
		write_point_coordinates(Points, dp, scale, ascii);
	    }

	}
    }

    return 1;
}


/* ************************************************************************* */
/* This function writes the vtk cells ************************************** */
/* ************************************************************************* */
int write_vtk_cells(FILE * ascii, struct Map_info *Map, VTKInfo * info,
		    int *types, int typenum)
{
    int type, i, j, k, centroid;
    static struct line_pnts *Points;
    struct line_cats *Cats;

    /*The keywords may only be written once! */
    int vertkeyword = 1;
    int linekeyword = 1;
    int polykeyword = 1;

    G_message("Writing vtk cells");

    Points = Vect_new_line_struct();	/* init line_pnts struct */
    Cats = Vect_new_cats_struct();

    /*For every available vector type */
    for (k = 0; k < typenum; k++) {


	/*POINT KERNEL CENTROID */
	if (types[k] == GV_POINT || types[k] == GV_KERNEL ||
	    types[k] == GV_CENTROID) {
	    Vect_rewind(Map);

	    /*Write the vertices */
	    if (info->typeinfo[types[k]]->numpoints > 0) {
		if (vertkeyword) {
		    fprintf(ascii, "VERTICES %i %i\n", info->maxnumvertices,
			    info->maxnumvertices * 2);
		    vertkeyword = 0;
		}
		for (i = 0; i < info->typeinfo[types[k]]->numpoints; i++) {
		    fprintf(ascii, "1 %i\n",
			    i + info->typeinfo[types[k]]->pointoffset);
		}
		fprintf(ascii, "\n");
	    }
	}
    }
    for (k = 0; k < typenum; k++) {
	/*LINE BOUNDARY */
	if (types[k] == GV_LINE || types[k] == GV_BOUNDARY) {
	    Vect_rewind(Map);

	    if (info->maxnumlines > 0) {
		if (linekeyword) {
		    fprintf(ascii, "LINES %i %i\n", info->maxnumlines,
			    info->maxnumlinepoints + info->maxnumlines);
		    linekeyword = 0;
		}

		Vect_rewind(Map);
		i = 0;
		while (1) {

		    if (-1 == (type = Vect_read_next_line(Map, Points, Cats)))
			break;
		    if (type == -2)	/* EOF */
			break;
		    if (type == types[k]) {

			/*Check for data generation */
			if (Cats->n_cats == 0)
			    info->typeinfo[types[k]]->generatedata = 0;	/*No data generation */

			fprintf(ascii, "%i", Points->n_points);
			while (Points->n_points--) {
			    fprintf(ascii, " %i",
				    i +
				    info->typeinfo[types[k]]->pointoffset);
			    i++;
			}
			fprintf(ascii, "\n");
		    }
		}
	    }
	}
    }
    for (k = 0; k < typenum; k++) {
	/*LINE BOUNDARY FACE */
	if (types[k] == GV_FACE) {
	    Vect_rewind(Map);

	    if (info->maxnumpolygons > 0) {
		if (polykeyword) {
		    fprintf(ascii, "POLYGONS %i %i\n",
			    info->maxnumpolygons,
			    info->maxnumpolygonpoints + info->maxnumpolygons);
		    polykeyword = 0;
		}

		Vect_rewind(Map);
		i = 0;
		while (1) {

		    if (-1 == (type = Vect_read_next_line(Map, Points, Cats)))
			break;
		    if (type == -2)	/* EOF */
			break;
		    if (type == types[k]) {

			/*Check for data generation */
			if (Cats->n_cats == 0)
			    info->typeinfo[types[k]]->generatedata = 0;	/*No data generation */

			fprintf(ascii, "%i", Points->n_points);
			while (Points->n_points--) {
			    fprintf(ascii, " %i",
				    i +
				    info->typeinfo[types[k]]->pointoffset);
			    i++;
			}
			fprintf(ascii, "\n");
		    }
		}
	    }
	}
    }

    for (k = 0; k < typenum; k++) {
	/*AREA */
	if (types[k] == GV_AREA) {
	    Vect_rewind(Map);

	    if (info->maxnumpolygons > 0) {
		if (polykeyword) {
		    fprintf(ascii, "POLYGONS %i %i\n",
			    info->maxnumpolygons,
			    info->maxnumpolygonpoints + info->maxnumpolygons);
		    polykeyword = 0;
		}

		j = 0;
		for (i = 1; i <= info->typeinfo[types[k]]->numpolygons; i++) {
		    centroid = Vect_get_area_centroid(Map, i);
		    if (centroid > 0) {
			Vect_read_line(Map, NULL, Cats, centroid);
		    }
		    Vect_get_area_points(Map, i, Points);

		    /*Check for data generation */
		    if (Cats->n_cats == 0)
			info->typeinfo[types[k]]->generatedata = 0;	/*No data generation */

		    fprintf(ascii, "%i", Points->n_points);
		    while (Points->n_points--) {
			fprintf(ascii, " %i",
				j + info->typeinfo[types[k]]->pointoffset);
			j++;
		    }
		    fprintf(ascii, "\n");
		}

	    }
	}
    }

    return 1;
}

/* ************************************************************************* */
/* This function writes the categories as vtk cell data ******************** */
/* ************************************************************************* */
int write_vtk_cat_data(FILE * ascii, struct Map_info *Map, VTKInfo * info,
		       int layer, int *types, int typenum, int dp)
{
    int type, cat, i, k, centroid;
    static struct line_pnts *Points;
    struct line_cats *Cats;

    /*The keywords may only be written once! */
    int numcelldata =
	info->maxnumvertices + info->maxnumlines + info->maxnumpolygons;

    Points = Vect_new_line_struct();	/* init line_pnts struct */
    Cats = Vect_new_cats_struct();

    G_message("Writing category cell data");

    if (numcelldata > 0) {
	/*Write the pointdata */
	fprintf(ascii, "CELL_DATA %i\n", numcelldata);
	fprintf(ascii, "SCALARS cat_%s int 1\n", Map->name);
	fprintf(ascii, "LOOKUP_TABLE default\n");

	/*For every available vector type */
	for (k = 0; k < typenum; k++) {
	    /*POINT KERNEL CENTROID */
	    if (types[k] == GV_POINT || types[k] == GV_KERNEL ||
		types[k] == GV_CENTROID) {

		Vect_rewind(Map);

		while (1) {
		    if (-1 == (type = Vect_read_next_line(Map, Points, Cats)))
			break;
		    if (type == -2)	/* EOF */
			break;
		    if (type == types[k]) {
			Vect_cat_get(Cats, layer, &cat);
			fprintf(ascii, " %d", cat);
		    }
		}
	    }
	}

	for (k = 0; k < typenum; k++) {
	    /*LINE BOUNDARY */
	    if (types[k] == GV_LINE || types[k] == GV_BOUNDARY) {
		Vect_rewind(Map);
		while (1) {
		    if (-1 == (type = Vect_read_next_line(Map, Points, Cats)))
			break;
		    if (type == -2)	/* EOF */
			break;
		    if (type == types[k]) {
			Vect_cat_get(Cats, layer, &cat);
			fprintf(ascii, " %d", cat);
		    }
		}
	    }
	}

	for (k = 0; k < typenum; k++) {
	    /*FACE */
	    if (types[k] == GV_FACE) {
		Vect_rewind(Map);
		while (1) {
		    if (-1 == (type = Vect_read_next_line(Map, Points, Cats)))
			break;
		    if (type == -2)	/* EOF */
			break;
		    if (type == types[k]) {
			Vect_cat_get(Cats, layer, &cat);
			fprintf(ascii, " %d", cat);
		    }
		}
	    }
	}

	for (k = 0; k < typenum; k++) {
	    /*AREA */
	    if (types[k] == GV_AREA) {
		Vect_rewind(Map);
		for (i = 1; i <= info->typeinfo[types[k]]->numpolygons; i++) {
		    centroid = Vect_get_area_centroid(Map, i);
		    if (centroid > 0) {
			Vect_read_line(Map, NULL, Cats, centroid);
		    }
		    Vect_cat_get(Cats, layer, &cat);
		    fprintf(ascii, " %d", cat);
		}
	    }
	}
    }


    return 1;
}

int write_vtk_db_data(FILE * ascii, struct Map_info *Map, VTKInfo * info,
		      int layer, int *types, int typenum, int dp)
{
    G_message("Writing database cell/point data");

    return 1;
}

/* ************************************************************************* */
/* This function writes the point coordinates and the geometric feature **** */
/* ************************************************************************* */
int write_vtk(FILE * ascii, struct Map_info *Map, int layer, int *types,
	      int typenum, int dp, double scale)
{
    VTKInfo *info;
    VTKTypeInfo **typeinfo;
    int i;
    int infonum =
	GV_POINT + GV_KERNEL + GV_CENTROID + GV_LINE + GV_BOUNDARY + GV_FACE +
	GV_AREA;


    /*Initiate the typeinfo structure for every supported type */
    typeinfo = (VTKTypeInfo **) calloc(infonum, sizeof(VTKTypeInfo *));
    for (i = 0; i < infonum; i++) {
	typeinfo[i] = (VTKTypeInfo *) calloc(1, sizeof(VTKTypeInfo));
	typeinfo[i]->numpoints = 0;
	typeinfo[i]->pointoffset = 0;
	typeinfo[i]->numvertices = 0;
	typeinfo[i]->verticesoffset = 0;
	typeinfo[i]->numlines = 0;
	typeinfo[i]->lineoffset = 0;
	typeinfo[i]->numpolygons = 0;
	typeinfo[i]->polygonoffset = 0;
	typeinfo[i]->generatedata = 1;
    }

    /*Initiate the info structure */
    info = (VTKInfo *) calloc(infonum, sizeof(VTKInfo));
    info->maxnumpoints = 0;
    info->maxnumvertices = 0;
    info->maxnumlines = 0;
    info->maxnumlinepoints = 0;
    info->maxnumpolygons = 0;
    info->maxnumpolygonpoints = 0;
    info->typeinfo = typeinfo;

    /*1. write the points */
    write_vtk_points(ascii, Map, info, types, typenum, dp, scale);

    /*2. write the cells */
    write_vtk_cells(ascii, Map, info, types, typenum);

    /*3. write the cat data */
    write_vtk_cat_data(ascii, Map, info, layer, types, typenum, dp);

    /*4. write the DB data */
    /* not yet implemented
       write_vtk_db_data(ascii, Map, info, layer, types, typenum, dp);
     */

    /*Release the memory */
    for (i = 0; i < infonum; i++) {
	free(typeinfo[i]);
    }
    free(typeinfo);
    free(info);

    return 1;
}

/* ************************************************************************* */
/* This function writes the point coordinates ****************************** */
/* ************************************************************************* */
void write_point_coordinates(struct line_pnts *Points, int dp, double scale,
			     FILE * ascii)
{
    char *xstring = NULL, *ystring = NULL, *zstring = NULL;
    double *xptr, *yptr, *zptr;

    xptr = Points->x;
    yptr = Points->y;
    zptr = Points->z;

    while (Points->n_points--) {
	G_asprintf(&xstring, "%.*f", dp, *xptr++ - x_extent);
	G_trim_decimal(xstring);
	G_asprintf(&ystring, "%.*f", dp, *yptr++ - y_extent);
	G_trim_decimal(ystring);
	G_asprintf(&zstring, "%.*f", dp, scale * (*zptr++));
	G_trim_decimal(zstring);
	fprintf(ascii, "%s %s %s \n", xstring, ystring, zstring);
    }

    return;
}
