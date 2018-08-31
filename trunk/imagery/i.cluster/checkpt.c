#include <grass/gis.h>
#include <grass/glocale.h>
#include "global.h"
#include "local_proto.h"


int checkpoint(struct Cluster *X, int n)
{
    time_t elapsed_time, cur_time;
    int c, band;

    switch (n) {
    case 1:
	print_band_means(report, X);
	if (insigfile) {
	    fprintf(report, _("using seed means (%d files)%s"),
                    ref.nfiles, HOST_NEWLINE);
	    for (c = 0; c < in_sig.nsigs; c++)
		for (band = 0; band < ref.nfiles; band++)
		    X->mean[band][c] = in_sig.sig[c].mean[band];
	}
	print_seed_means(report, X);
	break;
    case 2:
	print_class_means(report, X);
	print_distribution(report, X);
	break;
    case 3:
	fprintf(report, _("%s######## iteration %d ###########%s"),
		HOST_NEWLINE, X->iteration, HOST_NEWLINE);
	fprintf(report, _("%d classes, %.2f%% points stable%s"),
		I_cluster_nclasses(X, 1), (double)X->percent_stable,
                HOST_NEWLINE);
	/*
	   I_cluster_sum2 (X);
	   print_class_means(report,X);
	 */
	print_distribution(report, X);
	if (G_verbose() > G_verbose_std()) {
	    cur_time = time(NULL);
	    elapsed_time = cur_time - start_time;
	    G_message(_("Iteration %.2d: convergence %.2f%% (%s elapsed, %s left)"),
		      X->iteration, (double)X->percent_stable,
		      print_time(elapsed_time),
		      print_time(iters * elapsed_time / (X->iteration + 1) -
				 elapsed_time));
	}
	break;
    case 4:
	/*
	   fprintf (report, _("%smerging class %d into %d%s"),
	   HOST_NEWLINE, X->merge2+1, X->merge1+1, HOST_NEWLINE);
	 */
	break;
    }
    fflush(report);
    return 1;
}
