#include <stdlib.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include "kappa.h"
#include "local_proto.h"

void print2csv_error_mat(int hdr)
{
    int j;

    int cndx, rndx;
    int first_col, last_col;
    int thisone;
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

    if (fd == NULL)
        G_fatal_error(
            _("Cannot open file <%s> to write cats and counts (error matrix)"),
            output);
    else {
        /* format and print out the error matrix in panels */
        first_col = 0;
        last_col = ncat;
        /* name line */
        /*fprintf(fd, "\t\t\t  MAP1\n"); */
        /* cat line */
        fprintf(fd, "cat#\t");
        /* print labels MAP1 */
        for (j = 0; j < ncat; j++) {
            cats = rlst;
            cl = Rast_get_c_cat((CELL *)&(cats[j]), &(layers[0].labels));
            if (cl)
                G_strip(cl);
            if (cl == NULL || *cl == 0)
                fprintf(fd, "%ld\t", cats[j]);
            else
                fprintf(fd, "%s\t", cl);
        }
        /*for (cndx = first_col; cndx < last_col; cndx++) */
        /*    fprintf(fd, "%ld\t", rlst[cndx]); */
        fprintf(fd, "RowSum");
        fprintf(fd, "\n");
        /* body of the matrix */
        for (rndx = 0; rndx < ncat; rndx++) {
            cats = rlst;
            cl = Rast_get_c_cat((CELL *)&(cats[rndx]), &(layers[1].labels));
            if (cl)
                G_strip(cl);
            if (cl == NULL || *cl == 0)
                fprintf(fd, "%ld\t", cats[rndx]);
            else
                fprintf(fd, "%s\t", cl);
            /* entries */
            for (cndx = first_col; cndx < last_col; cndx++) {
                thisone = (ncat * rndx) + cndx;
                fprintf(fd, "%ld\t", metrics->matrix[thisone]);
            }
            /* row marginal summation */
            fprintf(fd, "%ld", metrics->row_sum[rndx]);
            fprintf(fd, "\n");
        }
        /* column marginal summation */
        fprintf(fd, "ColSum\t");
        for (cndx = first_col; cndx < last_col; cndx++) {
            fprintf(fd, "%ld\t", metrics->col_sum[cndx]);
        }
        /* grand total */
        fprintf(fd, "%ld", metrics->observations);
        fprintf(fd, "\n\n");
        if (output != NULL)
            fclose(fd);
    }
}
