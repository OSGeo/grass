#if defined(_OPENMP)
#include <omp.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/glocale.h>

/*! \brief Set the number of threads for OpenMP
    The intended usage is at the beginning of a C tool when parameters are
    processed, namely the G_OPT_M_NPROCS standard option.

    \param opt A nprocs Option struct to specify the number of threads
    \return the number of threads set up for OpenMP parallel computing
*/

int G_set_omp_num_threads(struct Option *opt)
{
    /* make sure Option is not null */
    if (opt == NULL)
        G_fatal_error(_("Option is NULL."));
    else if (opt->key == NULL)
        G_fatal_error(_("Option key is NULL."));

    int threads = atoi(opt->answer);
#if defined(_OPENMP)
    int num_logic_procs = omp_get_num_procs();
    if (threads < 1) {
        threads += num_logic_procs;
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
