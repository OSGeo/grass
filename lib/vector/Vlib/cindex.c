/*!
   \file lib/vector/Vlib/cindex.c

   \brief Vector library - category index management

   Higher level functions for reading/writing/manipulating vectors.

   (C) 2001-2013 by the GRASS Development Team

   This program is free software under the GNU General Public License
   (>=v2). Read the file COPYING that comes with GRASS for details.

   \author Radim Blazek
   \author Some contribution by Martin Landa <landa.martin gmail.com>
 */

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <grass/vector.h>
#include <grass/glocale.h>

#include "local_proto.h"

#define SEP                                                                    \
    "------------------------------------------------------------------------" \
    "------------------\n"

static void check_status(struct Map_info *Map)
{
    if (!Map->plus.cidx_up_to_date)
        G_fatal_error(_("Category index is not up to date"));
}

static void check_index(struct Map_info *Map, int index)
{
    if (index < 0 || index >= Map->plus.n_cidx)
        G_fatal_error(_("Layer index out of range"));
}

/* search for first occurrence of cat in cat index, starting at first */
static int ci_search_cat(struct Cat_index *ci, int first, int cat)
{
    int lo, hi, mid;

    lo = first;
    if (lo < 0)
        lo = 0;
    if (ci->cat[lo][0] > cat)
        return -1;
    if (ci->cat[lo][0] == cat)
        return lo;

    hi = ci->n_cats - 1;
    if (first > hi)
        return -1;

    /* deferred test for equality */
    while (lo < hi) {
        mid = (lo + hi) >> 1;
        if (ci->cat[mid][0] < cat)
            lo = mid + 1;
        else
            hi = mid;
    }
    if (ci->cat[lo][0] == cat)
        return lo;

    return -1;
}

/*!
   \brief Get number of layers in category index

   \param Map pointer to Map_info structure

   \return number of layers
 */
int Vect_cidx_get_num_fields(struct Map_info *Map)
{
    check_status(Map);

    return Map->plus.n_cidx;
}

/*!
   \brief Get layer number for given index

   G_fatal_error() is called when index not found.

   \param Map pointer to Map_info structure
   \param index layer index: from 0 to Vect_cidx_get_num_fields() - 1

   \return layer number
 */
int Vect_cidx_get_field_number(struct Map_info *Map, int index)
{
    check_status(Map);
    check_index(Map, index);

    return Map->plus.cidx[index].field;
}

/*!
   \brief Get layer index for given layer number

   \param Map pointer to Map_info structure
   \param field layer number

   \return layer index
   \return -1 if not found
 */
int Vect_cidx_get_field_index(struct Map_info *Map, int field)
{
    int i;
    const struct Plus_head *Plus;

    G_debug(2, "Vect_cidx_get_field_index() field = %d", field);

    check_status(Map);
    Plus = &(Map->plus);

    for (i = 0; i < Plus->n_cidx; i++) {
        if (Plus->cidx[i].field == field)
            return i;
    }

    return -1;
}

/*!
   \brief Get number of unique categories for given layer index

   G_fatal_error() is called when index not found.

   \param Map pointer to Map_info structure
   \param index layer index (starts at 0)

   \return number of unique categories
   \return -1 on error
 */
int Vect_cidx_get_num_unique_cats_by_index(struct Map_info *Map, int index)
{
    check_status(Map);
    check_index(Map, index);

    return Map->plus.cidx[index].n_ucats;
}

/*!
   \brief Get number of categories for given layer index

   \param Map pointer to Map_info structure
   \param index layer index

   \return number of categories
   \return -1 on error
 */
int Vect_cidx_get_num_cats_by_index(struct Map_info *Map, int index)
{
    check_status(Map);
    check_index(Map, index);

    return Map->plus.cidx[index].n_cats;
}

/*!
   \brief Get number of feature types for given layer index

   G_fatal_error() is called when index not found.

   \param Map pointer to Map_info structure
   \param field_index layer index

   \return number of feature types
   \return -1 on error
 */
int Vect_cidx_get_num_types_by_index(struct Map_info *Map, int field_index)
{
    check_status(Map);
    check_index(Map, field_index);

    return Map->plus.cidx[field_index].n_types;
}

/*!
   \brief Get count of feature types for given field and type index

   \param Map pointer to Map_info structure
   \param field_index layer index
   \param type_index type index
   \param[out] type feature type (GV_POINT, ...)
   \param[out] count number of features or NULL

   \return 1 on success
   \return 0 on error
 */
int Vect_cidx_get_type_count_by_index(struct Map_info *Map, int field_index,
                                      int type_index, int *type, int *count)
{
    check_status(Map);
    check_index(Map, field_index);

    *type = Map->plus.cidx[field_index].type[type_index][0];
    if (count)
        *count = Map->plus.cidx[field_index].type[type_index][1];

    return 1;
}

/*!
   \brief Get count of features of certain type by layer and type

   \param Map pointer to Map_info structure
   \param field layer number
   \param type feature type

   \return feature count
   \return 0 if no features, no such field or no such type in category index
 */
int Vect_cidx_get_type_count(struct Map_info *Map, int field, int type)
{
    int i, fi, count = 0;

    G_debug(3, "Vect_cidx_get_type_count() field = %d, type = %d", field, type);

    check_status(Map);

    if ((fi = Vect_cidx_get_field_index(Map, field)) < 0)
        return 0; /* field not found */
    G_debug(3, "field_index = %d", fi);

    G_debug(3, "ntypes = %d", Map->plus.cidx[fi].n_types);
    for (i = 0; i < Map->plus.cidx[fi].n_types; i++) {
        int tp, cnt;

        tp = Map->plus.cidx[fi].type[i][0];
        cnt = Map->plus.cidx[fi].type[i][1];
        if (tp & type)
            count += cnt;
        G_debug(3, "%d tp = %d, cnt= %d count = %d", i, tp, cnt, count);
    }

    return count;
}

/*!
   \brief Get category, feature type and id for given layer and category index

   \param Map pointer to Map_info structure
   \param field_index layer index
   \param cat_index category index
   \param[out] cat category number
   \param[out] type feature type
   \param[out] id feature id

   \return 1 on success
   \return 0 on error
 */
int Vect_cidx_get_cat_by_index(struct Map_info *Map, int field_index,
                               int cat_index, int *cat, int *type, int *id)
{
    check_status(Map); /* This check is slow ? */
    check_index(Map, field_index);

    if (cat_index < 0 || cat_index >= Map->plus.cidx[field_index].n_cats)
        G_fatal_error(_("Category index out of range"));

    *cat = Map->plus.cidx[field_index].cat[cat_index][0];
    *type = Map->plus.cidx[field_index].cat[cat_index][1];
    *id = Map->plus.cidx[field_index].cat[cat_index][2];

    return 1;
}

/*!
   \brief Get list of unique categories for given layer index

   \param Map pointer to Map_info structure
   \param field_index layer index
   \param[out] list output list of cats

   \return 1 on success
   \return 0 on error
 */
int Vect_cidx_get_unique_cats_by_index(struct Map_info *Map, int field_index,
                                       struct ilist *list)
{
    int c;
    struct Cat_index *ci;

    check_status(Map);
    check_index(Map, field_index);

    ci = &(Map->plus.cidx[field_index]);

    /* force sorting index -- really needed? */
    dig_cidx_sort(&(Map->plus));

    Vect_reset_list(list);
    if (ci->n_cats > 0)
        Vect_list_append(list, ci->cat[0][0]);
    for (c = 1; c < ci->n_cats; c++) {
        if (ci->cat[c][0] != ci->cat[c - 1][0])
            Vect_list_append(list, ci->cat[c][0]);
    }

    return list->n_values == ci->n_ucats ? 1 : 0;
}

/*!
   \brief Find next line/area id for given category, start_index and type_mask

   \param Map pointer to Map_info structure
   \param field_index layer index
   \param cat category number
   \param type_mask requested feature type
   \param start_index start search at this index (0 - whole category index)
   \param[out] type returned type
   \param[out] id returned line/area id

   \return index to array
   \return -1 not found
 */
int Vect_cidx_find_next(struct Map_info *Map, int field_index, int cat,
                        int type_mask, int start_index, int *type, int *id)
{
    int cat_index;
    struct Cat_index *ci;

    G_debug(3,
            "Vect_cidx_find_next() cat = %d, type_mask = %d, start_index = %d",
            cat, type_mask, start_index);

    check_status(Map); /* This check is slow ? */
    check_index(Map, field_index);
    *type = *id = 0;

    /* pointer to category index */
    ci = &(Map->plus.cidx[field_index]);

    cat_index = ci_search_cat(ci, start_index, cat);
    G_debug(3, "cat_index = %d", cat_index);

    if (cat_index < 0)
        return -1;

    do {
        G_debug(3, "  cat_index = %d", cat_index);
        if (ci->cat[cat_index][0] == cat && ci->cat[cat_index][1] & type_mask) {
            *type = ci->cat[cat_index][1];
            *id = ci->cat[cat_index][2];
            G_debug(3, "  type match -> record found");
            return cat_index;
        }
        cat_index++;
    } while (cat_index < ci->n_cats);

    return -1;
}

/*!
   \brief Find all line/area id's for given category

   \param Map pointer to Map_info structure
   \param layer layer number
   \param type_mask feature type of objects to search for
   \param cat category number
   \param[out] lines array of ids of found lines/points
 */
void Vect_cidx_find_all(struct Map_info *Map, int layer, int type_mask, int cat,
                        struct ilist *lines)
{
    int type, line;
    struct Cat_index *ci;
    int field_index, idx;

    Vect_reset_list(lines);
    field_index = Vect_cidx_get_field_index(Map, layer);

    if (field_index == -1) {
        /* not found */
        return;
    }
    ci = &(Map->plus.cidx[field_index]);

    if ((type_mask & GV_AREA) && type_mask != GV_AREA)
        G_fatal_error(_("Mixing IDs of areas and primitives"));

    idx =
        Vect_cidx_find_next(Map, field_index, cat, type_mask, 0, &type, &line);

    if (idx == -1) {
        return;
    }

    do {
        if (ci->cat[idx][0] != cat) {
            break;
        }
        if (ci->cat[idx][1] & type_mask) {
            Vect_list_append(lines, ci->cat[idx][2]);
        }
        idx++;
    } while (idx < ci->n_cats);
    return;
}

/*!
   \brief Write (dump) category index in text form to file

   \param Map pointer to Map_info structure
   \param[out] out output file

   \return 1 on success
   \return 0 on error
 */
int Vect_cidx_dump(struct Map_info *Map, FILE *out)
{
    int i, field, nfields, ntypes;

    G_debug(2, "Vect_cidx_dump()");

    check_status(Map);

    nfields = Vect_cidx_get_num_fields(Map);
    fprintf(out,
            "---------- CATEGORY INDEX DUMP: Number of layers: %d "
            "--------------------------------------\n",
            nfields);

    for (i = 0; i < nfields; i++) {
        int j, nucats, ncats;

        field = Vect_cidx_get_field_number(Map, i);
        nucats = Vect_cidx_get_num_unique_cats_by_index(Map, i);
        ncats = Vect_cidx_get_num_cats_by_index(Map, i);
        ntypes = Vect_cidx_get_num_types_by_index(Map, i);

        fprintf(out,
                "Layer %6d  number of unique cats: %7d  number of "
                "cats: %7d  number of types: %d\n",
                field, nucats, ncats, ntypes);
        fprintf(out, SEP);

        fprintf(out, "            type |     count\n");
        for (j = 0; j < ntypes; j++) {
            int type, count;

            Vect_cidx_get_type_count_by_index(Map, i, j, &type, &count);
            fprintf(out, "           %5d | %9d\n", type, count);
        }

        fprintf(out, " category | type | line/area\n");
        for (j = 0; j < ncats; j++) {
            int cat, type, id;

            Vect_cidx_get_cat_by_index(Map, i, j, &cat, &type, &id);
            fprintf(out, "%9d | %4d | %9d\n", cat, type, id);
        }

        fprintf(out, SEP);
    }

    return 1;
}

/*!
   \brief Save category index to binary file (cidx)

   \param Map pointer to Map_info structure

   \return 0 on success
   \return 1 on error
 */
int Vect_cidx_save(struct Map_info *Map)
{
    struct Plus_head *plus;
    char path[GPATH_MAX];
    struct gvfile fp;

    G_debug(2, "Vect_cidx_save()");
    check_status(Map);

    plus = &(Map->plus);

    dig_file_init(&fp);

    Vect__get_path(path, Map);
    fp.file = G_fopen_new(path, GV_CIDX_ELEMENT);
    if (fp.file == NULL) {
        G_warning(_("Unable to create category index file for vector map <%s>"),
                  Vect_get_name(Map));
        return 1;
    }

    /* set portable info */
    dig_init_portable(&(plus->cidx_port), dig__byte_order_out());

    if (0 > dig_write_cidx(&fp, plus)) {
        G_warning(_("Error writing out category index file"));
        fclose(fp.file);
        return 1;
    }

    fclose(fp.file);

    return 0;
}

/*!
   \brief Read category index from cidx file if exists

   \param Map pointer to Map_info structure
   \param head_only read only header of the file

   \return 0 on success
   \return 1 if file does not exist
   \return -1 error, file exists but cannot be read
 */
int Vect_cidx_open(struct Map_info *Map, int head_only)
{
    int ret;
    char file_path[GPATH_MAX], path[GPATH_MAX];
    struct gvfile fp;
    struct Plus_head *Plus;

    G_debug(2, "Vect_cidx_open(): name = %s mapset= %s", Map->name,
            Map->mapset);

    Plus = &(Map->plus);

    Vect__get_path(path, Map);
    Vect__get_element_path(file_path, Map, GV_CIDX_ELEMENT);

    if (access(file_path, F_OK) != 0) { /* does not exist */
        return 1;
    }

    dig_file_init(&fp);
    fp.file = G_fopen_old(path, GV_CIDX_ELEMENT, Map->mapset);

    if (fp.file == NULL) { /* category index file is not available */
        const char *map_name = Vect_get_full_name(Map);
        G_warning(_("Unable to open category index file for vector map <%s>"),
                  map_name);
        G_free((void *)map_name);
        return -1;
    }

    /* load category index to memory */
    ret = dig_read_cidx(&fp, Plus, head_only);

    fclose(fp.file);

    if (ret == 1) {
        G_debug(3, "Cannot read cidx");
        return -1;
    }

    return 0;
}
