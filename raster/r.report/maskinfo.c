#include <string.h>
#include <grass/gis.h>
#include <grass/raster.h>

static char *append(char *, char *);
static int do_text(char *, long, long);
static int reclass_text(char *, struct Reclass *, int);

char *maskinfo(void)
{
    struct Reclass reclass;
    char *results;
    char text[100];
    int next;
    int first;

    results = NULL;
    if (G_find_raster("MASK", G_mapset()) == NULL)
	return "none";
    if (Rast_get_reclass("MASK", G_mapset(), &reclass) <= 0) {
	sprintf(text, "MASK in %s", G_mapset());
	return append(results, text);
    }

    sprintf(text, "%s in %s", reclass.name, reclass.mapset);
    results = append(results, text);
    next = 0;
    first = 1;
    do {
	next = reclass_text(text, &reclass, next);
	if (*text == 0)
	    break;
	if (first) {
	    first = 0;
	    results = append(results, ", categories");
	}
	results = append(results, " ");
	results = append(results, text);
    }
    while (next >= 0);
    Rast_free_reclass(&reclass);
    return results;
}

static int reclass_text(char *text, struct Reclass *reclass, int next)
{
    int i;
    int n;
    int first;

    *text = 0;

    n = reclass->num;

    first = -1;
    for (i = next; i < n; i++) {
	if (reclass->table[i]) {
	    if (first < 0)
		first = i;
	}
	else if (first >= 0) {
	    do_text(text, (long)(first + reclass->min),
		    (long)(i - 1 + reclass->min));
	    first = -1;
	    if (strlen(text) > 60)
		return i;
	}
    }
    if (first >= 0)
	do_text(text, (long)(first + reclass->min),
		(long)(i - 1 + reclass->min));
    return -1;
}

static int do_text(char *text, long first, long last)
{
    char work[40];

    if (*text)
	strcat(text, " ");

    if (first == last)
	sprintf(work, "%ld", first);
    else
	sprintf(work, "%ld-%ld", first, last);

    strcat(text, work);

    return 0;
}

static char *append(char *results, char *text)
{
    if (results == NULL)
	return G_store(text);
    results = G_realloc(results, strlen(results) + strlen(text) + 1);
    strcat(results, text);
    return results;
}
