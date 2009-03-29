
/****************************************************************************
 *
 * MODULE:       v.select - select features from one map by features in another map.
 * AUTHOR(S):    Radim Blazek <radim.blazek gmail.com> (original contributor)
 *               Glynn Clements <glynn gclements.plus.com>, Markus Neteler <neteler itc.it>
 *               Martin Landa <landa.martin gmail.com> (GEOS support)
 * PURPOSE:      
 * COPYRIGHT:    (C) 2003-2009 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <grass/config.h>
#include <grass/gis.h>
#include <grass/dbmi.h>
#include <grass/Vect.h>
#include <grass/glocale.h>

#include "proto.h"

int main(int argc, char *argv[])
{
    int i, iopt;
    int operator;
    int aline, nalines, nskipped;
    int ltype, itype[2], ifield[2];
    int **cats, *ncats, nfields, *fields;
    char *pre[2];
    struct GModule *module;
    struct {
	struct Option *input[2], *output, *type[2], *field[2],
	    *operator, *relate;
    } parm;
    struct {
	struct Flag *table, *reverse, *geos;
    } flag;
    struct Map_info In[2], Out;
    struct field_info *IFi, *OFi;
    struct line_pnts *APoints, *BPoints;
    struct line_cats *ACats, *BCats;
    int *ALines;		/* List of lines: 0 do not output, 1 - write to output */
    struct ilist *List, *TmpList, *BoundList;

    G_gisinit(argv[0]);

    pre[0] = "a";
    pre[1] = "b";

    module = G_define_module();
    module->keywords = _("vector, spatial query");
    module->description =
	_("Selects features from vector map (A) by features from other vector map (B).");

    parm.input[0] = G_define_standard_option(G_OPT_V_INPUT);
    parm.input[0]->description = _("Name of input vector map (A)");
    parm.input[0]->key = "ainput";
    
    parm.type[0] = G_define_standard_option(G_OPT_V_TYPE);
    parm.type[0]->label = _("Feature type (vector map A)");
    parm.type[0]->key = "atype";
    parm.type[0]->guisection = _("Selection");

    parm.field[0] = G_define_standard_option(G_OPT_V_FIELD);
    parm.field[0]->label = _("Layer number (vector map A)");
    parm.field[0]->key = "alayer";
    parm.field[0]->guisection = _("Selection");

    parm.input[1] = G_define_standard_option(G_OPT_V_INPUT);
    parm.input[1]->description = _("Name of input vector map (B)");
    parm.input[1]->key = "binput";

    parm.type[1] = G_define_standard_option(G_OPT_V_TYPE);
    parm.type[1]->label = _("Feature type (vector map B)");
    parm.type[1]->key = "btype";
    parm.type[1]->guisection = _("Selection");
    
    parm.field[1] = G_define_standard_option(G_OPT_V_FIELD);
    parm.field[1]->label = _("Layer number (vector map B)");
    parm.field[1]->key = "blayer";
    parm.field[1]->guisection = _("Selection");
    
    parm.output = G_define_standard_option(G_OPT_V_OUTPUT);

    parm.operator = G_define_option();
    parm.operator->key = "operator";
    parm.operator->type = TYPE_STRING;
    parm.operator->required = YES;
    parm.operator->multiple = NO;
    parm.operator->label =
	_("Operator defines required relation between features");
    parm.operator->description =
	_("A feature is written to output if the result of operation 'ainput operator binput' is true. "
	  "An input feature is considered to be true, if category of given layer is defined.");
#ifndef HAVE_GEOS
    parm.operator->options = "overlaps";
    parm.operator->answer = "overlaps";
    parm.operator->descriptions = _("overlaps;features partially or completely overlap");
#else
    parm.operator->options = "equals,disjoint,intersects,touches,crosses,within,contains,overlaps,relate";
    parm.operator->descriptions = _("equals;features are spatially equals (requires flag 'g');"
				    "disjoint;features do not spatially intersect (requires flag 'g');"
				    "intersects;features spatially intersect (requires flag 'g');"
				    "touches;features spatially touches (requires flag 'g');"
				    "crosses;features spatially crosses (requires flag 'g');"
				    "within;feature A is completely inside feature B (requires flag 'g');"
				    "contains;feature B is completely inside feature A (requires flag 'g');"
				    "overlaps;features spatilly overlap;"
				    "relate;feature A is spatially related to feature B "
				    "(requires 'relate' option and flag 'g');");
#endif

    parm.relate = G_define_option();
    parm.relate->key = "relate";
    parm.relate->type = TYPE_STRING;
    parm.relate->required = NO;
    parm.relate->multiple = NO;
    parm.relate->description = _("Intersection Matrix Pattern used for 'relate' operator");
    
    flag.table = G_define_flag();
    flag.table->key = 't';
    flag.table->description = _("Do not create attribute table");

    flag.reverse = G_define_flag();
    flag.reverse->key = 'r';
    flag.reverse->description = _("Reverse selection");
    flag.reverse->guisection = _("Selection");
#ifdef HAVE_GEOS
    flag.geos = G_define_flag();
    flag.geos->key = 'g';
    flag.geos->description = _("Use GEOS operators");
#else
    flag.geos = NULL;
#endif

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);
    
    if (parm.operator->answer[0] == 'e')
	operator = OP_EQUALS;
    else if (parm.operator->answer[0] == 'd')
	operator = OP_DISJOINT;
    else if (parm.operator->answer[0] == 'i')
	operator = OP_INTERSECTS;
    else if (parm.operator->answer[0] == 't')
	operator = OP_TOUCHES;
    else if (parm.operator->answer[0] == 'c' && parm.operator->answer[1] == 'r')
	operator = OP_CROSSES;
    else if (parm.operator->answer[0] == 'w')
	operator = OP_WITHIN;
    else if (parm.operator->answer[0] == 'c' && parm.operator->answer[1] == 'o')
	operator = OP_CONTAINS;
    else if (parm.operator->answer[0] == 'o')
	operator = OP_OVERLAPS;
    else if (parm.operator->answer[0] == 'r')
	operator = OP_RELATE;
    else
	G_fatal_error(_("Unknown operator"));
    
    if (operator != OP_OVERLAPS && flag.geos && !flag.geos->answer) {
	G_fatal_error(_("Enable GEOS operators (flag '%c') to use '%s'"),
		      flag.geos->key, parm.operator->answer);
    }

    if (operator == OP_RELATE && !parm.relate->answer) {
	G_fatal_error(_("Required parameter <%s> not set"),
		      parm.relate->key);
    }
    
    for (iopt = 0; iopt < 2; iopt++) {
	itype[iopt] = Vect_option_to_types(parm.type[iopt]);
	ifield[iopt] = atoi(parm.field[iopt]->answer);

	Vect_check_input_output_name(parm.input[iopt]->answer, parm.output->answer,
				     GV_FATAL_EXIT);

	Vect_set_open_level(2);
	Vect_open_old(&(In[iopt]), parm.input[iopt]->answer, "");
    }
    
    /* Read field info */
    IFi = Vect_get_field(&(In[0]), ifield[0]);

    APoints = Vect_new_line_struct();
    BPoints = Vect_new_line_struct();
    ACats = Vect_new_cats_struct();
    BCats = Vect_new_cats_struct();
    List = Vect_new_list();
    TmpList = Vect_new_list();
    BoundList = Vect_new_list();

    /* Open output */
    Vect_open_new(&Out, parm.output->answer, Vect_is_3d(&(In[0])));
    Vect_set_map_name(&Out, _("Output from v.select"));
    Vect_set_person(&Out, G_whoami());
    Vect_copy_head_data(&(In[0]), &Out);
    Vect_hist_copy(&(In[0]), &Out);
    Vect_hist_command(&Out);

    nskipped = 0;
    nalines = Vect_get_num_lines(&(In[0]));

#ifdef HAVE_GEOS
    initGEOS(G_message, G_fatal_error);
    GEOSGeometry *AGeom = NULL;
#else
    void *AGeom = NULL;
#endif

    /* Alloc space for input lines array */
    ALines = (int *)G_calloc(nalines + 1, sizeof(int));

    /* Lines in A. Go through all lines and mark those that meets condition */
    if (itype[0] & (GV_POINTS | GV_LINES)) {
	G_message(_("Processing features..."));
	
	for (aline = 1; aline <= nalines; aline++) {
	    BOUND_BOX abox;

	    G_debug(3, "aline = %d", aline);
	    G_percent(aline, nalines, 2);	/* must be before any continue */

	    /* Check category */
	    if (Vect_get_line_cat(&(In[0]), aline, ifield[0]) < 0) {
		nskipped++;
		continue;
	    }

	    /* Read line and check type */
	    ltype = Vect_read_line(&(In[0]), APoints, NULL, aline);
	    if (!(ltype & itype[0]))
		continue;

	    Vect_get_line_box(&(In[0]), aline, &abox);
	    abox.T = PORT_DOUBLE_MAX;
	    abox.B = -PORT_DOUBLE_MAX;

	    if (flag.geos && flag.geos->answer) {
		if (!(ltype & (GV_POINT | GV_LINE)))
		    continue;
#ifdef HAVE_GEOS
		AGeom = Vect_line_to_geos(&(In[0]), APoints, ltype);
#endif
		if (!AGeom)
		    G_fatal_error(_("Unable to read line id %d from vector map <%s>"),
				  aline, Vect_get_full_name(&(In[0])));
	    }

	    /* Check if this line overlaps any feature in B */
	    /* x Lines in B */
	    if (itype[1] & (GV_POINTS | GV_LINES)) {
		int i;
		int found = 0;
		
		/* Lines */
		Vect_select_lines_by_box(&(In[1]), &abox, itype[1], List);
		for (i = 0; i < List->n_values; i++) {
		    int bline;
		    
		    bline = List->value[i];
		    G_debug(3, "  bline = %d", bline);
		    
		    /* Check category */
		    if (!Vect_get_line_cat(&(In[1]), bline, ifield[1]) < 0) {
			nskipped++;
			continue;
		    }
		    
		    if (flag.geos && flag.geos->answer) {
#ifdef HAVE_GEOS
			if(line_relate_geos(&(In[1]), AGeom,
					    bline, operator, parm.relate->answer)) {

			    found = 1;
			    break;
			}
#endif
		    }
		    else {
			Vect_read_line(&(In[1]), BPoints, NULL, bline);

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
	    if (itype[1] & GV_AREA) {
		int i;
		
		Vect_select_areas_by_box(&(In[1]), &abox, List);
		for (i = 0; i < List->n_values; i++) {
		    int barea;
		    
		    barea = List->value[i];
		    G_debug(3, "  barea = %d", barea);
		    
		    if (Vect_get_area_cat(&(In[1]), barea, ifield[1]) < 0) {
			nskipped++;
			continue;
		    }

		    if (flag.geos && flag.geos->answer) {
#ifdef HAVE_GEOS
			if(area_relate_geos(&(In[1]), AGeom,
					    barea, operator, parm.relate->answer)) {
			    ALines[aline] = 1;
			    break;
			}
#endif
		    }
		    else {
			if (line_overlap_area(&(In[0]), aline, &(In[1]), barea)) {
			    ALines[aline] = 1;
			    break;
			}
		    }
		}
	    }
	    if (flag.geos && flag.geos->answer) {
#ifdef HAVE_GEOS
		GEOSGeom_destroy(AGeom);
#endif
		AGeom = NULL;
	    }
	}
    }
    
    /* Areas in A. */
    if (itype[0] & GV_AREA) {
	int aarea, naareas;

	G_message(_("Processing areas..."));
	
	naareas = Vect_get_num_areas(&(In[0]));

	for (aarea = 1; aarea <= naareas; aarea++) {
	    BOUND_BOX abox;

	    G_percent(aarea, naareas, 2);	/* must be before any continue */

	    if (Vect_get_area_cat(&(In[0]), aarea, ifield[0]) < 0) {
		nskipped++;
		continue;
	    }
	
	    Vect_get_area_box(&(In[0]), aarea, &abox);
	    abox.T = PORT_DOUBLE_MAX;
	    abox.B = -PORT_DOUBLE_MAX;

	    if (flag.geos && flag.geos->answer) { 
#ifdef HAVE_GEOS
		AGeom = Vect_read_area_geos(&(In[0]), aarea);
#endif
		if (!AGeom)
		    G_fatal_error(_("Unable to read area id %d from vector map <%s>"),
				  aline, Vect_get_full_name(&(In[0])));
	    }

	    /* x Lines in B */
	    if (itype[1] & (GV_POINTS | GV_LINES)) {
		Vect_select_lines_by_box(&(In[1]), &abox, itype[1], List);

		for (i = 0; i < List->n_values; i++) {
		    int bline;

		    bline = List->value[i];

		    if (Vect_get_line_cat(&(In[1]), bline, ifield[1]) < 0) {
			nskipped++;
			continue;
		    }
		    
		    if (flag.geos && flag.geos->answer) {
#ifdef HAVE_GEOS
			if(line_relate_geos(&(In[1]), AGeom,
					    bline, operator, parm.relate->answer)) {
			    add_aarea(&(In[0]), aarea, ALines);
			    break;
			}
#endif
		    }
		    else {
			if (line_overlap_area(&(In[1]), bline, &(In[0]), aarea)) {
			    add_aarea(&(In[0]), aarea, ALines);
			    continue;
			}
		    }
		}
	    }

	    /* x Areas in B */
	    if (itype[1] & GV_AREA) {
		int naisles;
		int found = 0;

		/* List of areas B */

		/* Make a list of features forming area A */
		Vect_reset_list(List);

		Vect_get_area_boundaries(&(In[0]), aarea, BoundList);
		for (i = 0; i < BoundList->n_values; i++) {
		    Vect_list_append(List, abs(BoundList->value[i]));
		}

		naisles = Vect_get_area_num_isles(&(In[0]), aarea);

		for (i = 0; i < naisles; i++) {
		    int j, aisle;

		    aisle = Vect_get_area_isle(&(In[0]), aarea, i);

		    Vect_get_isle_boundaries(&(In[0]), aisle, BoundList);
		    for (j = 0; j < BoundList->n_values; j++) {
			Vect_list_append(List, BoundList->value[j]);
		    }
		}

		Vect_select_areas_by_box(&(In[1]), &abox, TmpList);

		for (i = 0; i < List->n_values; i++) {
		    int j, aline;

		    aline = abs(List->value[i]);

		    for (j = 0; j < TmpList->n_values; j++) {
			int barea, bcentroid;

			barea = TmpList->value[j];
			G_debug(3, "  barea = %d", barea);

			if (Vect_get_area_cat(&(In[1]), barea, ifield[1]) < 0) {
			    nskipped++;
			    continue;
			}

			/* Check if any centroid of area B is in area A.
			 * This test is important in if area B is completely within area A */
			bcentroid = Vect_get_area_centroid(&(In[1]), barea);
			Vect_read_line(&(In[1]), BPoints, NULL, bcentroid);

			if (flag.geos && flag.geos->answer) {
#ifdef HAVE_GEOS
			    if(area_relate_geos(&(In[1]), AGeom,
						barea, operator, parm.relate->answer)) {
				found = 1;
				break;
			    }
#endif
			}
			else {
			    if (Vect_point_in_area(&(In[0]), aarea,
						   BPoints->x[0], BPoints->y[0])) {
				found = 1;
				break;
			    }
			    
			    /* Check intersectin of lines from List with area B */
			    if (line_overlap_area(&(In[0]), aline,
						  &(In[1]), barea)) {
				found = 1;
				break;
			    }
			}
		    }
		    if (found) {
			add_aarea(&(In[0]), aarea, ALines);
			break;
		    }
		}
	    }
	    if (flag.geos && flag.geos->answer) {
#ifdef HAVE_GEOS
		GEOSGeom_destroy(AGeom);
#endif
		AGeom = NULL;
	    }
	}
    }
    
    Vect_close(&(In[1]));

#ifdef HAVE_GEOS
    finishGEOS();
#endif

    /* Write lines */
    nfields = Vect_cidx_get_num_fields(&(In[0]));
    cats = (int **)G_malloc(nfields * sizeof(int *));
    ncats = (int *)G_malloc(nfields * sizeof(int));
    fields = (int *)G_malloc(nfields * sizeof(int));
    for (i = 0; i < nfields; i++) {
	ncats[i] = 0;
	cats[i] =
	    (int *)G_malloc(Vect_cidx_get_num_cats_by_index(&(In[0]), i) *
			    sizeof(int));
	fields[i] = Vect_cidx_get_field_number(&(In[0]), i);
    }

    G_message(_("Writing selected features..."));
    for (aline = 1; aline <= nalines; aline++) {
	int atype;

	G_debug(4, "aline = %d ALines[aline] = %d", aline, ALines[aline]);
	G_percent(aline, nalines, 2);
	
	if ((!flag.reverse->answer && !(ALines[aline])) ||
	    (flag.reverse->answer && ALines[aline]))
	    continue;

	atype = Vect_read_line(&(In[0]), APoints, ACats, aline);
	Vect_write_line(&Out, atype, APoints, ACats);

	if (!(flag.table->answer) && (IFi != NULL)) {
	    for (i = 0; i < ACats->n_cats; i++) {
		int f, j;

		for (j = 0; j < nfields; j++) {	/* find field */
		    if (fields[j] == ACats->field[i]) {
			f = j;
			break;
		    }
		}
		cats[f][ncats[f]] = ACats->cat[i];
		ncats[f]++;
	    }
	}
    }

    /* Copy tables */
    if (!(flag.table->answer)) {
	int ttype, ntabs = 0;

	G_message(_("Writing attributes..."));

	/* Number of output tabs */
	for (i = 0; i < Vect_get_num_dblinks(&(In[0])); i++) {
	    int f, j;

	    IFi = Vect_get_dblink(&(In[0]), i);

	    for (j = 0; j < nfields; j++) {	/* find field */
		if (fields[j] == IFi->number) {
		    f = j;
		    break;
		}
	    }
	    if (ncats[f] > 0)
		ntabs++;
	}

	if (ntabs > 1)
	    ttype = GV_MTABLE;
	else
	    ttype = GV_1TABLE;

	for (i = 0; i < nfields; i++) {
	    int ret;

	    if (fields[i] == 0)
		continue;

	    /* Make a list of categories */
	    IFi = Vect_get_field(&(In[0]), fields[i]);
	    if (!IFi) {		/* no table */
		G_warning(_("Layer %d - no table"), fields[i]);
		continue;
	    }

	    OFi =
		Vect_default_field_info(&Out, IFi->number, IFi->name, ttype);

	    ret =
		db_copy_table_by_ints(IFi->driver, IFi->database, IFi->table,
				      OFi->driver,
				      Vect_subst_var(OFi->database, &Out),
				      OFi->table, IFi->key, cats[i],
				      ncats[i]);

	    if (ret == DB_FAILED) {
		G_warning(_("Layer %d - unable to copy table"), fields[i]);
	    }
	    else {
		Vect_map_add_dblink(&Out, OFi->number, OFi->name, OFi->table,
				    IFi->key, OFi->database, OFi->driver);
	    }
	}
    }

    Vect_close(&(In[0]));

    Vect_build(&Out);
    Vect_close(&Out);

    if (nskipped > 0) {
      G_warning(_("%d features without category skipped"), nskipped);
    }

    G_done_msg(_("%d features written to output."), Vect_get_num_lines(&Out));

    exit(EXIT_SUCCESS);
}
