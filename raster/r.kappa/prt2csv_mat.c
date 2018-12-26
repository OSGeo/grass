#include <stdlib.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include "kappa.h"
#include "local_proto.h"


static int longcomp(const void *aa, const void *bb);
static int collapse(long *l, int n);


void prn2csv_error_mat(int out_cols, int hdr)
{
    int i, j, k;
    int ncat1, ncat2;
    long x;
    long *clst;

    int cndx, rndx;
    int first_col = 0, last_col = 0;
    int thisone;
    long t_row, t_col;
    long t_rowcount, grand_count;
    const char *mapone;
    FILE *fd;

    long *cats;
    char *cl;

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
	out_cols = 2048;
	t_rowcount = 0;
	first_col = 0;
	last_col = ncat;
	/* name line */
	/*fprintf(fd, "\t\t\t  MAP1\n");*/
	/* cat line */
	fprintf(fd, "cat#\t");
        /* print labels MAP1*/
	for (j = 0; j < ncat; j++) {
          cats = rlst;
	  cl = Rast_get_c_cat((CELL *) &(cats[j]), &(layers[0].labels));
	  if (cl)
	    G_strip(cl);
	  if (cl == NULL || *cl == 0)
	    cl = "NULL";
	  fprintf(fd, "%s\t", cl);
	}
	/*for (cndx = first_col; cndx < last_col; cndx++) */
	/*    fprintf(fd, "%ld\t", rlst[cndx]); */
        fprintf(fd, "RowSum");
        fprintf(fd, "\n");
        /* body of the matrix */
	mapone = "MAP2";
        for (rndx = 0; rndx < ncat; rndx++) {
            cats = rlst;
	    cl = Rast_get_c_cat((CELL *) &(cats[rndx]),&(layers[1].labels));
	    if (cl)
	      G_strip(cl);
	    if (cl == NULL || *cl == 0)
	      cl = "NULL";
	    fprintf(fd, "%s\t", cl);
	    /* entries */
	    for (cndx = first_col; cndx < last_col; cndx++) {
	        thisone = (ncat * rndx) + cndx;
	        fprintf(fd, "%ld\t", matr[thisone]);
	    }
	    /* row marginal summation */
	    t_row = 0;
	    for (k = 0; k < ncat; k++)
	       t_row += matr[rndx * ncat + k];
	    t_rowcount += t_row;
	    fprintf(fd, "%ld", t_row);
	    fprintf(fd, "\n");
	}
	/* column marginal summation */
	fprintf(fd, "ColSum\t");
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
	fprintf(fd, "%ld", t_rowcount);
	fprintf(fd, "\n\n");
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

    return (*a - *b);
}
