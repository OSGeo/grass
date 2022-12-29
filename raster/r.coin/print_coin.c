
/****************************************************************************
 *
 * MODULE:       r.coin
 *
 * AUTHOR(S):    Michael O'Shea - CERL
 *               Michael Shapiro - CERL
 *
 * PURPOSE:      Calculates the coincidence of two raster map layers.
 *
 * COPYRIGHT:    (C) 2006 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 ***************************************************************************/

#include "coin.h"
#include <grass/glocale.h>


int print_coin(int Conformat, int out_cols, int tofile)
{
    int Thisone;
    int num_panels, at_panel;
    int first_col, last_col;
    int width;

    long colcount, rowcount, t_rowcount;
    long colcount_no_0, rowcount_no_0;
    long count;
    double colarea, rowarea, t_rowarea;
    double colarea_no_0, rowarea_no_0;
    double area;

    int addflag;
    char topformat[133], midformat[133], namformat[133];
    char fillformat[133];
    const char *mapone;
    int col0, row0;

    if (tofile) {
	fprintf(stderr, _("Preparing report ..."));
	fflush(stderr);
	dumpfile = fopen(dumpname, "w");
    }
    else
	dumpfile = stdout;

    print_coin_hdr(Conformat);

    row0 = no_data2;
    col0 = no_data1;
    out_cols = (out_cols == 132) ? 7 : 3;

    if (Conformat == 'x')
	out_cols += 2;

    num_panels = ncat1 / out_cols;
    if (ncat1 % out_cols)
	num_panels++;

    width = out_cols * 12 + 32;
    sprintf(topformat, "+%%%d.%ds+", width, width);
    sprintf(midformat, "|%%%d.%ds|", width, width);
    if (Conformat != 'x')
	sprintf(namformat, "|        | %%-%ds|    Panel Row Total    |",
		width - 34);
    else
	sprintf(namformat, "|        | %%-%ds|", width - 34);

    for (at_panel = 0; at_panel < num_panels; at_panel++) {
	first_col = at_panel * out_cols;
	last_col = first_col + out_cols;
	if (last_col >= ncat1) {
	    last_col = ncat1;
	    width = (last_col - first_col) * 12 + 32;
	    if (Conformat == 'x')
		width -= 24;
	    sprintf(topformat, "+%%%d.%ds+", width, width);
	    sprintf(midformat, "|%%%d.%ds|", width, width);
	    if (Conformat != 'x')
		sprintf(namformat,
			"|        | %%-%ds|    Panel Row Total    |",
			width - 34);
	    else
		sprintf(namformat, "|        | %%-%ds|", width - 10);
	}

	/* Determine if room enough for Table Row Total at end of last panel    */
	addflag = 0;
	if (at_panel == (num_panels - 1) &&
	    (last_col - first_col) < (out_cols - 2)) {
	    addflag = 1;
	    sprintf(fillformat, "%%1.%ds%%s",
		    (out_cols - (last_col - first_col)) * 12 - 25);
	}
	if (Conformat == 'x')
	    addflag = 0;

    /*========================================================================*/
	/* panel line       */
	fprintf(dumpfile, "Panel #%d of %d\n", at_panel + 1, num_panels);
	fprintf(dumpfile, topformat, midline);
	if (addflag)
	    fprintf(dumpfile, fillformat, fill, "+-----------------------+");
	fprintf(dumpfile, "\n");

    /*========================================================================*/
	/* name line        */
	fprintf(dumpfile, namformat, map1name);
	if (addflag)
	    fprintf(dumpfile, fillformat, fill, "|    Table Row Total    |");
	fprintf(dumpfile, "\n");

    /*========================================================================*/
	/* cat line         */
	fprintf(dumpfile, "|   cat# |");
	for (Cndex = first_col; Cndex < last_col; Cndex++)
	    fprintf(dumpfile, " %9ld |", (long)catlist1[Cndex]);
	if (Conformat != 'x')
	    fprintf(dumpfile, "   w cat 0 | w/o cat 0 |");
	if (addflag)
	    fprintf(dumpfile, fillformat, fill, "|   w cat 0 | w/o cat 0 |");
	fprintf(dumpfile, "\n");

    /*========================================================================*/
	fprintf(dumpfile, midformat, midline);
	if (addflag)
	    fprintf(dumpfile, fillformat, fill, "|-----------------------|");
	fprintf(dumpfile, "\n");

    /*========================================================================*/
	/* body of table    */
	t_rowcount = 0;
	t_rowarea = 0;
	mapone = map2name;
	for (Rndex = 0; Rndex < ncat2; Rndex++) {
	    if (*(mapone) != '\0')
		fprintf(dumpfile, "|%c %5ld |", *(mapone)++,
			(long)catlist2[Rndex]);
	    else
		fprintf(dumpfile, "|  %5ld |", (long)catlist2[Rndex]);

	    /* column entries   */
	    rowcount = rowcount_no_0 = 0;
	    rowarea = rowarea_no_0 = 0;
	    for (Cndex = first_col; Cndex < last_col; Cndex++) {
		Thisone = (ncat1 * Rndex) + Cndex;
		print_entry(Conformat, table[Thisone].count,
			    table[Thisone].area);
		rowcount += table[Thisone].count;
		rowarea += table[Thisone].area;
		if (Cndex != col0) {
		    rowcount_no_0 += table[Thisone].count;
		    rowarea_no_0 += table[Thisone].area;
		}
	    }

	    /* Panel Row totals (including/excluding col 0)     */
	    if (Conformat != 'x') {
		print_entry(Conformat, rowcount, rowarea);
		print_entry(Conformat, rowcount_no_0, rowarea_no_0);
	    }

	    /* Table Row Total entries (if appropriate) */
	    if (addflag) {
		fprintf(dumpfile, fillformat, fill, "|");
		row_total(Rndex, 1, &rowcount, &rowarea);
		row_total(Rndex, 0, &rowcount_no_0, &rowarea_no_0);
		print_entry(Conformat, rowcount, rowarea);
		print_entry(Conformat, rowcount_no_0, rowarea_no_0);
		t_rowcount += rowcount;
		t_rowarea += rowarea;
	    }
	    fprintf(dumpfile, "\n");
	}
	if (Conformat != 'y') {

    /*========================================================================*/
	    fprintf(dumpfile, midformat, midline);
	    if (addflag)
		fprintf(dumpfile, fillformat, fill,
			"|-----------------------|");
	    fprintf(dumpfile, "\n");

    /*========================================================================*/
	    /* 'Total' line at bottom of panel  */
	    fprintf(dumpfile, "|Total   |");
	    for (Cndex = first_col; Cndex < last_col; Cndex++)
		fprintf(dumpfile, "           |");	/*Cndex); */
	    if (Conformat != 'x')
		fprintf(dumpfile, "           |           |");
	    if (addflag)
		fprintf(dumpfile, fillformat, fill,
			"|           |           |");
	    fprintf(dumpfile, "\n");

    /*========================================================================*/
	    /* column totals including row 0    */
	    fprintf(dumpfile, "|with 0  |");
	    colcount_no_0 = colcount = 0;
	    colarea_no_0 = colarea = 0;
	    for (Cndex = first_col; Cndex < last_col; Cndex++) {
		col_total(Cndex, 1, &count, &area);
		print_entry(Conformat, count, area);
		colcount += count;
		colarea += area;
		if (Cndex != col0) {
		    colcount_no_0 += count;
		    colarea_no_0 += area;
		}
	    }

	    /* Grand Totals of Panel Row totals (including/excluding col 0)     */
	    if (Conformat != 'x') {
		print_entry(Conformat, colcount, colarea);
		print_entry(Conformat, colcount_no_0, colarea_no_0);
	    }

	    /* Grand Totals of Table Row totals (including/excluding col 0)     */
	    if (addflag) {
		fprintf(dumpfile, fillformat, fill, "|");
		print_entry(Conformat, t_rowcount, t_rowarea);
		if (col0 >= 0) {
		    col_total(col0, 1, &count, &area);
		    print_entry(Conformat, t_rowcount - count,
				t_rowarea - area);
		}
		else
		    print_entry(Conformat, t_rowcount, t_rowarea);
	    }
	    fprintf(dumpfile, "\n");

    /*========================================================================*/
	    fprintf(dumpfile, midformat, midline);
	    if (addflag)
		fprintf(dumpfile, fillformat, fill,
			"|-----------------------|");
	    fprintf(dumpfile, "\n");

    /*========================================================================*/
	    /* column totals excluding row 0    */
	    fprintf(dumpfile, "|w/o 0   |");
	    colcount_no_0 = colcount = 0;
	    colarea_no_0 = colarea = 0;
	    for (Cndex = first_col; Cndex < last_col; Cndex++) {
		col_total(Cndex, 0, &count, &area);
		print_entry(Conformat, count, area);
		colcount += count;
		colarea += area;
		if (Cndex != col0) {
		    colcount_no_0 += count;
		    colarea_no_0 += area;
		}
	    }

	    /* Grand Totals of Panel Row totals (including/excluding col 0)     */
	    if (Conformat != 'x') {
		print_entry(Conformat, colcount, colarea);
		print_entry(Conformat, colcount_no_0, colarea_no_0);
	    }

	    /* Grand Totals of Table Row totals (including/excluding col 0)     */
	    if (addflag) {
		fprintf(dumpfile, fillformat, fill, "|");
		rowcount_no_0 = rowcount = t_rowcount;
		rowarea_no_0 = rowarea = t_rowarea;
		if (row0 >= 0) {
		    row_total(row0, 1, &count, &area);
		    rowcount -= count;
		    rowarea -= area;
		    row_total(row0, 0, &count, &area);
		    rowcount_no_0 -= count;
		    rowarea_no_0 -= area;
		}
		print_entry(Conformat, rowcount, rowarea);
		print_entry(Conformat, rowcount_no_0, rowarea_no_0);
	    }
	    fprintf(dumpfile, "\n");

    /*========================================================================*/
	}
	fprintf(dumpfile, topformat, midline);
	if (addflag)
	    fprintf(dumpfile, fillformat, fill, "+-----------------------+");
	fprintf(dumpfile, "\n\n");
    }

/*========================================================================*/
    /* Table Row Total Panel (if no room at end of last panel)      */
    if (!addflag && Conformat != 'x') {
	/* header       */
	fprintf(dumpfile, "+--------------------------------+\n");
	fprintf(dumpfile, "|        |    Table Row Total    |\n");
	fprintf(dumpfile, "|   cat# |   w cat 0 | w/o cat 0 |\n");
	fprintf(dumpfile, "|--------------------------------|\n");

	/* body         */
	mapone = map2name;
	t_rowcount = 0;
	for (Rndex = 0; Rndex < ncat2; Rndex++) {
	    if (*(mapone) != '\0')
		fprintf(dumpfile, "|%c %5ld |", *(mapone)++,
			(long)catlist2[Rndex]);
	    else
		fprintf(dumpfile, "|  %5ld |", (long)catlist2[Rndex]);

	    row_total(Rndex, 1, &count, &area);
	    print_entry(Conformat, count, area);
	    t_rowcount += count;
	    t_rowarea += area;

	    row_total(Rndex, 0, &count, &area);
	    print_entry(Conformat, count, area);
	    fprintf(dumpfile, "\n");
	}

	/* Grand Totals */
	if (Conformat != 'y') {
	    fprintf(dumpfile, "|--------------------------------|\n");
	    fprintf(dumpfile, "|Total   |           |           |\n");

	    fprintf(dumpfile, "|with 0  |");
	    print_entry(Conformat, t_rowcount, t_rowarea);
	    rowcount = t_rowcount;
	    rowarea = t_rowarea;
	    if (col0 >= 0) {
		col_total(col0, 1, &count, &area);
		rowcount -= count;
		rowarea -= area;
	    }
	    print_entry(Conformat, rowcount, rowarea);
	    fprintf(dumpfile, "\n");

	    fprintf(dumpfile, "|--------------------------------|\n");

	    fprintf(dumpfile, "|w/o 0   |");
	    rowcount = t_rowcount;
	    rowarea = t_rowarea;
	    if (row0 >= 0) {
		row_total(row0, 1, &count, &area);
		rowcount -= count;
		rowarea -= area;
	    }
	    rowcount_no_0 = rowcount;
	    rowarea_no_0 = rowarea;
	    if (col0 >= 0) {
		col_total(col0, 0, &count, &area);
		rowcount_no_0 -= count;
		rowarea_no_0 -= area;
	    }
	    print_entry(Conformat, rowcount, rowarea);
	    print_entry(Conformat, rowcount_no_0, rowarea_no_0);

	    fprintf(dumpfile, "\n");
	}
	fprintf(dumpfile, "+--------------------------------+\n\n");
    }

    if (tofile) {
	fclose(dumpfile);
	fprintf(stderr, "\n");
    }
    return 0;
}
