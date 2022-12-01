#include <stdlib.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include "kappa.h"
#include "local_proto.h"


void prn_json()
{
    bool first;
    FILE *fd;

    if (output != NULL)
        fd = fopen(output, "w");
    else
        fd = stdout;

    if (fd == NULL) {
        G_fatal_error(_("Cannot open file <%s> to write JSON output"),
                      output);
        return;
    }

    fprintf(fd, "{\n");
    fprintf(fd, "    \"map1\": \"%s\",\n", maps[0]);
    fprintf(fd, "    \"map2\": \"%s\",\n", maps[1]);
    fprintf(fd, "    \"observations\": %ld,\n", metrics->obs);
    fprintf(fd, "    \"correct\": %ld,\n", metrics->correct);
    fprintf(fd, "    \"total_acc\": %.5f,\n", metrics->total_acc);
    fprintf(fd, "    \"kappa\": %.5f,\n", metrics->kappa);
    fprintf(fd, "    \"kappa_var\": %.5f,\n", metrics->kappa_var);
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
    fprintf(fd, "    \"prod_acc\": [");
    first = 1;
    for (int i = 0; i < ncat; i++) {
        if (first)
            first = 0;
        else
            fprintf(fd, ", ");
        fprintf(fd, "%.5f", metrics->prod_acc[i]);
    }
    fprintf(fd, "],\n");
    fprintf(fd, "    \"user_acc\": [");
    first = 1;
    for (int i = 0; i < ncat; i++) {
        if (first)
            first = 0;
        else
            fprintf(fd, ", ");
        fprintf(fd, "%.5f", metrics->user_acc[i]);
    }
    fprintf(fd, "],\n");
    fprintf(fd, "    \"cond_kappa\": [");
    first = 1;
    for (int i = 0; i < ncat; i++) {
        if (first)
            first = 0;
        else
            fprintf(fd, ", ");
        fprintf(fd, "%.5f", metrics->cond_kappa[i]);
    }
    fprintf(fd, "]\n");

    fprintf(fd, "}\n");
    if (output != NULL)
        fclose(fd);
}
