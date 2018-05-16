#include <stdlib.h>

#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/glocale.h>

#include "proto.h"

int select_lines(struct Map_info *aIn, int atype, int afield,
                 struct Map_info *bIn, int btype, int bfield,
                 int cat_flag, int operator, const char *relate,
                 int *ALines, int *AAreas, int* nskipped)
{
    int i, ai;
    int nblines, bline, ltype;
    int nfound = 0;
    
    struct line_pnts *APoints, *BPoints;
    struct line_pnts *OPoints, **IPoints;
    int isle, nisles, isles_alloc;
    struct ilist *BoundList;
    struct boxlist *List;

#ifdef HAVE_GEOS
    initGEOS(G_message, G_fatal_error);
    GEOSGeometry *BGeom = NULL;
#else
    void *BGeom = NULL;
#endif

    nskipped[0] = nskipped[1] = 0;
    APoints = Vect_new_line_struct();
    BPoints = Vect_new_line_struct();
    OPoints = Vect_new_line_struct();
    isles_alloc = 10;
    IPoints = G_malloc(isles_alloc * sizeof(struct line_pnts *));
    for (i = 0; i < isles_alloc; i++)
	IPoints[i] = Vect_new_line_struct();
    nisles = 0;
    
    List = Vect_new_boxlist(1);
    BoundList = Vect_new_list();

    nblines = Vect_get_num_lines(bIn);
    
    /* Lines in B */
    if (btype & (GV_POINTS | GV_LINES)) {
	G_message(_("Processing features..."));
	
	G_percent(0, nblines, 2);
	for (bline = 1; bline <= nblines; bline++) {
	    struct bound_box bbox;

	    G_debug(3, "bline = %d", bline);
	    G_percent(bline, nblines, 1);	/* must be before any continue */

	    /* Check type */
	    ltype = Vect_get_line_type(bIn, bline);
	    if (!(ltype & btype))
		continue;

	    /* Check category */
	    if (!cat_flag && Vect_get_line_cat(bIn, bline, bfield) < 0) {
		nskipped[1]++;
		continue;
	    }

	    Vect_reset_line(BPoints);

	    Vect_get_line_box(bIn, bline, &bbox);

	    /* Check if this line overlaps any feature in A */
	    /* x Lines in A */
	    if (atype & (GV_POINTS | GV_LINES)) {
		
		/* Lines */
		Vect_select_lines_by_box(aIn, &bbox, atype, List);
		for (ai = 0; ai < List->n_values; ai++) {
		    int aline;
		    
		    aline = List->id[ai];
		    G_debug(3, "  aline = %d", aline);

		    if (ALines[aline] == 1)
			continue;

		    /* Check type */
		    ltype = Vect_get_line_type(aIn, aline);
		    if (!(ltype & atype))
			continue;

		    /* Check category */
		    if (!cat_flag &&
			Vect_get_line_cat(aIn, aline, afield) < 0) {
			nskipped[0]++;
			continue;
		    }
		    
		    if (operator != OP_OVERLAP) {
#ifdef HAVE_GEOS
			if (!BGeom)
			    BGeom = Vect_read_line_geos(bIn, bline, &ltype);
			if (!BGeom)
			    G_fatal_error(_("Unable to read line id %d from vector map <%s>"),
					  bline, Vect_get_full_name(bIn));

			if (line_relate_geos(aIn, BGeom, aline,
			                     operator, relate)) {
			    ALines[aline] = 1;
			    nfound += 1;
			}
#endif
		    }
		    else {
			if (BPoints->n_points == 0)
			    Vect_read_line(bIn, BPoints, NULL, bline);
			Vect_read_line(aIn, APoints, NULL, aline);

			if (Vect_line_check_intersection2(BPoints, APoints, 0)) {
			    ALines[aline] = 1;
			    nfound += 1;
			}
		    }
		}
	    }
	    
	    /* x Areas in A. */
	    if (atype & GV_AREA) {
		
		Vect_select_areas_by_box(aIn, &bbox, List);
		for (ai = 0; ai < List->n_values; ai++) {
		    int aarea;
		    
		    aarea = List->id[ai];
		    G_debug(3, "  aarea = %d", aarea);
		    
		    if (AAreas[aarea] == 1)
			continue;
		    
		    if (Vect_get_area_centroid(aIn, aarea) < 1)
			continue;

		    if (!cat_flag &&
		        Vect_get_area_cat(aIn, aarea, afield) < 0) {
			nskipped[0]++;
			continue;
		    }

		    if (operator != OP_OVERLAP) {
#ifdef HAVE_GEOS
			if (!BGeom)
			    BGeom = Vect_read_line_geos(bIn, bline, &ltype);
			if (!BGeom)
			    G_fatal_error(_("Unable to read line id %d from vector map <%s>"),
					  bline, Vect_get_full_name(bIn));

			if (area_relate_geos(aIn, BGeom, aarea, 
					     operator, relate)) {
			    add_aarea(aIn, aarea, ALines, AAreas);
			    nfound += 1;
			}
#endif
		    }
		    else {
			if (BPoints->n_points == 0)
			    Vect_read_line(bIn, BPoints, NULL, bline);
			Vect_get_area_points(aIn, aarea, OPoints);
			nisles = Vect_get_area_num_isles(aIn, aarea);
			if (nisles >= isles_alloc) {
			    IPoints = G_realloc(IPoints, (nisles + 10) * sizeof(struct line_pnts *));
			    for (i = isles_alloc; i < nisles + 10; i++)
				IPoints[i] = Vect_new_line_struct();
			    isles_alloc = nisles + 10;
			}
			for (i = 0; i < nisles; i++) {
			    isle = Vect_get_area_isle(aIn, aarea, i);
			    Vect_get_isle_points(aIn, isle, IPoints[i]);
			}

			if (line_overlap_area(BPoints, OPoints, IPoints, nisles)) {
			    add_aarea(aIn, aarea, ALines, AAreas);
			    nfound += 1;
			}
		    }
		}
	    }
#ifdef HAVE_GEOS
	    if (BGeom != NULL) {
		GEOSGeom_destroy(BGeom);
		BGeom = NULL;
	    }
#endif
	}
    }
    
    /* Areas in B. */
    if (btype & GV_AREA) {
	int barea, nbareas, bcentroid;

	G_message(_("Processing areas..."));
	
	nbareas = Vect_get_num_areas(bIn);

	G_percent(0, nbareas, 1);
	for (barea = 1; barea <= nbareas; barea++) {
	    struct bound_box bbox;

	    G_percent(barea, nbareas, 2);

	    if ((bcentroid = Vect_get_area_centroid(bIn, barea)) < 1)
		continue;

	    if (!cat_flag &&
	        Vect_get_area_cat(bIn, barea, bfield) < 0) {
		nskipped[1]++;
		continue;
	    }

	    Vect_reset_line(BPoints);

	    Vect_get_area_box(bIn, barea, &bbox);
	    bbox.T = PORT_DOUBLE_MAX;
	    bbox.B = -PORT_DOUBLE_MAX;

	    /* x Lines in A */
	    if (atype & (GV_POINTS | GV_LINES)) {
		Vect_select_lines_by_box(aIn, &bbox, atype, List);

		for (ai = 0; ai < List->n_values; ai++) {
		    int aline;

		    aline = List->id[ai];
		    if (ALines[aline] == 1)
			continue;

		    /* Check type */
		    ltype = Vect_get_line_type(aIn, aline);
		    if (!(ltype & atype))
			continue;

		    if (!cat_flag &&
			Vect_get_line_cat(aIn, aline, afield) < 0) {
			nskipped[0]++;
			continue;
		    }
		    
		    if (operator != OP_OVERLAP) {
#ifdef HAVE_GEOS
			if (!BGeom)
			    BGeom = Vect_read_area_geos(bIn, barea);
			if (!BGeom)
			    G_fatal_error(_("Unable to read area id %d from vector map <%s>"),
					  barea, Vect_get_full_name(bIn));
			if (line_relate_geos(aIn, BGeom, aline,
					     operator, relate)) {
			    ALines[aline] = 1;
			    nfound += 1;
			}
#endif
		    }
		    else {
			if (BPoints->n_points == 0) {
			    Vect_read_line(bIn, BPoints, NULL, bcentroid);
			    Vect_get_area_points(bIn, barea, OPoints);
			    nisles = Vect_get_area_num_isles(bIn, barea);
			    if (nisles >= isles_alloc) {
				IPoints = G_realloc(IPoints, (nisles + 10) * sizeof(struct line_pnts *));
				for (i = isles_alloc; i < nisles + 10; i++)
				    IPoints[i] = Vect_new_line_struct();
				isles_alloc = nisles + 10;
			    }
			    for (i = 0; i < nisles; i++) {
				isle = Vect_get_area_isle(bIn, barea, i);
				Vect_get_isle_points(bIn, isle, IPoints[i]);
			    }
			}

			Vect_read_line(aIn, APoints, NULL, aline);

			if (line_overlap_area(APoints, OPoints, IPoints, nisles)) {
			    ALines[aline] = 1;
			    nfound += 1;
			}
		    }
		}
	    }

	    /* x Areas in A */
	    if (atype & GV_AREA) {

		/* List of areas A */
		Vect_select_areas_by_box(aIn, &bbox, List);

		for (ai = 0; ai < List->n_values; ai++) {
		    int found = 0;
		    int aarea, acentroid;

		    aarea = List->id[ai];
		    G_debug(3, "  aarea = %d", aarea);
		    
		    if (AAreas[aarea] == 1)
			continue;

		    if ((acentroid = Vect_get_area_centroid(aIn, aarea)) < 1)
			continue;

		    if (!cat_flag &&
		        Vect_get_area_cat(aIn, aarea, afield) < 0) {
			nskipped[0]++;
			continue;
		    }

		    if (operator != OP_OVERLAP) {
#ifdef HAVE_GEOS
			if (!BGeom)
			    BGeom = Vect_read_area_geos(bIn, barea);
			if (!BGeom)
			    G_fatal_error(_("Unable to read area id %d from vector map <%s>"),
					  barea, Vect_get_full_name(bIn));
			if (area_relate_geos(aIn, BGeom, aarea,
					     operator, relate)) {
			    found = 1;
			}
#endif
		    }
		    else {
			if (BPoints->n_points == 0) {
			    Vect_read_line(bIn, BPoints, NULL, bcentroid);
			    Vect_get_area_points(bIn, barea, OPoints);
			    nisles = Vect_get_area_num_isles(bIn, barea);
			    if (nisles >= isles_alloc) {
				IPoints = G_realloc(IPoints, (nisles + 10) * sizeof(struct line_pnts *));
				for (i = isles_alloc; i < nisles + 10; i++)
				    IPoints[i] = Vect_new_line_struct();
				isles_alloc = nisles + 10;
			    }
			    for (i = 0; i < nisles; i++) {
				isle = Vect_get_area_isle(bIn, barea, i);
				Vect_get_isle_points(bIn, isle, IPoints[i]);
			    }
			}

			/* A inside B ? */
			Vect_read_line(aIn, APoints, NULL, acentroid);
			if (line_overlap_area(APoints, OPoints, IPoints, nisles)) {
			    found = 1;
			}

			/* B inside A ? */
			if (!found) {
			    struct bound_box abox;

			    Vect_get_area_box(aIn, aarea, &abox);
			    abox.T = PORT_DOUBLE_MAX;
			    abox.B = -PORT_DOUBLE_MAX;

			    if (Vect_point_in_area(BPoints->x[0], BPoints->y[0], aIn,
						   aarea, &abox)) {
				found = 1;
			    }
			}

			/* A overlaps B ? */
			if (!found) {
			    Vect_get_area_boundaries(aIn, aarea, BoundList);
			    for (i = 0; i < BoundList->n_values; i++) {
				Vect_read_line(aIn, APoints, NULL, abs(BoundList->value[i]));
				if (line_overlap_area(APoints, OPoints, IPoints, nisles)) {
				    found = 1;
				    break;
				}
			    }
			}

			if (!found) {
			    int j, naisles;

			    naisles = Vect_get_area_num_isles(aIn, aarea);
			    for (j = 0; j < naisles; j++) {

				isle = Vect_get_area_isle(aIn, aarea, j);

				Vect_get_isle_boundaries(aIn, isle, BoundList);
				for (i = 0; i < BoundList->n_values; i++) {
				    Vect_read_line(aIn, APoints, NULL, abs(BoundList->value[i]));
				    if (line_overlap_area(APoints, OPoints, IPoints, nisles)) {
					found = 1;
					break;
				    }
				}
				if (found)
				    break;
			    }
			}
		    }
		    if (found) {
			add_aarea(aIn, aarea, ALines, AAreas);
			nfound += 1;
		    }
		}
	    }
#ifdef HAVE_GEOS
	    if (BGeom != NULL) {
		GEOSGeom_destroy(BGeom);
		BGeom = NULL;
	    }
#endif
	}
    }

    Vect_destroy_line_struct(APoints);
    Vect_destroy_line_struct(BPoints);
    Vect_destroy_list(BoundList);
    Vect_destroy_boxlist(List);

    return nfound;
}
