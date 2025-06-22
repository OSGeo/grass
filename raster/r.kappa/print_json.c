#include <stdbool.h>
#include <stdlib.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include "kappa.h"
#include "local_proto.h"

void print_json(void)
{
    bool first;
    FILE *fd;

    if (output != NULL)
        fd = fopen(output, "w");
    else
        fd = stdout;

    if (fd == NULL)
        G_fatal_error(_("Cannot open file <%s> to write JSON output"), output);

    fprintf(fd, "{\n");
    fprintf(fd, "    \"reference\": \"%s\",\n", maps[0]);
    fprintf(fd, "    \"classification\": \"%s\",\n", maps[1]);
    fprintf(fd, "    \"observations\": %ld,\n", metrics->observations);
    fprintf(fd, "    \"correct\": %ld,\n", metrics->correct);
    fprintf(fd, "    \"overall_accuracy\": %.5f,\n", metrics->overall_accuracy);
    if (metrics->kappa == na_value)
        fprintf(fd, "    \"kappa\": null,\n");
    else
        fprintf(fd, "    \"kappa\": %.5f,\n", metrics->kappa);
    if (metrics->kappa_variance == na_value)
        fprintf(fd, "    \"kappa_variance\": null,\n");
    else
        fprintf(fd, "    \"kappa_variance\": %.5f,\n", metrics->kappa_variance);
    fprintf(fd, "    \"cats\": [");
    first = 1;
    for (int i = 0; i < ncat; i++) {
        if (first)
            first = 0;
        else
            fprintf(fd, ", ");
        fprintf(fd, "%ld", rlst[i]);
    }
    fprintf(fd, "],\n");
    fprintf(fd, "    \"matrix\": [\n        [");
    first = 1;
    for (int i = 0; i < ncat; i++) {
        if (first)
            first = 0;
        else
            fprintf(fd, "],\n        [");
        bool cfirst = 1;

        for (int j = 0; j < ncat; j++) {
            if (cfirst)
                cfirst = 0;
            else
                fprintf(fd, ", ");
            fprintf(fd, "%ld", metrics->matrix[ncat * i + j]);
        }
    }
    fprintf(fd, "]\n    ],\n");
    fprintf(fd, "    \"row_sum\": [");
    first = 1;
    for (int i = 0; i < ncat; i++) {
        if (first)
            first = 0;
        else
            fprintf(fd, ", ");
        fprintf(fd, "%ld", metrics->row_sum[i]);
    }
    fprintf(fd, "],\n");
    fprintf(fd, "    \"col_sum\": [");
    first = 1;
    for (int i = 0; i < ncat; i++) {
        if (first)
            first = 0;
        else
            fprintf(fd, ", ");
        fprintf(fd, "%ld", metrics->col_sum[i]);
    }
    fprintf(fd, "],\n");
    fprintf(fd, "    \"producers_accuracy\": [");
    first = 1;
    for (int i = 0; i < ncat; i++) {
        if (first)
            first = 0;
        else
            fprintf(fd, ", ");
        if (metrics->producers_accuracy[i] == na_value)
            fprintf(fd, "null");
        else
            fprintf(fd, "%.5f", metrics->producers_accuracy[i]);
    }
    fprintf(fd, "],\n");
    fprintf(fd, "    \"users_accuracy\": [");
    first = 1;
    for (int i = 0; i < ncat; i++) {
        if (first)
            first = 0;
        else
            fprintf(fd, ", ");
        if (metrics->users_accuracy[i] == na_value)
            fprintf(fd, "null");
        else
            fprintf(fd, "%.5f", metrics->users_accuracy[i]);
    }
    fprintf(fd, "],\n");
    fprintf(fd, "    \"conditional_kappa\": [");
    first = 1;
    for (int i = 0; i < ncat; i++) {
        if (first)
            first = 0;
        else
            fprintf(fd, ", ");
        if (metrics->conditional_kappa[i] == na_value)
            fprintf(fd, "null");
        else
            fprintf(fd, "%.5f", metrics->conditional_kappa[i]);
    }
    fprintf(fd, "],\n");
    if (metrics->mcc == na_value)
        fprintf(fd, "    \"mcc\": null\n");
    else
        fprintf(fd, "    \"mcc\": %.5f\n", metrics->mcc);

    fprintf(fd, "}\n");
    if (output != NULL)
        fclose(fd);
}
