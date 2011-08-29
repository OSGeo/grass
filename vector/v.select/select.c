#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/glocale.h>

#include "proto.h"

int select_lines(struct Map_info *aIn, int atype, int afield,
		 struct Map_info *bIn, int btype, int bfield,
		 int cat_flag, int operator, const char *relate,
		 int *ALines)
{
    int i;
    int nalines, aline, nskipped, ltype;
    
    struct line_pnts *APoints, *BPoints;
    struct ilist *BoundList, *LList;
    struct boxlist *List, *TmpList;

#ifdef HAVE_GEOS
    initGEOS(G_message, G_fatal_error);
    GEOSGeometry *AGeom = NULL;
#else
    void *AGeom = NULL;
#endif

    nskipped = 0;
    APoints = Vect_new_line_struct();
    BPoints = Vect_new_line_struct();
    List = Vect_new_boxlist(1);
    TmpList = Vect_new_boxlist(1);
    BoundList = Vect_new_list();
    LList = Vect_new_list();

    nalines = Vect_get_num_lines(aIn);
    
    /* Lines in A. Go through all lines and mark those that meets condition */
    if (atype & (GV_POINTS | GV_LINES)) {
	G_message(_("Processing features..."));
	
	for (aline = 1; aline <= nalines; aline++) {
	    struct bound_box abox;

	    G_debug(3, "aline = %d", aline);
	    G_percent(aline, nalines, 2);	/* must be before any continue */

	    /* Check category */
	    if (!cat_flag && Vect_get_line_cat(aIn, aline, afield) < 0) {
		nskipped++;
		continue;
	    }

	    /* Read line and check type */
	    if (operator != OP_OVERLAP) {
#ifdef HAVE_GEOS
		AGeom = Vect_read_line_geos(aIn, aline, &ltype);
#endif
		if (!(ltype & (GV_POINT | GV_LINE)))
		    continue;

		if (!AGeom)
		    G_fatal_error(_("Unable to read line id %d from vector map <%s>"),
				  aline, Vect_get_full_name(aIn));
	    }
	    else {
		ltype = Vect_read_line(aIn, APoints, NULL, aline);
	    }
	    
	    if (!(ltype & atype))
		continue;
	    
	    Vect_get_line_box(aIn, aline, &abox);

	    /* Check if this line overlaps any feature in B */
	    /* x Lines in B */
	    if (btype & (GV_POINTS | GV_LINES)) {
		int i;
		int found = 0;
		
		/* Lines */
		Vect_select_lines_by_box(bIn, &abox, btype, List);
		for (i = 0; i < List->n_values; i++) {
		    int bline;
		    
		    bline = List->id[i];
		    G_debug(3, "  bline = %d", bline);
		    
		    /* Check category */
		    if (!cat_flag &&
			Vect_get_line_cat(bIn, bline, bfield) < 0) {
			nskipped++;
			continue;
		    }
		    
		    if (operator != OP_OVERLAP) {
#ifdef HAVE_GEOS
			if(line_relate_geos(bIn, AGeom,
					    bline, operator, relate)) {

			    found = 1;
			    break;
			}
#endif
		    }
		    else {
			Vect_read_line(bIn, BPoints, NULL, bline);

			if (Vect_line_check_intersection(APoints, BPoints, 0)) {
			    found = 1;
			    break;
			}
		    }
		}
		
		if (found) {
		    ALines[aline] = 1;
		    continue;	/* Go to next A line */
		}
	    }
	    
	    /* x Areas in B. */
	    if (btype & GV_AREA) {
		int i;
		
		Vect_select_areas_by_box(bIn, &abox, List);
		for (i = 0; i < List->n_values; i++) {
		    int barea;
		    
		    barea = List->id[i];
		    G_debug(3, "  barea = %d", barea);
		    
		    if (Vect_get_area_cat(bIn, barea, bfield) < 0) {
			nskipped++;
			continue;
		    }

		    if (operator != OP_OVERLAP) {
#ifdef HAVE_GEOS
			if(area_relate_geos(bIn, AGeom,
					    barea, operator, relate)) {
			    ALines[aline] = 1;
			    break;
			}
#endif
		    }
		    else {
			if (line_overlap_area(aIn, aline, bIn, barea, List->box[i])) {
			    ALines[aline] = 1;
			    break;
			}
		    }
		}
	    }
	    if (operator != OP_OVERLAP) {
#ifdef HAVE_GEOS
		GEOSGeom_destroy(AGeom);
#endif
		AGeom = NULL;
	    }
	}
    }
    
    /* Areas in A. */
    if (atype & GV_AREA) {
	int aarea, naareas;

	G_message(_("Processing areas..."));
	
	naareas = Vect_get_num_areas(aIn);

	for (aarea = 1; aarea <= naareas; aarea++) {
	    struct bound_box abox;

	    G_percent(aarea, naareas, 2);	/* must be before any continue */

	    if (Vect_get_area_cat(aIn, aarea, afield) < 0) {
		nskipped++;
		continue;
	    }
	
	    Vect_get_area_box(aIn, aarea, &abox);
	    abox.T = PORT_DOUBLE_MAX;
	    abox.B = -PORT_DOUBLE_MAX;

	    if (operator != OP_OVERLAP) {
#ifdef HAVE_GEOS
		AGeom = Vect_read_area_geos(aIn, aarea);
#endif
		if (!AGeom)
		    G_fatal_error(_("Unable to read area id %d from vector map <%s>"),
				  aline, Vect_get_full_name(aIn));
	    }

	    /* x Lines in B */
	    if (btype & (GV_POINTS | GV_LINES)) {
		Vect_select_lines_by_box(bIn, &abox, btype, List);

		for (i = 0; i < List->n_values; i++) {
		    int bline;

		    bline = List->id[i];

		    if (!cat_flag &&
			Vect_get_line_cat(bIn, bline, bfield) < 0) {
			nskipped++;
			continue;
		    }
		    
		    if (operator != OP_OVERLAP) {
#ifdef HAVE_GEOS
			if(line_relate_geos(bIn, AGeom,
					    bline, operator, relate)) {
			    add_aarea(aIn, aarea, ALines);
			    break;
			}
#endif
		    }
		    else {
			if (line_overlap_area(bIn, bline, aIn, aarea, abox)) {
			    add_aarea(aIn, aarea, ALines);
			    continue;
			}
		    }
		}
	    }

	    /* x Areas in B */
	    if (btype & GV_AREA) {
		int naisles;
		int found = 0;

		/* List of areas B */

		/* Make a list of features forming area A */
		Vect_reset_list(LList);

		Vect_get_area_boundaries(aIn, aarea, BoundList);
		for (i = 0; i < BoundList->n_values; i++) {
		    Vect_list_append(LList, abs(BoundList->value[i]));
		}

		naisles = Vect_get_area_num_isles(aIn, aarea);

		for (i = 0; i < naisles; i++) {
		    int j, aisle;

		    aisle = Vect_get_area_isle(aIn, aarea, i);

		    Vect_get_isle_boundaries(aIn, aisle, BoundList);
		    for (j = 0; j < BoundList->n_values; j++) {
			Vect_list_append(LList, BoundList->value[j]);
		    }
		}

		Vect_select_areas_by_box(bIn, &abox, TmpList);

		for (i = 0; i < LList->n_values; i++) {
		    int j, aline;

		    aline = abs(LList->value[i]);

		    for (j = 0; j < TmpList->n_values; j++) {
			int barea, bcentroid;

			barea = TmpList->id[j];
			G_debug(3, "  barea = %d", barea);

			if (Vect_get_area_cat(bIn, barea, bfield) < 0) {
			    nskipped++;
			    continue;
			}

			/* Check if any centroid of area B is in area A.
			 * This test is important in if area B is completely within area A */
			bcentroid = Vect_get_area_centroid(bIn, barea);
			Vect_read_line(bIn, BPoints, NULL, bcentroid);

			if (operator != OP_OVERLAP) {
#ifdef HAVE_GEOS
			    if(area_relate_geos(bIn, AGeom,
						barea, operator, relate)) {
				found = 1;
				break;
			    }
#endif
			}
			else {
			    if (Vect_point_in_area(BPoints->x[0], BPoints->y[0], aIn,
			                           aarea, abox)) {
				found = 1;
				break;
			    }
			    
			    /* Check intersectin of lines from List with area B */
			    if (line_overlap_area(aIn, aline,
						  bIn, barea, List->box[j])) {
				found = 1;
				break;
			    }
			}
		    }
		    if (found) {
			add_aarea(aIn, aarea, ALines);
			break;
		    }
		}
	    }
	    if (operator != OP_OVERLAP) {
#ifdef HAVE_GEOS
		GEOSGeom_destroy(AGeom);
#endif
		AGeom = NULL;
	    }
	}
    }

    Vect_destroy_line_struct(APoints);
    Vect_destroy_line_struct(BPoints);
    Vect_destroy_list(BoundList);
    Vect_destroy_list(LList);
    Vect_destroy_boxlist(List);
    Vect_destroy_boxlist(TmpList);

    return nskipped;
}
