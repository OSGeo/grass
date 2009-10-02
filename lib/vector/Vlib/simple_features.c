/*!
  \file vector/Vlib/simple_features.c
 
  \brief Vector library - OGC Simple Features Access

  Note: In progress. Currently on GV_POINT, GV_LINE are supported.
  
  Higher level functions for reading/writing/manipulating vectors.
  
  (C) 2009 by the GRASS Development Team
  
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
  
  \author Martin Landa <landa.martin gmail.com>
*/

#include <stdio.h>

#include <grass/vector.h>
#include <grass/glocale.h>

static int check_sftype(const struct line_pnts *, int, int, int);
static int get_sftype(const struct line_pnts *, int, int);
static void print_point(const struct line_pnts *, int, int, int, FILE *);

/*!
  \brief Get SF type of given vector feature

  List of supported feature types:
   - GV_POINT    -> SF_POINT
   - GV_LINE     -> SF_LINE / SF_LINESTRING / SF_LINEARRING
  
  \param Points  pointer to line_pnts structure
  \param type    feature type (GV_POINT, GV_LINE, ...)
  \param with_z  non-zero value for 3D data

  \return SF type identificator (see list of supported types)
  \return -1 on error
*/
int Vect_sfa_get_line_type(const struct line_pnts *Points, int type, int with_z)
{
    return get_sftype(Points, type, with_z);
}

/*!
  \brief Check SF type

  E.g. if <em>type</em> is GV_LINE with two or more segments and the
  start node is identical with the end node, and <em>sftype</em> is
  SF_LINEARRING, functions returns 1, otherwise 0.

  \param Points pointer to line_pnts structure
  \param type feature type (GV_POINT, GV_LINE, ...)
  \param sftype SF type to be checked (SF_POINT, SF_LINE, ...)
  \param with_z non-zero value for 3D data
  
  \return 1 if type is sftype
  \return 0 type differs from sftype
*/
int Vect_sfa_check_line_type(const struct line_pnts *Points, int type, int sftype, int with_z)
{
    return check_sftype(Points, type, sftype, with_z);
}

/*!
  \brief Print feature in Well-Known Text format

  \param Points    pointer to line_pnts structure
  \param type      feature type
  \param with_z    non-zero value for 3D data
  \param precision floating number precision 
  \param[out] file file where to write the output
*/
void Vect_sfa_write_line_wkt(const struct line_pnts *Points, int type, int with_z, int precision, FILE *file)
{
    int i, sftype;
    
    sftype = Vect_sfa_get_line_type(Points, type, with_z);
    
    switch(sftype) {
    case SF_POINT: {
	fprintf(file, "POINT(");
	print_point(Points, 0, with_z, precision, file);
	fprintf(file, ")\n");
	break;
    }
    case SF_LINESTRING: case SF_LINE: case SF_LINEARRING: {
	if (sftype == SF_LINESTRING)
	    fprintf(file, "LINESTRING(");
	else if (sftype ==  SF_LINE)
	    fprintf(file, "LINE(");
	else
	    fprintf(file, "LINEARRING(");
	for (i = 0; i < Points->n_points; i++) {
	    print_point(Points, i, with_z, precision, file);
	    if (i < Points->n_points - 1)
		fprintf(file, ", ");
	}
	fprintf(file, ")\n");
	break;
    }
    case SF_POLYGON: {
	/* write only outter/inner ring */
	fprintf(file, "(");
	for (i = 0; i < Points->n_points; i++) {
	    print_point(Points, i, with_z, precision, file);
	    if (i < Points->n_points - 1)
		fprintf(file, ", ");
	}
	fprintf(file, ")");
	break;
    }
    default: {
	G_warning(_("Unknown Simple Features type (%d)"), sftype);
	break;
    }
    }
    fflush(file);
}

int check_sftype(const struct line_pnts *points, int type, int sftype, int with_z)
{
    if (type == GV_POINT && sftype == SF_POINT) {
	return 1;
    }

    if (type == GV_LINE) {
	if (sftype == SF_LINESTRING) {
	    return 1;
	}
	if (sftype == SF_LINE && Vect_get_num_line_points(points) == 2) {
	    return 1;
	}
	if (sftype == SF_LINEARRING) {
	    int num = Vect_get_num_line_points(points);
	    if (points->x[0] == points->x[num-1] &&
		points->y[0] == points->y[num-1]) {
		if (!with_z) {
		    return 1;
		}
		else if (points->z[0] == points->z[num-1]) {
		    return 1;
		}
	    }
	}
    }

    if (type == GV_BOUNDARY) {
	int num = Vect_get_num_line_points(points);
	if (sftype == SF_POLYGON &&
	    points->x[0] == points->x[num-1] &&
	    points->y[0] == points->y[num-1]) {
	    return 1;
	}
    }

    return 0;
}

int get_sftype(const struct line_pnts *points, int type, int with_z)
{
    if (check_sftype(points, type, SF_POINT, with_z))
	return SF_POINT;

    if (check_sftype(points, type, SF_LINE, with_z))
	return SF_LINE;

    if (check_sftype(points, type, SF_LINEARRING, with_z))
	return SF_LINEARRING;

    if (check_sftype(points, type, SF_LINESTRING, with_z))
	return SF_LINESTRING;

    if (check_sftype(points, type, SF_POLYGON, with_z))
	return SF_POLYGON;

    return -1;
}

void print_point(const struct line_pnts *Points, int index, int with_z, int precision, FILE *file)
{
    fprintf(file, "%.*f %.*f", precision, Points->x[index], precision, Points->y[index]);
    if (with_z)
	fprintf(file, " %.*f", precision, Points->z[index]);
}
