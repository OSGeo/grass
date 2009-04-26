#include <stdlib.h>
#include <grass/gis.h>
#include <grass/Vect.h>
#include <grass/glocale.h>
#include "data_types.h"

struct vertex *sites;
static struct edge *edges;
static struct edge **free_list_e;

static unsigned int n_free_e;

void alloc_memory(unsigned int n)
{
    struct edge *e;
    int i;

    /* Sites storage. */
    sites = (struct vertex *)G_calloc(n, sizeof(struct vertex));
    if (sites == MY_NULL)
	G_fatal_error(_("Not enough memory."));

    /* Edges. Euler's formula - at most 3n edges on a set of n sites */
    n_free_e = 3 * n;
    edges = e = (struct edge *)G_calloc(n_free_e, sizeof(struct edge));
    if (edges == MY_NULL)
	G_fatal_error(_("Not enough memory."));

    free_list_e = (struct edge **)G_calloc(n_free_e, sizeof(struct edge *));
    if (free_list_e == MY_NULL)
	G_fatal_error(_("Not enough memory."));
    for (i = 0; i < n_free_e; i++, e++)
	free_list_e[i] = e;
}

void alloc_sites(unsigned int n)
{
    /* Sites storage. */
    sites = (struct vertex *)G_calloc(n, sizeof(struct vertex));
    if (sites == MY_NULL)
	G_fatal_error(_("Not enough memory."));
}

void realloc_sites(unsigned int n)
{
    /* Sites storage. */
    sites = (struct vertex *)G_realloc(sites, n * sizeof(struct vertex));
    if (sites == MY_NULL)
	G_fatal_error(_("Not enough memory."));
}

void alloc_edges(unsigned int n)
{
    struct edge *e;
    int i;

    /* Edges. Euler's formula - at most 3n edges on a set of n sites */
    n_free_e = 3 * n;
    edges = e = (struct edge *)G_calloc(n_free_e, sizeof(struct edge));
    if (edges == MY_NULL)
	G_fatal_error(_("Not enough memory."));

    free_list_e = (struct edge **)G_calloc(n_free_e, sizeof(struct edge *));
    if (free_list_e == MY_NULL)
	G_fatal_error(_("Not enough memory."));
    for (i = 0; i < n_free_e; i++, e++)
	free_list_e[i] = e;
}


void free_memory()
{
    G_free(sites);
    G_free(edges);
    G_free(free_list_e);
}

struct edge *get_edge()
{
    if (n_free_e < 1)
	G_fatal_error(_("All allocated edges have been used."));
    return (free_list_e[--n_free_e]);
}

void free_edge(struct edge *e)
{
    free_list_e[n_free_e++] = e;
}
