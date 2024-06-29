#if defined(_OPENMP)
#include <omp.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <grass/gis.h>
#include <grass/glocale.h>

/*! \brief A helper function to setup the number of threads for C modules
   supported by OpenMP
   \param nprocs the number of threads specified by the user
   \return the number of threads set up for OpenMP parallel computing
 */

int G_setup_threads(char *nprocs)
{
    int threads = atoi(nprocs);
#if defined(_OPENMP)
    int max_cores = omp_get_num_procs();
    if (threads > max_cores) {
        G_warning(
            _("The number of threads specified is greater than the "
              "number of processors available (%d). The number of threads "
              "will be set to %d."),
            max_cores, max_cores);
        threads = max_cores;
    }
    else if (threads < 1) {
        threads += max_cores;
        threads = (threads < 1) ? 1 : threads;
    }
    omp_set_num_threads(threads);
    G_message(_("%d threads are set up for parallel computing."), threads);
#else
    if (threads != 1) {
        G_warning(_("GRASS GIS is not compiled with OpenMP support, parallel "
                    "computation is disabled. Only one thread will be used."));
        threads = 1;
    }
#endif

    return threads;
}
