#include "global.h"
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/raster3d.h>
#include <grass/vector.h>
#include <grass/manage.h>
#include <grass/glocale.h>

static int region_overlaps(const struct Cell_head *, const char *, const char *,
                           int);
static int compare_elist(const void *, const void *);

void make_list(struct elist **el, int *lcount, int *lalloc,
               const struct list *elem, const char *mapset,
               const struct Cell_head *window)
{
    char path[GPATH_MAX];
    const char *element, *alias;
    char **list;
    int count, i;
    int type;

    element = elem->element[0];
    alias = elem->alias;

    G_file_name(path, element, "", mapset);
    if (access(path, 0) != 0)
        return;

    if ((list = G_ls2(path, &count)) == NULL)
        return;

    if (strcmp(alias, "raster") == 0)
        type = TYPE_RAST;
    else if (strcmp(alias, "raster_3d") == 0)
        type = TYPE_RAST3D;
    else if (strcmp(alias, "vector") == 0)
        type = TYPE_VECT;
    else
        type = TYPE_OTHERS;

    /* Suppress "... found in more mapsets" warnings from G_find_file2. */
    G_suppress_warnings(1);

    if (*lcount + count > *lalloc) {
        *lalloc = *lcount + count + 10;
        *el = G_realloc(*el, *lalloc * sizeof(struct elist));
    }

    for (i = 0; i < count; i++) {

        /* If region= is used, read the map region. */
        if (window) {
            /* If the map region doesn't overlap with the input region, don't
             * print the map. */
            if (!region_overlaps(window, list[i], mapset, type))
                continue;
        }

        (*el)[*lcount].type = G_store(alias);
        (*el)[*lcount].name = list[i];
        (*el)[*lcount].mapset = G_store(mapset);
        (*lcount)++;
    }

    G_suppress_warnings(0);

    G_free(list);
}

void print_list(FILE *fp, struct elist *el, int count, const char *separator,
                int add_type, int add_mapset, enum OutputFormat format)
{
    int i;
    char fullname[1024];
    JSON_Array *root_array = NULL;
    JSON_Value *root_value = NULL;
    JSON_Object *map_object = NULL;
    JSON_Value *map_value = NULL;

    if (!count)
        return;

    if (format == JSON) {
        root_value = G_json_value_init_array();
        if (root_value == NULL) {
            G_fatal_error(_("Failed to initialize JSON array. Out of memory?"));
        }
        root_array = G_json_array(root_value);
    }

    qsort(el, count, sizeof(struct elist), compare_elist);

    for (i = 0; i < count; i++) {
        int need_mapset = 0;

        if (format == SHELL) {
            if (i != 0)
                fprintf(fp, "%s", separator);

            if (add_type)
                fprintf(fp, "%s/", el[i].type);

            fprintf(fp, "%s", el[i].name);

            if (!add_mapset) {
                if (i + 1 < count)
                    need_mapset = strcmp(el[i].name, el[i + 1].name) == 0;
                if (!need_mapset && i > 0)
                    need_mapset = strcmp(el[i].name, el[i - 1].name) == 0;
            }
            if (add_mapset || need_mapset)
                fprintf(fp, "@%s", el[i].mapset);
        }
        else {
            map_value = G_json_value_init_object();
            if (map_value == NULL) {
                G_fatal_error(
                    _("Failed to initialize JSON object. Out of memory?"));
            }
            map_object = G_json_object(map_value);
            snprintf(fullname, sizeof(fullname), "%s@%s", el[i].name,
                     el[i].mapset);

            G_json_object_set_string(map_object, "name", el[i].name);
            G_json_object_set_string(map_object, "mapset", el[i].mapset);
            G_json_object_set_string(map_object, "type", el[i].type);
            G_json_object_set_string(map_object, "fullname", fullname);
            G_json_array_append_value(root_array, map_value);
        }
    }

    if (format == JSON) {
        char *json_string = G_json_serialize_to_string_pretty(root_value);
        if (!json_string) {
            G_json_value_free(root_value);
            G_fatal_error(_("Failed to serialize JSON to pretty format."));
        }

        fputs(json_string, fp);

        G_json_free_serialized_string(json_string);
        G_json_value_free(root_value);
    }

    fflush(fp);
}

static int region_overlaps(const struct Cell_head *window, const char *name,
                           const char *mapset, int type)
{
    int has_region;
    struct Cell_head map_window;
    RASTER3D_Region region3d;
    struct Map_info Map;
    struct bound_box box;

    switch (type) {
    case TYPE_RAST:
        Rast_get_cellhd(name, mapset, &map_window);
        has_region = 1;
        break;
    case TYPE_RAST3D:
        if (Rast3d_read_region_map(name, mapset, &region3d) < 0)
            G_fatal_error(_("Unable to read header of 3D raster map <%s@%s>"),
                          name, mapset);
        Rast3d_region_to_cell_head(&region3d, &map_window);
        has_region = 1;
        break;
    case TYPE_VECT:
        Vect_set_open_level(2);
        if (Vect_open_old_head(&Map, name, mapset) < 2)
            G_fatal_error(
                _("Unable to open vector map <%s@%s> on topological level"),
                name, mapset);
        Vect_get_map_box(&Map, &box);
        Vect_close(&Map);

        map_window.north = box.N;
        map_window.south = box.S;
        map_window.west = box.W;
        map_window.east = box.E;
        has_region = 1;
        break;
    default:
        has_region = 0;
        break;
    }

    /* If an element doesn't have a concept of region at all, return 1 so we
     * can always print it. */
    if (!has_region)
        return 1;

    /* If the map region is outside the input region, return 0. Otherwise
     * return 1 */
    return !(window->north <= map_window.south ||
             window->south >= map_window.north ||
             window->west >= map_window.east ||
             window->east <= map_window.west);
}

static int compare_elist(const void *a, const void *b)
{
    struct elist *al = (struct elist *)a;
    struct elist *bl = (struct elist *)b;
    int ret;

    if (!(ret = strcmp(al->type, bl->type))) {
        if (!(ret = strcmp(al->name, bl->name)))
            ret = strcmp(al->mapset, bl->mapset);
    }

    return ret;
}
