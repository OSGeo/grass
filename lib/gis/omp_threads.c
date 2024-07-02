#if defined(_OPENMP)
#include <omp.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/glocale.h>

/*! \brief A helper function to setup the number of threads for C modules
   supported by OpenMP
   \param opt A nprocs Option struct to specify the number of threads
   \return the number of threads set up for OpenMP parallel computing
 */

int G_set_omp_num_threads(struct Option *opt)
{
    /* make sure the nproc Option is given */
    if (opt == NULL)
        G_fatal_error(_("Option is NULL."));
    else if (opt->key == NULL)
        G_fatal_error(_("Option key is NULL."));
    else if (strcmp(opt->key, "nprocs") != 0)
        G_fatal_error(_("This function can only be used for 'nprocs' Option."));

    int threads = atoi(opt->answer);
#if defined(_OPENMP)
    int CPU_threads = omp_get_num_procs();
    if (threads > CPU_threads) {
        G_warning(_("The number of threads specified for OpenMP is more "
                    "than the number of CPU threads (%d). The performance "
                    "may be degraded for an CPU-bound program. Reference: "
                    "https://www.baeldung.com/cs/servers-threads-number"),
                  CPU_threads);
    }
    else if (threads < 1) {
        threads += CPU_threads;
        threads = (threads < 1) ? 1 : threads;
    }
    omp_set_num_threads(threads);
    G_verbose_message(_("%d threads are set up for parallel computing."),
                      threads);
#else
    if (threads != 1) {
        G_warning(_("GRASS GIS is not compiled with OpenMP support, parallel "
                    "computation is disabled. Only one thread will be used."));
        threads = 1;
    }
#endif
    return threads;
}
