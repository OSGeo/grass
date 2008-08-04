#include <stdio.h>
#include <stdlib.h>
#include <grass/imagery.h>


void *I_malloc(size_t n)
{
    void *b;

    b = G_malloc(n);

    return b;
}


void *I_realloc(void *b, size_t n)
{
    b = G_realloc(b, n);

    return b;
}


int I_free(void *b)
{
    if ((char *)b != NULL)
	G_free(b);

    return 0;
}


double **I_alloc_double2(int a, int b)
{
    double **x;
    int i;

    x = (double **)I_malloc((a + 1) * sizeof(double *));

    for (i = 0; i < a; i++) {
	int n;

	x[i] = (double *)I_malloc(b * sizeof(double));

	for (n = 0; n < b; n++)
	    x[i][n] = 0;
    }
    x[a] = NULL;

    return x;
}


int *I_alloc_int(int a)
{
    int *x;
    int i;

    x = (int *)I_malloc(a * sizeof(int));

    for (i = 0; i < a; i++)
	x[i] = 0;

    return x;
}

int **I_alloc_int2(int a, int b)
{
    int **x;
    int i, n;

    x = (int **)I_malloc((a + 1) * sizeof(int *));

    for (i = 0; i < a; i++) {
	x[i] = (int *)I_malloc(b * sizeof(int));

	for (n = 0; n < b; n++)
	    x[i][n] = 0;
    }
    x[a] = NULL;

    return x;
}


int I_free_int2(int **x)
{
    int i;

    if (x != NULL) {
	for (i = 0; x[i] != NULL; i++)
	    G_free(x[i]);
	G_free(x);
    }

    return 0;
}


int I_free_double2(double **x)
{
    int i;

    if (x != NULL) {
	for (i = 0; x[i] != NULL; i++)
	    G_free(x[i]);
	G_free(x);
    }

    return 0;
}


double ***I_alloc_double3(int a, int b, int c)
{
    double ***x;
    int i, n;

    x = (double ***)G_malloc((a + 1) * sizeof(double **));

    for (i = 0; i < a; i++) {
	x[i] = I_alloc_double2(b, c);
	if (x[i] == NULL) {
	    for (n = 0; n < i; n++)
		G_free(x[n]);
	    G_free(x);

	    return (double ***)NULL;
	}
    }
    x[a] = NULL;

    return x;
}


int I_free_double3(double ***x)
{
    int i;

    if (x != NULL) {
	for (i = 0; x[i] != NULL; i++)
	    I_free_double2(x[i]);
	G_free(x);
    }

    return 0;
}
