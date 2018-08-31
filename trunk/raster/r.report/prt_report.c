#include <stdlib.h>
#include <string.h>
#include <grass/raster.h>
#include "global.h"

int print_report(int unit1, int unit2)
{
    int ns, nl, nx;
    char num[100];
    int len, new;
    CELL *cats, *prev;
    int first;
    int i;
    int divider_level;
    int after_header;
    int need_format;
    int with_stats;
    char *cp;
    int spacing;
    char dot;

    /* examine units, determine output format */
    for (i = unit1; i <= unit2; i++) {
	need_format = 1;
	unit[i].label[0] = "";
	unit[i].label[1] = "";

	switch (unit[i].type) {
	case CELL_COUNTS:
	    need_format = 0;
	    unit[i].len = 5;
	    unit[i].label[0] = " cell";
	    unit[i].label[1] = "count";
	    ns = 0;
	    sprintf(num, "%ld", count_sum(&ns, -1));
	    len = strlen(num);
	    if (len > unit[i].len)
		unit[i].len = len;
	    break;

	case PERCENT_COVER:
	    need_format = 0;
	    unit[i].dp = 2;
	    unit[i].len = 6;
	    unit[i].label[0] = "  %  ";
	    unit[i].label[1] = "cover";
	    unit[i].eformat = 0;
	    break;

	case SQ_METERS:
	    unit[i].label[0] = "square";
	    unit[i].label[1] = "meters";
	    unit[i].factor = 1.0;
	    break;

	case SQ_KILOMETERS:
	    unit[i].label[0] = "  square  ";
	    unit[i].label[1] = "kilometers";
	    unit[i].factor = 1.0e-6;
	    break;

	case ACRES:
	    unit[i].label[0] = "";
	    unit[i].label[1] = "acres";
	    unit[i].factor = 2.47105381467165e-4;	/* 640 acres in a sq mile */
	    break;

	case HECTARES:
	    unit[i].label[0] = "";
	    unit[i].label[1] = "hectares";
	    unit[i].factor = 1.0e-4;
	    break;

	case SQ_MILES:
	    unit[i].label[0] = "square";
	    unit[i].label[1] = " miles";
	    unit[i].factor = 3.86102158542446e-7;	/* 1 / ( (0.0254m/in * 12in/ft * 5280ft/mi)^2 ) */
	    break;

	default:
	    G_fatal_error("Unit %d not yet supported", unit[i].type);
	}
	if (need_format) {
	    unit[i].dp = 6;
	    unit[i].len = 10;
	    unit[i].eformat = 0;
	    ns = 0;
	    format_parms(area_sum(&ns, -1) * unit[i].factor,
			 &unit[i].len, &unit[i].dp, &(unit[i].eformat),
			 e_format);
	}
    }

    /* figure out how big the category numbers are when printed */
    for (nl = 0; nl < nlayers; nl++)
	layers[nl].nlen = 0;

    for (ns = 0; ns < nstats; ns++) {
	cats = Gstats[ns].cats;
	for (nl = 0; nl < nlayers; nl++) {
	    construct_val_str(nl, &cats[nl], num);
	    len = strlen(num);
	    if (len > layers[nl].nlen)
		layers[nl].nlen = len;
	}
    }

    /* compute maximum category description lengths */
    len = page_width - 2;
    for (i = unit1; i <= unit2; i++)
	len -= (unit[i].len + 1);
    for (nl = 0; nl < nlayers; nl++) {
	len -= (layers[nl].nlen + 1);
	layers[nl].clen = len;
    }

    /* print the report */

    header(unit1, unit2);
    after_header = 1;
    new = 1;

    divider_level = -1;
    for (ns = 0; ns < nstats; ns++) {
	int NS;

	cats = Gstats[ns].cats;

	/* determine the number of lines needed to print the cat labels 
	 * by pretending to print the labels and counting the number of
	 * print calls needed
	 */

	if (page_length > 0) {
	    i = 0;
	    for (nl = 0; nl < nlayers; nl++) {
		cp = construct_cat_label(nl, cats[nl]);

		while (cp) {
		    i++;
		    cp = print_label(cp, layers[nl].clen, 0, 0, ' ');
		}
	    }
	    if (nunits)
		i += nlayers;	/* divider lines */

	    /* if we don't have enough lines, go to a new page */
	    if (nlines <= i + 2) {
		trailer();
		header(unit1, unit2);
		after_header = 1;
		new = 2;
	    }
	}

	/* print the report */
	for (nl = 0; nl < nlayers; nl++) {
	    if (new || (prev[nl] != cats[nl])) {
		/* divider line between layers */

		if (nunits && divider_level != nl && !after_header) {
		    for (nx = 0; nx < nl; nx++)
			fprintf(stdout, "|%*s", layers[nx].nlen, "");
		    fprintf(stdout, "|");
		    for (nx = layers[nl].clen + layers[nx].nlen + 1; nx > 0;
			 nx--)
			fprintf(stdout, "-");
		    for (i = unit1; i <= unit2; i++) {
			fprintf(stdout, "|");
			for (nx = unit[i].len; nx > 0; nx--)
			    fprintf(stdout, "-");
		    }
		    fprintf(stdout, "|");
		    newline();
		}
		divider_level = nl;
		after_header = 0;

		first = 1;
		if (!new)
		    new = 1;

		cp = construct_cat_label(nl, cats[nl]);

		while (cp) {
		    for (nx = 0; nx < nl; nx++)
			fprintf(stdout, "|%*s", layers[nx].nlen, "");
		    if (first) {
			construct_val_str(nl, &cats[nl], num);
			fprintf(stdout, "|%*s|", layers[nl].nlen, num);
		    }
		    else
			fprintf(stdout, "|%*s|", layers[nl].nlen, "");

		    with_stats = nunits && first;
		    /*
		       if (new == 2 && nl != nlayers-1)
		       with_stats = 0;
		     */
		    if (with_stats)
			/* if it's not the lowest level of the table */
		    {
			if (nl != nlayers - 1) {
			    if (new != 2)
				NS = ns;	/* to memorise total */

			    /* if new is 2 then the total for this class should be reprinted on the
			       top of the page. So we need to remember ns of total in case we need to
			       print it again later */
			    spacing = 0;
			    dot = '_';
			}
			else {
			    spacing = 2;
			    dot = '.';
			}
		    }
		    else {
			spacing = 0;
			dot = ' ';
		    }
		    cp = print_label(cp, layers[nl].clen, 1, spacing, dot);
		    if (with_stats) {
			for (i = unit1; i <= unit2; i++)
			    if (nl != nlayers - 1)
				print_unit(i, NS, nl);
			    else
				print_unit(i, ns, nl);
		    }
		    else {
			for (i = unit1; i <= unit2; i++)
			    fprintf(stdout, "|%*s", unit[i].len, "");
		    }
		    fprintf(stdout, "|");
		    newline();
		    first = 0;
		}
	    }
	}
	new = 0;
	prev = cats;
    }
    /* overall totals */
    if (nunits) {
	divider("|");
	print_label("|TOTAL", layers[0].nlen + layers[0].clen + 2, 1, 0, ' ');
	for (i = unit1; i <= unit2; i++)
	    print_unit(i, 0, -1);
	/*
	   print_unit(i,-1,-1);
	 */
	fprintf(stdout, "|");
	newline();
    }
    trailer();

    return 0;
}

int construct_val_str(int nl, CELL * pval, char *str)
{
    char str1[50], str2[50];
    char *descr;
    DCELL dLow, dHigh;

    if (Rast_is_c_null_value(pval))
	sprintf(str, "%s", no_data_str);
    else if (!is_fp[nl] || as_int)
	sprintf(str, "%d", *pval);
    else {			/* find out which floating point range to print */

	if (cat_ranges)
	    descr = Rast_get_ith_d_cat(&layers[nl].labels, *pval,
					   &dLow, &dHigh);
	else {
	    dLow = (DMAX[nl] - DMIN[nl]) / nsteps *
		(double)(*pval - 1) + DMIN[nl];
	    dHigh = (DMAX[nl] - DMIN[nl]) / nsteps * (double)*pval + DMIN[nl];
	}
	sprintf(str1, "%10f", dLow);
	sprintf(str2, "%10f", dHigh);
	G_strip(str1);
	G_strip(str2);
	G_trim_decimal(str1);
	G_trim_decimal(str2);
	sprintf(str, "%s-%s", str1, str2);
    }

    return 0;
}

char *construct_cat_label(int nl, CELL cat)
{
    DCELL dLow, dHigh;
    CELL tmp = cat;
    static char str[500];

    if (!is_fp[nl] || as_int)
	return Rast_get_c_cat(&cat, &layers[nl].labels);
    else {			/* find or construct the label for
				   floating point range to print */
	if (Rast_is_c_null_value(&tmp))
	    return G_store("no data");
	if (cat_ranges)
	    return Rast_get_ith_d_cat(&layers[nl].labels, cat,
					  &dLow, &dHigh);
	else {
	    dLow = (DMAX[nl] - DMIN[nl]) / (double)nsteps *
		(double)(cat - 1) + DMIN[nl];
	    dHigh = (DMAX[nl] - DMIN[nl]) / (double)nsteps *
		(double)cat + DMIN[nl];
	    sprintf(str, "from %s to %s",
		    Rast_get_d_cat(&dLow, &layers[nl].labels),
		    Rast_get_d_cat(&dHigh, &layers[nl].labels));
	    return str;
	}
    }				/* fp label */

    return 0;
}
