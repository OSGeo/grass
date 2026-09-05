#include <string.h>

#include <grass/gis.h>
#include <grass/raster.h>

#include "global.h"

static char *append(char *results, char *text);
static void do_text(char *text, long first, long last);
static int reclass_text(char *text, struct Reclass *reclass, int next);

char *maskinfo(void)
{
    struct Reclass reclass = {0};
    char *results;
    char text[2 * GNAME_MAX + GMAPSET_MAX];
    int next;
    int first;
    char mask_name[GNAME_MAX];
    char mask_mapset[GMAPSET_MAX];

    results = NULL;
    if (!Rast_mask_status(mask_name, mask_mapset, NULL, NULL, NULL))
        return G_store("none");
    if (Rast_get_reclass(mask_name, mask_mapset, &reclass) <= 0) {
        snprintf(text, sizeof(text), "%s in %s", mask_name, mask_mapset);
        Rast_free_reclass(&reclass);
        return append(results, text);
    }

    snprintf(text, sizeof(text), "%s in %s", reclass.name, reclass.mapset);
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
    } while (next >= 0);
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

static void do_text(char *text, long first, long last)
{
    char work[40];

    if (*text)
        strcat(text, " ");

    if (first == last)
        snprintf(work, sizeof(work), "%ld", first);
    else
        snprintf(work, sizeof(work), "%ld-%ld", first, last);

    strcat(text, work);
}

static char *append(char *results, char *text)
{
    if (results == NULL)
        return G_store(text);
    results = G_realloc(results, strlen(results) + strlen(text) + 1);
    strcat(results, text);
    return results;
}
