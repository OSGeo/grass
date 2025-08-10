#include <stdlib.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include "kappa.h"
#include "local_proto.h"

void print_error_mat(int out_cols, int hdr)
{
    int num_panels, at_panel;
    int cndx, rndx;
    int first_col = 0, last_col = 0;
    int addflag = 0;
    int thisone;
    long t_row;
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

    if (fd == NULL)
        G_fatal_error(
            _("Cannot open file <%s> to write cats and counts (error matrix)"),
            output);
    else {
        /* format and print out the error matrix in panels */
        out_cols = (out_cols == 132) ? 9 : 5;
        num_panels = ncat / out_cols;
        if (ncat % out_cols)
            num_panels++;
        fprintf(fd, "\nError Matrix (MAP1: reference, MAP2: classification)\n");

        for (at_panel = 0; at_panel < num_panels; at_panel++) {
            first_col = at_panel * out_cols;
            last_col = first_col + out_cols;
            if (last_col >= ncat) {
                last_col = ncat;
            }
            /* determine whether room available for row total at the end of last
             * panel */
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
                    fprintf(fd, "%ld\t", metrics->matrix[thisone]);
                }
                /* row marginal summation */
                if (addflag) {
                    fprintf(fd, "%ld", metrics->row_sum[rndx]);
                }
                fprintf(fd, "\n");
            }
            /* column marginal summation */
            fprintf(fd, "Col Sum\t\t");
            for (cndx = first_col; cndx < last_col; cndx++) {
                fprintf(fd, "%ld\t", metrics->col_sum[cndx]);
            }
            /* grand total */
            if (addflag)
                fprintf(fd, "%ld", metrics->observations);
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
                    fprintf(fd, " %9ld  ", metrics->matrix[thisone]);
                    t_row += metrics->matrix[thisone];
                }
                t_rowcount += t_row;
                grand_count += t_rowcount;
                fprintf(fd, "%9ld\n", t_rowcount);
            }
            fprintf(fd, "%9ld\n", grand_count);
        }
        if (output != NULL)
            fclose(fd);
    }
}
