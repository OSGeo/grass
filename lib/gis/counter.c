#include <grass/gis.h>

#ifdef USE_PTHREADS
#include <pthread.h>
static pthread_mutex_t mutex;
#endif

void G_init_counter(struct Counter *c, int v)
{
#ifdef USE_PTHREADS
    make_mutex();
#endif
    c->value = v;
}

int G_counter_next(struct Counter *c)
{
    int v;
#ifdef USE_PTHREADS
    pthread_mutex_lock(&mutex);
#endif
    v = c->value++;
#ifdef USE_PTHREADS
    pthread_mutex_unlock(&mutex);
#endif
    return v;
}

#ifdef USE_PTHREADS
static void make_mutex(void)
{
    static pthread_mutex_t t_mutex = PTHREAD_MUTEX_INITIALIZER;
    static int initialized;
    pthread_mutexattr_t attr;

    if (initialized)
	return;

    pthread_mutex_lock(&t_mutex);

    if (initialized) {
	pthread_mutex_unlock(&t_mutex);
	return;
    }

    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&r_mutex, &attr);
    initialized = 1;

    pthread_mutex_unlock(&t_mutex);
}
#endif

int G_is_initialized(int *p)
{
    if (*p)
	return 1;

#ifdef USE_PTHREADS
    make_mutex();
    pthread_mutex_lock(&mutex);
#endif
    return 0;
}

void G_initialize_done(int *p)
{
    *p = 1;

#ifdef USE_PTHREADS
    pthread_mutex_unlock(&mutex);
#endif
}

