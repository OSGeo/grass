#include <stdlib.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include "kappa.h"
#include "local_proto.h"

void print_kappa(void)
{
    int i;
    FILE *fd;

    if (output == NULL)
        fd = stdout;
    else if ((fd = fopen(output, "a")) == NULL)
        G_fatal_error(
            _("Cannot open file <%s> to write kappa and relevant parameters"),
            output);

    /* print out the commission and omission accuracy, and conditional kappa */
    fprintf(fd, "\nCats\t%% Commission\t%% Omission\tEstimated Kappa\n");
    for (i = 0; i < ncat; i++) {
        fprintf(fd, "%ld\t", rlst[i]);
        if (metrics->users_accuracy[i] == na_value)
            fprintf(fd, "NA\t\t");
        else
            fprintf(fd, "%f\t", 100 - metrics->users_accuracy[i]);
        if (metrics->producers_accuracy[i] == na_value)
            fprintf(fd, "NA\t\t");
        else
            fprintf(fd, "%f\t", 100 - metrics->producers_accuracy[i]);
        if (metrics->conditional_kappa[i] == na_value)
            fprintf(fd, "NA\n");
        else
            fprintf(fd, "%f\n", metrics->conditional_kappa[i]);
    }
    fprintf(fd, "\n");
    fprintf(fd, "Kappa\t\tKappa Variance\tMCC\n");
    if (metrics->kappa == na_value)
        fprintf(fd, "NA");
    else
        fprintf(fd, "%f", metrics->kappa);
    if (metrics->kappa_variance == na_value)
        fprintf(fd, "\tNA");
    else
        fprintf(fd, "\t%f", metrics->kappa_variance);
    if (metrics->mcc == na_value)
        fprintf(fd, "\tNA\n");
    else
        fprintf(fd, "\t%f\n", metrics->mcc);
    fprintf(fd, "\nObs Correct\tTotal Obs\t%% Observed Correct\n");
    fprintf(fd, "%ld\t\t%ld\t\t%f\n", metrics->correct, metrics->observations,
            metrics->overall_accuracy);
    if (output != NULL)
        fclose(fd);

    /* print labels for categories of maps */
    print_label();
}
