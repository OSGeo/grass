#include <grass/gis.h>
#include <grass/glocale.h>
#include <grass/imagery.h>
#include <grass/cluster.h>


/*
 * safe to call only during checkpoint(1)
 */

int print_seed_means(FILE * fd, struct Cluster *C)
{
    int band;
    int c;

    fprintf(fd, _("%sinitial means for each band%s%s"),
	    HOST_NEWLINE, HOST_NEWLINE, HOST_NEWLINE);

    for (c = 0; c < C->nclasses; c++) {
	fprintf(fd, _("class %-3d "), c + 1);
	for (band = 0; band < C->nbands; band++)
	    fprintf(fd, " %g", C->mean[band][c]);
	fprintf(fd, "%s", HOST_NEWLINE);
    }
    fprintf(fd, "%s", HOST_NEWLINE);

    return 0;
}
