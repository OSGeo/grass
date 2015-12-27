/*!
 * \file lib/gis/worker.c
 *
 * \brief GIS Library - Worker functions.
 *
 * (C) 2008-2014 by the GRASS Development Team
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author Glynn Clements
 */

#include <stdio.h>
#include <stdlib.h>
#include <grass/gis.h>
#include <grass/glocale.h>

#ifdef HAVE_PTHREAD_H

/****************************************************************************/

#include <pthread.h>

#define DEFAULT_WORKERS 0

struct worker {
    void (*func)(void *);
    void *closure;
    void **ref;
    pthread_t thread;
    pthread_cond_t cond;
    pthread_mutex_t mutex;
    int cancel;
};

static int num_workers;
static struct worker *workers;
static pthread_cond_t worker_cond;
static pthread_mutex_t worker_mutex;

/****************************************************************************/

static void *worker(void *arg)
{
    struct worker *w = arg;

    while (!w->cancel) {
	pthread_mutex_lock(&w->mutex);
	while (!w->func)
	    pthread_cond_wait(&w->cond, &w->mutex);

	(*w->func)(w->closure);

	w->func = NULL;
	w->closure = NULL;
	*w->ref = NULL;
	pthread_mutex_unlock(&w->mutex);
	pthread_cond_signal(&w->cond);
	pthread_cond_signal(&worker_cond);
    }

    return NULL;
}

static struct worker *get_worker(void)
{
    int i;

    for (i = 0; i < num_workers; i++) {
	struct worker *w = &workers[i];
	if (!w->func)
	    return w;
    }

    return NULL;
}

void G_begin_execute(void (*func)(void *), void *closure, void **ref, int force)
{
    struct worker *w;
 
    if (*ref)
	G_fatal_error(_("Task already has a worker"));

    pthread_mutex_lock(&worker_mutex);

    while (w = get_worker(), force && num_workers > 0 && !w)
	pthread_cond_wait(&worker_cond, &worker_mutex);
    *ref = w;

    if (!w) {
	pthread_mutex_unlock(&worker_mutex);
	(*func)(closure);
	return;
    }

    pthread_mutex_lock(&w->mutex);
    w->func = func;
    w->closure = closure;
    w->ref = ref;
    pthread_cond_signal(&w->cond);
    pthread_mutex_unlock(&w->mutex);

    pthread_mutex_unlock(&worker_mutex);
}

void G_end_execute(void **ref)
{
    struct worker *w = *ref;

    if (!w)
	return;

    pthread_mutex_lock(&w->mutex);
    while (*ref)
	pthread_cond_wait(&w->cond, &w->mutex);
    pthread_mutex_unlock(&w->mutex);
}

void G_init_workers(void)
{
    const char *p = getenv("WORKERS");
    int i;

    pthread_mutex_init(&worker_mutex, NULL);
    pthread_cond_init(&worker_cond, NULL);

    num_workers = p ? atoi(p) : DEFAULT_WORKERS;
    workers = G_calloc(num_workers, sizeof(struct worker));

    for (i = 0; i < num_workers; i++) {
	struct worker *w = &workers[i];
	pthread_mutex_init(&w->mutex, NULL);
	pthread_cond_init(&w->cond, NULL);
	pthread_create(&w->thread, NULL, worker, w);
    }
}

void G_finish_workers(void)
{
    int i;

    for (i = 0; i < num_workers; i++) {
	struct worker *w = &workers[i];
	w->cancel = 1;
	pthread_cancel(w->thread);
    }

    for (i = 0; i < num_workers; i++) {
	struct worker *w = &workers[i];
	pthread_join(w->thread, NULL);
	pthread_mutex_destroy(&w->mutex);
	pthread_cond_destroy(&w->cond);
    }

    pthread_mutex_destroy(&worker_mutex);
    pthread_cond_destroy(&worker_cond);
}

/****************************************************************************/

#else

/****************************************************************************/

void G_begin_execute(void (*func)(void *), void *closure, void **ref, int force)
{
    (*func)(closure);
}

void G_end_execute(void **ref)
{
}

void G_init_workers(void)
{
}

void G_finish_workers(void)
{
}

/****************************************************************************/

#endif

