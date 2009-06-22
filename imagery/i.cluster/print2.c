#include <grass/gis.h>
#include <grass/glocale.h>
#include <grass/imagery.h>
#include <grass/cluster.h>


/* safe to call only during checkpoint(2) and after
 * I_cluster_exec() completes
 * otherwise call I_cluster_sum2() before calling this routine
 */

int print_class_means(FILE * fd, struct Cluster *C)
{
    int band;
    int c;
    int n;

    fprintf(fd, _("\nclass means/stddev for each band\n\n"));

    for (c = 0; c < C->nclasses; c++) {
	fprintf(fd, "\n");
	fprintf(fd, _("class %d (%d)\n"), c + 1, n = C->count[c]);
	fprintf(fd, _("  means "));
	if (n > 0)
	    for (band = 0; band < C->nbands; band++)
		fprintf(fd, " %g", C->sum[band][c] / n);
	fprintf(fd, "\n");
	fprintf(fd, _("  stddev"));
	if (n > 1)
	    for (band = 0; band < C->nbands; band++)
		fprintf(fd, " %g",
			I_stddev(C->sum[band][c], C->sum2[band][c], n));
	fprintf(fd, "\n");
    }
    fprintf(fd, "\n");

    return 0;
}
