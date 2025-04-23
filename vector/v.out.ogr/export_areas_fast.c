#include <grass/glocale.h>

#include "local_proto.h"

static int export_areas_single(struct Map_info *, int, int, OGRFeatureDefnH,
                               OGRLayerH, struct field_info *, dbDriver *, int,
                               int *, const char **, int, int, int *, int *,
                               int);
static int export_areas_multi(struct Map_info *, int, int, OGRFeatureDefnH,
                              OGRLayerH, struct field_info *, dbDriver *, int,
                              int *, const char **, int, int, int *, int *,
                              int);
static OGRGeometryH create_polygon(struct Map_info *, int, struct line_pnts *,
                                   int);

#if 0
/* maybe useful */
static void reverse_points(struct line_pnts *Points)
{
    int i, j, nhalf;
    double tmp;

    nhalf = Points->n_points / 2;

    for (i = 0, j = Points->n_points - 1; i < nhalf; i++, j--) {
        tmp = Points->x[i];
        Points->x[i] = Points->x[j];
        Points->x[j] = tmp;

        tmp = Points->y[i];
        Points->y[i] = Points->y[j];
        Points->y[j] = tmp;

        tmp = Points->z[i];
        Points->z[i] = Points->z[j];
        Points->z[j] = tmp;
    }
}
#endif

/* export areas as single/multi-polygons */
int export_areas_fast(struct Map_info *In, int field, int multi, int donocat,
                      OGRFeatureDefnH Ogr_featuredefn, OGRLayerH Ogr_layer,
                      struct field_info *Fi, dbDriver *driver, int ncol,
                      int *colctype, const char **colname, int doatt, int nocat,
                      int *noatt, int *fout, int outer_ring_ccw)
{
    if (multi)
        /* export as multi-polygons */
        return export_areas_multi(
            In, field, donocat, Ogr_featuredefn, Ogr_layer, Fi, driver, ncol,
            colctype, colname, doatt, nocat, noatt, fout, outer_ring_ccw);

    /* export as polygons */
    return export_areas_single(In, field, donocat, Ogr_featuredefn, Ogr_layer,
                               Fi, driver, ncol, colctype, colname, doatt,
                               nocat, noatt, fout, outer_ring_ccw);
}

int export_areas_single(struct Map_info *In, int field, int donocat,
                        OGRFeatureDefnH Ogr_featuredefn, OGRLayerH Ogr_layer,
                        struct field_info *Fi, dbDriver *driver, int ncol,
                        int *colctype, const char **colname, int doatt,
                        int nocat, int *n_noatt, int *n_nocat,
                        int outer_ring_ccw)
{
    int i;
    int cat, last_cat, db_cat, centroid, area;
    int n_exported;

    struct line_pnts *Points;
    struct line_cats *Cats;

    int findex;
    struct Cat_index *ci;
    int cat_index, n_cats;

    dbString dbstring;
    char buf[SQL_BUFFER_SIZE];
    dbCursor cursor;
    int more;
    int key_col_index;

    OGRGeometryH Ogr_geometry;
    OGRFeatureH Ogr_feature;

    Points = Vect_new_line_struct();
    Cats = Vect_new_cats_struct();

    n_exported = 0;

    /* get category index for given field */
    findex = Vect_cidx_get_field_index(In, field);
    if (findex == -1) {
        G_fatal_error(_("Unable to export multi-features. No category index "
                        "for layer %d."),
                      field);
    }

    ci = &(In->plus.cidx[findex]);
    n_cats = ci->n_cats;

    if (donocat)
        G_message(_("Exporting features with category..."));

    /* select attributes ordered by category value */
    db_init_string(&dbstring);
    sprintf(buf, "SELECT * FROM %s ORDER BY %s ASC", Fi->table, Fi->key);
    G_debug(2, "SQL: %s", buf);
    db_set_string(&dbstring, buf);
    if (db_open_select_cursor(driver, &dbstring, &cursor, DB_SEQUENTIAL) !=
        DB_OK) {
        G_fatal_error(_("Cannot select attributes sorted by %s"), Fi->key);
    }

    if (db_fetch(&cursor, DB_NEXT, &more) != DB_OK)
        G_fatal_error(_("Unable to fetch data from table"));

    /* get index of key column */
    key_col_index = -1;
    for (i = 0; i < ncol; i++) {
        if (strcmp(Fi->key, colname[i]) == 0) {
            key_col_index = i;
            break;
        }
    }

    last_cat = -1;
    db_cat = -1;
    for (cat_index = 0; cat_index < n_cats; cat_index++) {

        G_percent(cat_index, n_cats, 5);

        /* get area's category */
        if (!(ci->cat[cat_index][1] & GV_CENTROID))
            continue;

        cat = ci->cat[cat_index][0];
        /* make sure the cidx is ordered by cat */
        if (cat < last_cat)
            G_fatal_error(_("Category index is not sorted ascending by cat!"));
        last_cat = cat;

        centroid = ci->cat[cat_index][2];

        area = Vect_get_centroid_area(In, centroid);

        if (area < 1) {
            /* centroid not in area or duplicate centroid */
            continue;
        }

        /* create polygon from area */
        Ogr_geometry = create_polygon(In, area, Points, outer_ring_ccw);

        /* add feature */
        Ogr_feature = OGR_F_Create(Ogr_featuredefn);
        OGR_F_SetGeometry(Ogr_feature, Ogr_geometry);
        /* get attributes */
        mk_att_fast(cat, Fi, ncol, colctype, colname, doatt, nocat, Ogr_feature,
                    n_noatt, &cursor, &more, &db_cat, key_col_index);
        if (OGR_L_CreateFeature(Ogr_layer, Ogr_feature) != OGRERR_NONE) {
            G_fatal_error(_("Failed to create OGR feature"));
        }
        else
            n_exported++;

        OGR_F_Destroy(Ogr_feature);
        OGR_G_DestroyGeometry(Ogr_geometry);
    }

    if (donocat)
        G_message(_("Exporting features without category..."));

    if (doatt) {
        db_close_cursor(&cursor);
        if (donocat) {
            cat = -1;
            if (db_open_select_cursor(driver, &dbstring, &cursor,
                                      DB_SEQUENTIAL) != DB_OK) {
                G_fatal_error(_("Cannot select attributes for cat = %d"), cat);
            }
            if (db_fetch(&cursor, DB_NEXT, &more) != DB_OK)
                G_fatal_error(_("Unable to fetch data from table"));
        }
    }

    for (area = 1; area <= Vect_get_num_areas(In); area++) {
        centroid = Vect_get_area_centroid(In, area);
        /* skip areas without centroid */
        if (centroid == 0)
            continue;

        /* get areas's category */
        Vect_get_area_cats(In, area, Cats);
        Vect_cat_get(Cats, field, &cat);
        /* skip areas with category */
        if (cat >= 0)
            continue;
        /* skip areas without category, do not export not labeled */
        if (cat < 0 && !donocat) {
            (*n_nocat)++;
            continue;
        }

        (*n_nocat)++;

        /* create polygon from area */
        Ogr_geometry = create_polygon(In, area, Points, outer_ring_ccw);

        /* add feature */
        Ogr_feature = OGR_F_Create(Ogr_featuredefn);
        OGR_F_SetGeometry(Ogr_feature, Ogr_geometry);
        /* no attributes for features without category */
        cat = -1;
        db_cat = -2;
        mk_att_fast(cat, Fi, ncol, colctype, colname, doatt, nocat, Ogr_feature,
                    n_noatt, &cursor, &more, &db_cat, key_col_index);
        if (OGR_L_CreateFeature(Ogr_layer, Ogr_feature) != OGRERR_NONE) {
            G_fatal_error(_("Failed to create OGR feature"));
        }
        else
            n_exported++;

        OGR_F_Destroy(Ogr_feature);
        OGR_G_DestroyGeometry(Ogr_geometry);
    }

    if (donocat && doatt)
        db_close_cursor(&cursor);

    Vect_destroy_line_struct(Points);

    return n_exported;
}

int export_areas_multi(struct Map_info *In, int field, int donocat,
                       OGRFeatureDefnH Ogr_featuredefn, OGRLayerH Ogr_layer,
                       struct field_info *Fi, dbDriver *driver, int ncol,
                       int *colctype, const char **colname, int doatt,
                       int nocat, int *n_noatt, int *n_nocat,
                       int outer_ring_ccw)
{
    int i, n_exported, area, centroid;
    int cat, last_cat, db_cat, line, findex, ipart;

    struct line_pnts *Points;
    struct line_cats *Cats;
    struct ilist *line_list, *lcats;

    struct Cat_index *ci;
    int cat_index, n_cats;

    dbString dbstring;
    char buf[SQL_BUFFER_SIZE];
    dbCursor cursor;
    int more;
    int key_col_index;

    OGRGeometryH Ogr_geometry, Ogr_geometry_part;
    OGRFeatureH Ogr_feature;
    OGRwkbGeometryType wkbtype, wkbtype_part;

    Points = Vect_new_line_struct();
    Cats = Vect_new_cats_struct();
    line_list = Vect_new_list();
    lcats = Vect_new_list();

    n_exported = 0;

    /* check if category index is available for given field */
    findex = Vect_cidx_get_field_index(In, field);
    if (findex == -1) {
        G_fatal_error(_("Unable to export multi-features. No category index "
                        "for layer %d."),
                      field);
    }

    ci = &(In->plus.cidx[findex]);
    n_cats = ci->n_cats;

    /* determine type */
    wkbtype_part = wkbPolygon;
    wkbtype = get_multi_wkbtype(wkbtype_part);

    if (donocat)
        G_message(_("Exporting features with category..."));

    key_col_index = -1;
    more = 1;
    if (doatt) {
        /* select attributes ordered by category value */
        db_init_string(&dbstring);
        sprintf(buf, "SELECT * FROM %s ORDER BY %s ASC", Fi->table, Fi->key);
        G_debug(2, "SQL: %s", buf);
        db_set_string(&dbstring, buf);
        if (db_open_select_cursor(driver, &dbstring, &cursor, DB_SEQUENTIAL) !=
            DB_OK) {
            G_fatal_error(_("Cannot select attributes sorted by %s"), Fi->key);
        }

        if (db_fetch(&cursor, DB_NEXT, &more) != DB_OK)
            G_fatal_error(_("Unable to fetch data from table"));

        /* get index of key column */
        key_col_index = -1;
        for (i = 0; i < ncol; i++) {
            if (strcmp(Fi->key, colname[i]) == 0) {
                key_col_index = i;
                break;
            }
        }
    }

    last_cat = -1;
    db_cat = -1;
    cat_index = 0;
    while (cat_index < n_cats) {

        G_percent(cat_index, n_cats, 5);

        cat = ci->cat[cat_index][0];

        /* make sure the cidx is ordered by cat */
        if (cat < last_cat)
            G_fatal_error(_("Category index is not sorted ascending by cat!"));
        last_cat = cat;

        /* collect all features with current cat */
        Vect_reset_list(line_list);
        while (cat_index < n_cats && ci->cat[cat_index][0] == cat) {
            if (ci->cat[cat_index][1] & GV_CENTROID) {
                Vect_list_append(line_list, ci->cat[cat_index][2]);
            }
            cat_index++;
        }

        /* create multi-feature */
        Ogr_geometry = OGR_G_CreateGeometry(wkbtype);

        /* build simple features geometry, go through all parts */
        for (ipart = 0; ipart < line_list->n_values; ipart++) {
            line = line_list->value[ipart];
            G_debug(3, "cat=%d, line=%d -> part=%d", cat, line, ipart);

            /* get centroid's category */
            Vect_read_line(In, NULL, Cats, line);
            /* check for category consistency */
            Vect_field_cat_get(Cats, field, lcats);
            if (!Vect_val_in_list(lcats, cat))
                G_fatal_error(_("Unable to create multi-feature. "
                                "Category %d not found in line %d, field %d"),
                              cat, line, field);

            /* find corresponding area */
            area = Vect_get_centroid_area(In, line);
            if (area <= 0)
                continue;

            /* create polygon from area */
            Ogr_geometry_part =
                create_polygon(In, area, Points, outer_ring_ccw);

            /* add part */
            OGR_G_AddGeometryDirectly(Ogr_geometry, Ogr_geometry_part);
        }

        if (!OGR_G_IsEmpty(Ogr_geometry)) {
            /* write multi-feature */
            Ogr_feature = OGR_F_Create(Ogr_featuredefn);
            OGR_F_SetGeometry(Ogr_feature, Ogr_geometry);
            /* get attributes */
            mk_att_fast(cat, Fi, ncol, colctype, colname, doatt, nocat,
                        Ogr_feature, n_noatt, &cursor, &more, &db_cat,
                        key_col_index);
            if (OGR_L_CreateFeature(Ogr_layer, Ogr_feature) != OGRERR_NONE) {
                G_fatal_error(_("Failed to create OGR feature"));
            }
            else
                n_exported++;

            OGR_F_Destroy(Ogr_feature);
        }
        else {
            /* skip empty features */
            G_debug(3, "multi-feature is empty -> skipped");
        }

        OGR_G_DestroyGeometry(Ogr_geometry);
    }

    if (donocat)
        G_message(_("Exporting features without category..."));

    /* check areas without category, if -c flag is given write them as
     * one multi-feature */
    Ogr_geometry = OGR_G_CreateGeometry(wkbtype);

    if (doatt) {
        db_close_cursor(&cursor);
        if (donocat) {
            cat = -1;
            if (db_open_select_cursor(driver, &dbstring, &cursor,
                                      DB_SEQUENTIAL) != DB_OK) {
                G_fatal_error(_("Cannot select attributes for cat = %d"), cat);
            }
            if (db_fetch(&cursor, DB_NEXT, &more) != DB_OK)
                G_fatal_error(_("Unable to fetch data from table"));
        }
    }

    for (area = 1; area <= Vect_get_num_areas(In); area++) {
        centroid = Vect_get_area_centroid(In, area);
        /* skip areas without centroid */
        if (centroid == 0)
            continue;

        /* get areas's category */
        Vect_get_area_cats(In, area, Cats);
        Vect_cat_get(Cats, field, &cat);
        /* skip areas with category */
        if (cat >= 0)
            continue;
        /* skip areas without category, do not export not labeled */
        if (cat < 0 && !donocat) {
            (*n_nocat)++;
            continue;
        }

        /* create polygon from area */
        Ogr_geometry_part = create_polygon(In, area, Points, outer_ring_ccw);

        /* add part */
        OGR_G_AddGeometryDirectly(Ogr_geometry, Ogr_geometry_part);

        (*n_nocat)++;
    }

    if (!OGR_G_IsEmpty(Ogr_geometry)) {
        /* write multi-feature */
        Ogr_feature = OGR_F_Create(Ogr_featuredefn);
        OGR_F_SetGeometry(Ogr_feature, Ogr_geometry);
        /* no attributes for features without category */
        cat = -1;
        db_cat = -2;
        mk_att_fast(cat, Fi, ncol, colctype, colname, doatt, nocat, Ogr_feature,
                    n_noatt, &cursor, &more, &db_cat, key_col_index);
        if (OGR_L_CreateFeature(Ogr_layer, Ogr_feature) != OGRERR_NONE) {
            G_fatal_error(_("Failed to create OGR feature"));
        }
        else
            n_exported++;

        OGR_F_Destroy(Ogr_feature);
    }
    else {
        /* skip empty features */
        G_debug(3, "multi-feature is empty -> skipped");
    }

    OGR_G_DestroyGeometry(Ogr_geometry);

    if (donocat && doatt)
        db_close_cursor(&cursor);

    Vect_destroy_line_struct(Points);
    Vect_destroy_cats_struct(Cats);
    Vect_destroy_list(line_list);
    Vect_destroy_list(lcats);

    return n_exported;
}

OGRGeometryH create_polygon(struct Map_info *In, int area,
                            struct line_pnts *Points, int outer_ring_ccw)
{
    int j, k;
    OGRGeometryH Ogr_geometry, ring;

    Vect_get_area_points(In, area, Points);

    Ogr_geometry = OGR_G_CreateGeometry(wkbPolygon);
    ring = OGR_G_CreateGeometry(wkbLinearRing);

    /* Area */
    if (Vect_is_3d(In)) {
        if (outer_ring_ccw) {
            for (j = Points->n_points - 1; j >= 0; j--)
                OGR_G_AddPoint(ring, Points->x[j], Points->y[j], Points->z[j]);
        }
        else {
            for (j = 0; j < Points->n_points; j++)
                OGR_G_AddPoint(ring, Points->x[j], Points->y[j], Points->z[j]);
        }
    }
    else {
        if (outer_ring_ccw) {
            for (j = Points->n_points - 1; j >= 0; j--)
                OGR_G_AddPoint_2D(ring, Points->x[j], Points->y[j]);
        }
        else {
            for (j = 0; j < Points->n_points; j++)
                OGR_G_AddPoint_2D(ring, Points->x[j], Points->y[j]);
        }
    }

    OGR_G_AddGeometryDirectly(Ogr_geometry, ring);

    /* Isles */
    for (k = 0; k < Vect_get_area_num_isles(In, area); k++) {
        Vect_get_isle_points(In, Vect_get_area_isle(In, area, k), Points);
        ring = OGR_G_CreateGeometry(wkbLinearRing);
        if (Vect_is_3d(In)) {
            if (outer_ring_ccw) {
                for (j = Points->n_points - 1; j >= 0; j--)
                    OGR_G_AddPoint(ring, Points->x[j], Points->y[j],
                                   Points->z[j]);
            }
            else {
                for (j = 0; j < Points->n_points; j++)
                    OGR_G_AddPoint(ring, Points->x[j], Points->y[j],
                                   Points->z[j]);
            }
        }
        else {
            if (outer_ring_ccw) {
                for (j = Points->n_points - 1; j >= 0; j--)
                    OGR_G_AddPoint_2D(ring, Points->x[j], Points->y[j]);
            }
            else {
                for (j = 0; j < Points->n_points; j++)
                    OGR_G_AddPoint_2D(ring, Points->x[j], Points->y[j]);
            }
        }
        OGR_G_AddGeometryDirectly(Ogr_geometry, ring);
    }

    return Ogr_geometry;
}
