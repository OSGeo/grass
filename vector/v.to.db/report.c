#include <grass/dbmi.h>
#include <grass/glocale.h>
#include "global.h"

int report(void)
{
    int i;
    char left[20], right[20];

    if (!options.print && !(options.option == O_COUNT ||
                            options.option == O_LENGTH ||
                            options.option == O_AREA)) {
        G_warning(_("No totals for selected option"));
        return 0;
    }
    
    switch (options.option) {
    case O_CAT:
	if (G_verbose() > G_verbose_min())
	    fprintf(stdout, "cat\n");
	for (i = 0; i < vstat.rcat; i++)
	    fprintf(stdout, "%d\n", Values[i].cat);
	break;

    case O_COUNT:
        if (options.print) {
            if (G_verbose() > G_verbose_min())
                fprintf(stdout, "cat%scount\n", options.fs);
            for (i = 0; i < vstat.rcat; i++)
                fprintf(stdout, "%d%s%d\n", Values[i].cat, options.fs, Values[i].count1);
        }
	if (options.total) {
	    int sum = 0;

	    for (i = 0; i < vstat.rcat; i++) {
		sum += Values[i].count1;
	    }
	    fprintf(stdout, "total count%s%d\n", options.fs, sum);
	}
	break;

    case O_AREA:
        if (options.print) {
            if (G_verbose() > G_verbose_min())
                fprintf(stdout, "cat%sarea\n", options.fs);
            for (i = 0; i < vstat.rcat; i++)
                fprintf(stdout, "%d%s%.15g\n", Values[i].cat, options.fs, Values[i].d1);
        }
	if (options.total) {
	    double sum = 0.0;

	    for (i = 0; i < vstat.rcat; i++) {
		sum += Values[i].d1;
	    }
	    fprintf(stdout, "total area%s%.15g\n", options.fs, sum);
	}
	break;

    case O_COMPACT:
	if (G_verbose() > G_verbose_min())
	    fprintf(stdout, "cat%scompact\n", options.fs);
	for (i = 0; i < vstat.rcat; i++)
	    fprintf(stdout, "%d%s%.15g\n", Values[i].cat, options.fs, Values[i].d1);
	break;

    case O_PERIMETER:
	if (G_verbose() > G_verbose_min())
	    fprintf(stdout, "cat%sperimeter\n", options.fs);
	for (i = 0; i < vstat.rcat; i++)
	    fprintf(stdout, "%d%s%.15g\n", Values[i].cat, options.fs, Values[i].d1);
	break;

    case O_LENGTH:
        if (options.print) {
            if (G_verbose() > G_verbose_min())
                fprintf(stdout, "cat%slength\n", options.fs);
            for (i = 0; i < vstat.rcat; i++)
                fprintf(stdout, "%d%s%.15g\n", Values[i].cat, options.fs, Values[i].d1);
        }
	if (options.total) {
	    double sum = 0.0;
            
	    for (i = 0; i < vstat.rcat; i++) {
		sum += Values[i].d1;
	    }
	    fprintf(stdout, "total length%s%.15g\n", options.fs, sum);
	}
	break;
    case O_SLOPE:
	if (G_verbose() > G_verbose_min())
	    fprintf(stdout, "cat%sslope\n", options.fs);
	for (i = 0; i < vstat.rcat; i++)
	    fprintf(stdout, "%d%s%.15g\n", Values[i].cat, options.fs, Values[i].d1);


	break;
    case O_SINUOUS:
	if (G_verbose() > G_verbose_min())
	    fprintf(stdout, "cat%ssinuous\n", options.fs);
	for (i = 0; i < vstat.rcat; i++)
	    fprintf(stdout, "%d%s%.15g\n", Values[i].cat, options.fs, Values[i].d1);
	break;
    case O_COOR:
    case O_START:
    case O_END:
	if (G_verbose() > G_verbose_min())
	    fprintf(stdout, "cat%sx%sy%sz\n", options.fs, options.fs, options.fs);
	for (i = 0; i < vstat.rcat; i++) {
	    if (Values[i].count1 == 1)
		fprintf(stdout, "%d%s%.15g%s%.15g%s%.15g\n", Values[i].cat, options.fs,
			Values[i].d1, options.fs, Values[i].d2, options.fs, Values[i].d3);
	}
	break;

    case O_SIDES:
	if (G_verbose() > G_verbose_min())
	    fprintf(stdout, "cat%sleft%sright\n", options.fs, options.fs);
	for (i = 0; i < vstat.rcat; i++) {
	    if (Values[i].count1 == 1) {
		if (Values[i].i1 >= 0)
		    sprintf(left, "%d", Values[i].i1);
		else
		    sprintf(left, "-1");	/* NULL, no area/cat */
	    }
	    else if (Values[i].count1 > 1) {
		sprintf(left, "-");
	    }
	    else {		/* Values[i].count1 == 0 */
		/* It can be OK if the category is assigned to an element
		   type which is not GV_BOUNDARY */
		/* -> TODO: print only if there is boundary with that cat */
		sprintf(left, "-");
	    }

	    if (Values[i].count2 == 1) {
		if (Values[i].i2 >= 0)
		    sprintf(right, "%d", Values[i].i2);
		else
		    sprintf(right, "-1");	/* NULL, no area/cat */
	    }
	    else if (Values[i].count2 > 1) {
		sprintf(right, "-");
	    }
	    else {		/* Values[i].count1 == 0 */
		sprintf(right, "-");
	    }

	    fprintf(stdout, "%d%s%s%s%s\n", Values[i].cat, options.fs, left, options.fs, right);
	}
	break;

    case O_QUERY:
	if (G_verbose() > G_verbose_min())
	    fprintf(stdout, "cat%squery\n", options.fs);
	for (i = 0; i < vstat.rcat; i++) {
	    if (Values[i].null) {
		fprintf(stdout, "%d|-\n", Values[i].cat);
	    }
	    else {
		switch (vstat.qtype) {
		case (DB_C_TYPE_INT):
		    fprintf(stdout, "%d%s%d\n", Values[i].cat, options.fs, Values[i].i1);
		    break;
		case (DB_C_TYPE_DOUBLE):
		    fprintf(stdout, "%d%s%15g\n", Values[i].cat, options.fs, Values[i].d1);
		    break;
		case (DB_C_TYPE_STRING):
		    fprintf(stdout, "%d%s%s\n", Values[i].cat, options.fs, Values[i].str1);
		    break;
		}
	    }
	}
	break;
    case O_AZIMUTH:
	if (G_verbose() > G_verbose_min())
	    fprintf(stdout, "cat%sazimuth\n", options.fs);
	for (i = 0; i < vstat.rcat; i++)
		fprintf(stdout, "%d%s%.15g\n", Values[i].cat, options.fs, Values[i].d1);
	break;
    }

    return 0;
}

int print_stat(void)
{
    if (vstat.rcat > 0) {
	int rcat_report;
	if(find_cat(-1, 0) != -1)
	    rcat_report = vstat.rcat - 1;
	else
	    rcat_report = vstat.rcat;
	
	G_message(_("%d categories read from vector map (layer %d)"),
		  rcat_report, options.field); /* don't report cat -1 */
    }
    if (vstat.select > 0)
	G_message(_("%d records selected from table (layer %d)"),
		  vstat.select, options.qfield);
    if (vstat.exist > 0)
	G_message(_("%d categories read from vector map exist in selection from table"),
		  vstat.exist);
    if (vstat.notexist > 0)
	G_message(_("%d categories read from vector map don't exist in selection from table"),
		  vstat.notexist);
    G_message(_("%d records updated/inserted (layer %d)"),
	      vstat.update, options.field);
    if (vstat.error > 0)
	G_message(_("%d update/insert errors (layer %d)"),
		  vstat.error, options.field);
    if (vstat.dupl > 0)
	G_message(_("%d categories with more points (coordinates not loaded)"),
		  vstat.dupl);

    return 0;
}
