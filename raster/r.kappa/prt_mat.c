#include <stdlib.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include "kappa.h"
#include "local_proto.h"


static int longcomp(const void *aa, const void *bb);
static int collapse(long *l, int n);


void prn_error_mat(int out_cols, int hdr)
{
    int i, j, k;
    int ncat1, ncat2;
    long x;
    long *clst;

    int num_panels, at_panel;
    int cndx, rndx;
    int first_col = 0, last_col = 0;
    int addflag = 0;
    int thisone;
    long t_row, t_col;
    long t_rowcount, grand_count;
    const char *mapone;
    FILE *fd;

    if (output != NULL) {
	if (hdr)
	    fd = fopen(output, "w");
	else
	    fd = fopen(output, "a");
    }
    else
	fd = stdout;

    if (fd == NULL) {
	G_fatal_error(_("Cannot open file <%s> to write cats and counts (error matrix)"),
		      output);
	return;
    }
    else {
	/* get the cat lists */
	rlst = (long *)G_calloc(nstats * 2, sizeof(long));
	clst = (long *)G_calloc(nstats, sizeof(long));
	for (i = 0; i < nstats; i++) {
	    rlst[i] = Gstats[i].cats[0];
	    clst[i] = Gstats[i].cats[1];
	}

	/* sort the cat lists */
	qsort(rlst, nstats, sizeof(long), longcomp);
	qsort(clst, nstats, sizeof(long), longcomp);

	/* remove repeated cats */
	ncat1 = collapse(rlst, nstats);
	ncat2 = collapse(clst, nstats);

	/* copy clst to the end of rlst, remove repeated cats, and free unused memory */
	for (i = 0; i < ncat2; i++)
	    rlst[ncat1 + i] = clst[i];
	qsort(rlst, ncat1 + ncat2, sizeof(long), longcomp);
	ncat = collapse(rlst, ncat1 + ncat2);
	rlst = (long *)G_realloc(rlst, ncat * sizeof(long));
	G_free(clst);

	/* allocate matrix and fill in with cats' value */
	matr = (long *)G_malloc(ncat * ncat * sizeof(long));
	for (i = 0; i < ncat * ncat; i++)
	    matr[i] = 0;
	for (i = 0; i < nstats; i++) {
	    for (j = 0; j < ncat; j++)
		if (rlst[j] == Gstats[i].cats[0])
		    break;
	    for (k = 0; k < ncat; k++)
		if (rlst[k] == Gstats[i].cats[1])
		    break;
	    /* matrix: reference in columns, classification in rows */
	    matr[j * ncat + k] = Gstats[i].count;
	}

	/* format and print out the error matrix in panels */
	out_cols = (out_cols == 132) ? 9 : 5;
	num_panels = ncat / out_cols;
	if (ncat % out_cols)
	    num_panels++;
	t_rowcount = 0;
	fprintf(fd, "\nError Matrix (MAP1: reference, MAP2: classification)\n");

	for (at_panel = 0; at_panel < num_panels; at_panel++) {
	    first_col = at_panel * out_cols;
	    last_col = first_col + out_cols;
	    if (last_col >= ncat) {
		last_col = ncat;
	    }
	    /* determine whether room available for row total at the end of last panel */
	    addflag = 0;
	    if (at_panel == (num_panels - 1) &&
		(last_col - first_col) < (out_cols - 1)) {
		addflag = 1;
	    }
	    /* panel line */
	    fprintf(fd, "Panel #%d of %d\n", at_panel + 1, num_panels);
	    /* name line */
	    fprintf(fd, "\t\t\t  MAP1\n");
	    /* cat line */
	    fprintf(fd, "     cat#\t");
	    for (cndx = first_col; cndx < last_col; cndx++)
		fprintf(fd, "%ld\t", rlst[cndx]);
	    if (addflag)
		fprintf(fd, "Row Sum");
	    fprintf(fd, "\n");
	    /* body of the matrix */
	    mapone = "MAP2";
	    for (rndx = 0; rndx < ncat; rndx++) {
		if (*(mapone) != '\0')
		    fprintf(fd, " %c %5ld\t", *(mapone)++, rlst[rndx]);
		else
		    fprintf(fd, "   %5ld\t", rlst[rndx]);
		/* entries */
		for (cndx = first_col; cndx < last_col; cndx++) {
		    thisone = (ncat * rndx) + cndx;
		    fprintf(fd, "%ld\t", matr[thisone]);
		}
		/* row marginal summation */
		if (addflag) {
		    t_row = 0;
		    for (k = 0; k < ncat; k++)
			t_row += matr[rndx * ncat + k];
		    t_rowcount += t_row;
		    fprintf(fd, "%ld", t_row);
		}
		fprintf(fd, "\n");
	    }
	    /* column marginal summation */
	    fprintf(fd, "Col Sum\t\t");
	    for (cndx = first_col; cndx < last_col; cndx++) {
		t_col = 0;
		x = cndx;
		for (k = 0; k < ncat; k++) {
		    t_col += matr[x];
		    x += ncat;
		}
		fprintf(fd, "%ld\t", t_col);
	    }
	    /* grand total */
	    if (addflag)
		fprintf(fd, "%ld", t_rowcount);
	    fprintf(fd, "\n\n");
	}

	/* Marginal summation if no room at the end of the last panel */
	if (!addflag) {
	    fprintf(fd, "cat#\tRow Sum\n");
	    mapone = layers[1].name;
	    t_row = 0;
	    t_rowcount = 0;
	    grand_count = 0;
	    for (rndx = 0; rndx < ncat; rndx++) {
		if (*(mapone) != '\0')
		    fprintf(fd, "%c %5ld", *(mapone)++, rlst[rndx]);
		else
		    fprintf(fd, "   %5ld", rlst[rndx]);
		for (cndx = first_col; cndx < last_col; cndx++) {
		    thisone = (ncat * rndx) + cndx;
		    fprintf(fd, " %9ld  ", matr[thisone]);
		    t_row += matr[thisone];
		}
		t_rowcount += t_row;
		grand_count += t_rowcount;
		fprintf(fd, "%9ld\n", t_rowcount);
	    }
	    fprintf(fd, "%9ld\n", grand_count);
	}
	G_free(matr);
	if (output != NULL)
	    fclose(fd);
    }
}


/* remove repeated values */
static int collapse(long *l, int n)
{
    long *c;
    int m;

    c = l;
    m = 1;
    while (n-- > 0) {
	if (*c != *l) {
	    c++;
	    *c = *l;
	    m++;
	}
	l++;
    }

    return m;
}


static int longcomp(const void *aa, const void *bb)
{
    const long *a = aa;
    const long *b = bb;

    if (*a < *b)
	return -1;

    return (*a > *b);
}
