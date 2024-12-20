#include <grass/glocale.h>

#include "local_proto.h"

static int export_lines_single(struct Map_info *, int, int, int, int,
                               OGRFeatureDefnH, OGRLayerH, struct field_info *,
                               dbDriver *, int, int *, const char **, int, int,
                               int *, int *);
static int export_lines_multi(struct Map_info *, int, int, int, int,
                              OGRFeatureDefnH, OGRLayerH, struct field_info *,
                              dbDriver *, int, int *, const char **, int, int,
                              int *, int *);

static void line_to_polygon(OGRGeometryH, const struct line_pnts *);

static void add_part(OGRGeometryH, OGRwkbGeometryType, int, struct line_pnts *);

static OGRGeometryH build_geometry(struct Map_info *, struct line_pnts *, int,
                                   int, int);

/* export primitives as single/multi-features */
int export_lines_fast(struct Map_info *In, int field, int otype, int multi,
                      int donocat, int force_poly,
                      OGRFeatureDefnH Ogr_featuredefn, OGRLayerH Ogr_layer,
                      struct field_info *Fi, dbDriver *driver, int ncol,
                      int *colctype, const char **colname, int doatt, int nocat,
                      int *n_noatt, int *n_nocat)
{
    if (multi)
        /* export as multi-features */
        return export_lines_multi(In, field, otype, donocat, force_poly,
                                  Ogr_featuredefn, Ogr_layer, Fi, driver, ncol,
                                  colctype, colname, doatt, nocat, n_noatt,
                                  n_nocat);

    /* export as single features */
    return export_lines_single(
        In, field, otype, donocat, force_poly, Ogr_featuredefn, Ogr_layer, Fi,
        driver, ncol, colctype, colname, doatt, nocat, n_noatt, n_nocat);
}

/* export line as single feature */
int export_lines_single(struct Map_info *In, int field, int otype, int donocat,
                        int force_poly, OGRFeatureDefnH Ogr_featuredefn,
                        OGRLayerH Ogr_layer, struct field_info *Fi,
                        dbDriver *driver, int ncol, int *colctype,
                        const char **colname, int doatt, int nocat,
                        int *n_noatt, int *n_nocat)
{
    int i, n_exported;
    int cat, last_cat, db_cat, type;

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
        for (i = 0; i < ncol; i++) {
            if (strcmp(Fi->key, colname[i]) == 0) {
                key_col_index = i;
                break;
            }
        }
    }

    last_cat = -1;
    db_cat = -1;
    for (cat_index = 0; cat_index < n_cats; cat_index++) {

        G_percent(cat_index, n_cats, 5);

        if (!(ci->cat[cat_index][1] & otype))
            continue;

        cat = ci->cat[cat_index][0];
        /* make sure the cidx is ordered by cat */
        if (cat < last_cat)
            G_fatal_error(_("Category index is not sorted ascending by cat!"));
        last_cat = cat;

        i = ci->cat[cat_index][2];

        /* read line */
        type = Vect_read_line(In, Points, Cats, i);
        G_debug(2, "line = %d type = %d", i, type);
        if (!(otype & type)) {
            /* skip lines with different type */
            G_debug(2, "type %d not specified -> skipping", type);
            continue;
        }

        Ogr_geometry = build_geometry(In, Points, type, otype, force_poly);

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

    /* this loop is needed to count features without category in the layer
     * to be exported */
    Vect_rewind(In);
    while (TRUE) {
        type = Vect_read_next_line(In, Points, Cats);
        if (type < 0)
            break;

        Vect_cat_get(Cats, field, &cat);
        if (cat >= 0)
            continue; /* skip features with category */
        if (cat < 0 && !donocat) {
            (*n_nocat)++;
            continue; /* skip lines without category, do not export
                       * not labeled */
        }

        (*n_nocat)++;

        db_cat = -2;
        cat = -1;

        /* code duplicated from above --> */
        Ogr_geometry = build_geometry(In, Points, type, otype, force_poly);

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
        /* <-- code duplicated from above */
    }
    if (doatt && donocat)
        db_close_cursor(&cursor);

    Vect_destroy_line_struct(Points);
    Vect_destroy_cats_struct(Cats);

    return n_exported;
}

/* export line as multi-feature */
int export_lines_multi(struct Map_info *In, int field, int otype, int donocat,
                       int force_poly, OGRFeatureDefnH Ogr_featuredefn,
                       OGRLayerH Ogr_layer, struct field_info *Fi,
                       dbDriver *driver, int ncol, int *colctype,
                       const char **colname, int doatt, int nocat, int *n_noatt,
                       int *n_nocat)
{
    int i, n_exported;
    int cat, last_cat, db_cat, type;
    int line, findex, ipart;

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

    OGRGeometryH Ogr_geometry;
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

    if (donocat)
        G_message(_("Exporting features with category..."));

    /* determine type */
    type = -1; /* unknown -> GeometryCollection */
    if (Vect_cidx_get_num_types_by_index(In, findex) == 1)
        Vect_cidx_get_type_count_by_index(In, findex, 0, &type, NULL);
    if (force_poly)
        wkbtype_part = wkbPolygon;
    else
        wkbtype_part = get_wkbtype(type, otype);
    wkbtype = get_multi_wkbtype(wkbtype_part);

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
            if (ci->cat[cat_index][1] & otype) {
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

            /* read line */
            type = Vect_read_line(In, Points, Cats, line);

            /* check for category consistency */
            Vect_field_cat_get(Cats, field, lcats);
            if (!Vect_val_in_list(lcats, cat))
                G_fatal_error(_("Unable to create multi-feature. "
                                "Category %d not found in line %d, field %d"),
                              cat, line, field);

            /* add part */
            add_part(Ogr_geometry, wkbtype_part, type == GV_LINE && force_poly,
                     Points);
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

    /* check lines without category, if -c flag is given write them as
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

    /* this loop is needed to count features without category in the layer
     * to be exported */
    Vect_rewind(In);
    while (TRUE) {
        type = Vect_read_next_line(In, Points, Cats);
        if (type < 0)
            break;

        Vect_cat_get(Cats, field, &cat);
        if (cat >= 0)
            continue; /* skip features with category */
        if (cat < 0 && !donocat) {
            (*n_nocat)++;
            continue; /* skip lines without category, do not export
                       * not labeled */
        }

        (*n_nocat)++;

        /* add part */
        add_part(Ogr_geometry, wkbtype_part, type == GV_LINE && force_poly,
                 Points);
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

/* build polygon for closed line */
void line_to_polygon(OGRGeometryH Ogr_geometry, const struct line_pnts *Points)
{
    int j;
    OGRGeometryH ring;

    ring = OGR_G_CreateGeometry(wkbLinearRing);

    /* create a ring */
    for (j = 0; j < Points->n_points; j++) {
        OGR_G_AddPoint(ring, Points->x[j], Points->y[j], Points->z[j]);
    }

    /* close ring */
    if (Points->x[Points->n_points - 1] != Points->x[0] ||
        Points->y[Points->n_points - 1] != Points->y[0] ||
        Points->z[Points->n_points - 1] != Points->z[0]) {
        OGR_G_AddPoint(ring, Points->x[0], Points->y[0], Points->z[0]);
    }

    OGR_G_AddGeometryDirectly(Ogr_geometry, ring);
}

void add_part(OGRGeometryH Ogr_geometry, OGRwkbGeometryType wkbtype_part,
              int force_poly, struct line_pnts *Points)
{
    int j;
    OGRGeometryH Ogr_geometry_part;

    Ogr_geometry_part = OGR_G_CreateGeometry(wkbtype_part);
    if (force_poly) {
        line_to_polygon(Ogr_geometry_part, Points);
    }
    else {
        if (OGR_G_GetGeometryType(Ogr_geometry_part) == wkbPoint) {
            /* GV_POINTS -> wkbPoint */
            OGR_G_AddPoint(Ogr_geometry_part, Points->x[0], Points->y[0],
                           Points->z[0]);
        }
        else { /* GV_LINES -> wkbLinestring */
            for (j = 0; j < Points->n_points; j++) {
                OGR_G_AddPoint(Ogr_geometry_part, Points->x[j], Points->y[j],
                               Points->z[j]);
            }
        }
    }
    OGR_G_AddGeometryDirectly(Ogr_geometry, Ogr_geometry_part);
}

static OGRGeometryH build_geometry(struct Map_info *In,
                                   struct line_pnts *Points, int type,
                                   int otype, int force_poly)
{
    OGRGeometryH Ogr_geometry;

    /* build simple features geometry */
    if ((type == GV_LINE && force_poly) || type == GV_FACE) {
        /* lines to polygons
           faces to 2.5D polygons */
        Ogr_geometry = OGR_G_CreateGeometry(wkbPolygon);
        line_to_polygon(Ogr_geometry, Points);
    }
    else {
        Ogr_geometry = OGR_G_CreateGeometry(get_wkbtype(type, otype));
        if (OGR_G_GetGeometryType(Ogr_geometry) == wkbPoint) {
            /* GV_POINTS -> wkbPoint */
            if (Vect_is_3d(In))
                OGR_G_AddPoint(Ogr_geometry, Points->x[0], Points->y[0],
                               Points->z[0]);
            else
                OGR_G_AddPoint_2D(Ogr_geometry, Points->x[0], Points->y[0]);
        }
        else {
            /* GV_LINES -> wkbLinestring */
            int j;
            for (j = 0; j < Points->n_points; j++) {
                if (Vect_is_3d(In))
                    OGR_G_AddPoint(Ogr_geometry, Points->x[j], Points->y[j],
                                   Points->z[j]);
                else
                    OGR_G_AddPoint_2D(Ogr_geometry, Points->x[j], Points->y[j]);
            }
        }
    }

    return Ogr_geometry;
}
