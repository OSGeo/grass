#include <grass/gis.h>
#include <grass/glocale.h>
#include <grass/imagery.h>
#include <grass/cluster.h>


int print_band_means(FILE * fd, struct Cluster *C)
{
    int band;

    fprintf(fd, "%s", HOST_NEWLINE);
    fprintf(fd, _("means and standard deviations for %d band%s%s%s"),
	    C->nbands, C->nbands == 1 ? "" : "s", HOST_NEWLINE, HOST_NEWLINE);
    fprintf(fd, _(" means  "));
    for (band = 0; band < C->nbands; band++)
	fprintf(fd, " %g", C->band_sum[band] / C->npoints);
    fprintf(fd, "%s", HOST_NEWLINE);
    fprintf(fd, _(" stddev "));
    for (band = 0; band < C->nbands; band++)
	fprintf(fd, " %g",
		I_stddev(C->band_sum[band], C->band_sum2[band], C->npoints));
    fprintf(fd, "%s%s", HOST_NEWLINE, HOST_NEWLINE);

    return 0;
}
