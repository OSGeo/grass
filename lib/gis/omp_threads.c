#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_OPENMP)
#include <omp.h>
#endif

#include <grass/gis.h>
#include <grass/glocale.h>

/*!
 * \brief Resolve a value of nprocs into a number of threads
 *
 * A positive value is the number of threads. With OpenMP, 0 is the
 * OpenMP default (omp_get_max_threads()), which follows OMP_NUM_THREADS.
 * A negative value is that many threads fewer than the processors
 * available to the process, at least one, and does not depend on
 * OMP_NUM_THREADS. Without OpenMP, 0 and negative values give one thread.
 *
 * \param nprocs value of an nprocs option
 *
 * \return the number of threads, at least 1
 *
 * \since version 8.6
 */
int G_resolve_nprocs(int nprocs)
{
#if defined(_OPENMP)
    if (nprocs == 0)
        return omp_get_max_threads();
    if (nprocs < 0)
        nprocs += omp_get_num_procs();
#endif
    return nprocs < 1 ? 1 : nprocs;
}

/*!
 * \brief Read the number of threads from an nprocs option
 *
 * Call after G_parser(). The answer must be an integer, optionally with
 * white space around it. Otherwise, the function ends with a fatal error
 * naming the option, also for answers such as 2x which the parser accepts
 * for an integer option. The integer is resolved with G_resolve_nprocs().
 *
 * The function does not set up OpenMP threads.
 *
 * \param nprocs an nprocs option, usually G_OPT_M_NPROCS
 *
 * \return the number of threads, at least 1
 *
 * \since version 8.6
 */
int G_nprocs_from_option(const struct Option *nprocs)
{
    if (nprocs == NULL)
        G_fatal_error(_("Option is NULL."));
    if (nprocs->key == NULL)
        G_fatal_error(_("Option key is NULL."));
    const char *answer = nprocs->answer ? nprocs->answer : "";
    char *end;
    errno = 0;
    long value = strtol(answer, &end, 10);
    if (errno || end == answer || end[strspn(end, " \t\n\v\f\r")] != '\0' ||
        value > INT_MAX || value < INT_MIN)
        G_fatal_error(
            _("Invalid value <%s> for option <%s>. Use an integer number of "
              "threads."),
            answer, nprocs->key);
    return G_resolve_nprocs((int)value);
}

/*! \brief Set the number of threads for OpenMP
    The intended usage is at the beginning of a C tool when parameters are
    processed, namely the G_OPT_M_NPROCS standard option.

    The number of threads is read with G_nprocs_from_option(). See
    G_resolve_nprocs() for what 0 and negative values mean. Without
    OpenMP, the function warns when more than one thread is requested.

    \param opt A nprocs Option struct to specify the number of threads
    \return the number of threads set up for OpenMP parallel computing

    \since version 8.5
*/
int G_set_omp_num_threads(struct Option *opt)
{
    int threads = G_nprocs_from_option(opt);
#if defined(_OPENMP)
    omp_set_num_threads(threads);
    G_verbose_message(n_("One thread is set up for parallel computing.",
                         "%d threads are set up for parallel computing.",
                         threads),
                      threads);
#else
    if (threads > 1)
        G_warning(_("GRASS is not compiled with OpenMP support, parallel "
                    "computation is disabled. Only one thread will be used."));
    threads = 1;
#endif
    return threads;
}
