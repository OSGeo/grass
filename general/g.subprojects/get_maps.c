#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include "local_proto.h"

static int cmp(const void *, const void *);

char** get_available_subprojects(int *nsubprojects)
{
    char **ms, **subproject_name;
    int i, n;

    ms = G_get_available_subprojects();
    for (n = 0; ms[n]; n++);

    subproject_name = (char **)G_malloc(n * sizeof(char *));
    for(i = 0; i < n; i++)
        subproject_name[i] = G_store(ms[i]);

    /* sort subprojects */
    qsort(subproject_name, n, sizeof(char *), cmp);

    *nsubprojects = n;
    
    return subproject_name;
}

int cmp(const void *a, const void *b) 
{
    return (strcmp(*(char **)a, *(char **)b));
}

const char *substitute_subproject(const char *subproject)
{
    if (strcmp(subproject, ".") == 0)
        return G_subproject();

    return subproject;
}
