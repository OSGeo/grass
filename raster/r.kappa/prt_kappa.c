#include <stdlib.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include "kappa.h"
#include "local_proto.h"


void prt_kappa(void)
{
    int i;
    FILE *fd;

    if (output == NULL)
        fd = stdout;
    else if ((fd = fopen(output, "a")) == NULL) {
        G_fatal_error(_("Cannot open file <%s> to write kappa and relevant parameters"),
                      output);
        return;
    }

    /* print out the comission and omission accuracy, and conditional kappa */
    fprintf(fd, "\nCats\t%% Comission\t%% Omission\tEstimated Kappa\n");
    for (i = 0; i < ncat; i++) {
        fprintf(fd, "%ld\t", rlst[i]);
        if (metrics->user_acc[i] == -999)
            fprintf(fd, "NA\t\t");
        else
            fprintf(fd, "%f\t", metrics->user_acc[i]);
        if (metrics->prod_acc[i] == -999)
            fprintf(fd, "NA\t\t");
        else
            fprintf(fd, "%f\t", metrics->prod_acc[i]);
        if (metrics->cond_kappa[i] == -999)
            fprintf(fd, "NA\n");
        else
            fprintf(fd, "%f\n", metrics->cond_kappa[i]);
    }
    fprintf(fd, "\n");
    fprintf(fd, "Kappa\t\tKappa Variance\n");
    if (metrics->kappa == -999)
        fprintf(fd, "NA");
    else
        fprintf(fd, "%f", metrics->kappa);
    if (metrics->kappa_var == -999)
        fprintf(fd, "\tNA\n");
    else
        fprintf(fd, "\t%f\n", metrics->kappa_var);

    fprintf(fd, "\nObs Correct\tTotal Obs\t%% Observed Correct\n");
    fprintf(fd, "%ld\t\t%ld\t\t%f\n", metrics->correct, metrics->obs,
            metrics->total_acc);
    if (output != NULL)
        fclose(fd);

    /* print labels for categories of maps */
    prt_label();
}
