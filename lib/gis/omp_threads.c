#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_OPENMP)
#include <omp.h>
#endif

#include <grass/gis.h>
#include <grass/glocale.h>

/*! \brief Set the number of threads for OpenMP
    The intended usage is at the beginning of a C tool when parameters are
    processed, namely the G_OPT_M_NPROCS standard option.

    If \em nprocs is set to 0, default OpenMP internal logic is used.
    If \em nprocs is a positive number, specified number of threads is used.
    If \em nprocs is a negative number, then <em>maximum threads - number</em>
    is used instead (e.g. to keep \em number of cores free for other use.

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
    if (threads == 0) {
        threads = omp_get_max_threads();
    }
    else {
        int num_logic_procs = omp_get_num_procs();
        if (threads < 1) {
            threads += num_logic_procs;
            threads = (threads < 1) ? 1 : threads;
        }
        omp_set_num_threads(threads);
    }
    G_verbose_message(n_("One thread is set up for parallel computing.",
                         "%d threads are set up for parallel computing.",
                         threads),
                      threads);
#else
    if (!(threads == 0 || threads == 1)) {
        G_warning(_("GRASS is not compiled with OpenMP support, parallel "
                    "computation is disabled. Only one thread will be used."));
    }
    threads = 1;
#endif
    return threads;
}
