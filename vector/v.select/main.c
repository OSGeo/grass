
/****************************************************************************
 *
 * MODULE:       v.select
 * AUTHOR(S):    Radim Blazek <radim.blazek gmail.com> (original contributor)
 *               Glynn Clements <glynn gclements.plus.com>
 *               Markus Neteler <neteler itc.it>
 *               Martin Landa <landa.martin gmail.com> (GEOS support)
 * PURPOSE:      Select features from one map by features in another map.
 * COPYRIGHT:    (C) 2003-2011 by the GRASS Development Team
 *
 *               This program is free software under the GNU General
 *               Public License (>=v2). Read the file COPYING that
 *               comes with GRASS for details.
 *
 *****************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/glocale.h>

#include "proto.h"

int main(int argc, char *argv[])
{
    int iopt;
    int operator;
    int nskipped, is_ogr;
    int itype[2], ifield[2];

    int *ALines; /* List of lines: 0 do not output, 1 - write to output */
    int **cats, *ncats, *fields, nfields;
    
    struct GModule *module;
    struct GParm parm;
    struct GFlag flag;
    struct Map_info In[2], Out;
    struct field_info *IFi;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("vector"));
    G_add_keyword(_("spatial query"));
    module->description =
	_("Selects features from vector map (A) by features from other vector map (B).");

    parse_options(&parm, &flag);
    
    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);
    
    if (parm.operator->answer[0] == 'e')
	operator = OP_EQUALS;
    else if (parm.operator->answer[0] == 'd') {
	/* operator = OP_DISJOINT; */
	operator = OP_INTERSECTS;
	flag.reverse->answer = YES;
    }
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
    else if (parm.operator->answer[0] == 'o') {
	if (strcmp(parm.operator->answer, "overlaps") == 0)
	    operator = OP_OVERLAPS;
	else
	    operator = OP_OVERLAP;
    }
    else if (parm.operator->answer[0] == 'r')
	operator = OP_RELATE;
    else
	G_fatal_error(_("Unknown operator '%s'"), parm.operator->answer);
    
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

    /* Alloc space for input lines array */
    ALines = (int *)G_calloc(Vect_get_num_lines(&(In[0])) + 1, sizeof(int));

    /* Read field info */
    IFi = Vect_get_field(&(In[0]), ifield[0]);

    /* Open output */
    Vect_open_new(&Out, parm.output->answer, Vect_is_3d(&(In[0])));
    Vect_set_map_name(&Out, _("Output from v.select"));
    Vect_set_person(&Out, G_whoami());
    Vect_copy_head_data(&(In[0]), &Out);
    Vect_hist_copy(&(In[0]), &Out);
    Vect_hist_command(&Out);
    
    /* Select features */
    nskipped = select_lines(&(In[0]), itype[0], ifield[0],
			    &(In[1]), itype[1], ifield[1],
			    flag.cat->answer ? 1 : 0, operator,
			    parm.relate->answer,
			    ALines);
    
    Vect_close(&(In[1]));

#ifdef HAVE_GEOS
    finishGEOS();
#endif

    is_ogr = Vect_maptype(&Out) == GV_FORMAT_OGR_DIRECT;

    nfields = Vect_cidx_get_num_fields(&(In[0]));
    cats = (int **)G_malloc(nfields * sizeof(int *));
    ncats = (int *)G_malloc(nfields * sizeof(int));
    fields = (int *)G_malloc(nfields * sizeof(int));

    /* Write lines */
    if (!flag.table->answer && is_ogr) {
	/* Copy attributes for OGR output */
	if (!IFi)
	    G_fatal_error(_("Database connection not defined for layer <%s>"),
			  parm.field[0]->answer);
	Vect_map_add_dblink(&Out, IFi->number, IFi->name, IFi->table, IFi->key,
			    IFi->database, IFi->driver);
    }
    write_lines(&(In[0]), IFi, ALines,
		&Out, flag.table->answer ? 1 : 0, flag.reverse->answer ? 1 : 0,
		nfields, fields, ncats, cats);

    /* Copy tables */
    if (!flag.table->answer && !is_ogr) {
	copy_tabs(&(In[0]), &Out,
		  nfields, fields, ncats, cats);
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
