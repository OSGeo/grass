#include <grass/glocale.h>

#include "local_proto.h"

static int export_areas_single(struct Map_info *, int, int, 
                               OGRFeatureDefnH, OGRLayerH,
                               struct field_info *, dbDriver *, int, int *, 
                               const char **, int, int,
                               int *, int *);
static int export_areas_multi(struct Map_info *, int, int, 
                              OGRFeatureDefnH, OGRLayerH,
                              struct field_info *, dbDriver *, int, int *, 
                              const char **, int, int,
                              int *, int *);
static OGRGeometryH create_polygon(struct Map_info *, int, struct line_pnts *);

/* export areas as single/multi-polygons */
int export_areas(struct Map_info *In, int field, int multi, int donocat,
                 OGRFeatureDefnH Ogr_featuredefn,OGRLayerH Ogr_layer,
                 struct field_info *Fi, dbDriver *driver, int ncol, int *colctype,
                 const char **colname, int doatt, int nocat,
                 int *noatt, int *fout)
{
    if (multi)
        /* export as multi-polygons */
        return export_areas_multi(In, field, donocat,
                                  Ogr_featuredefn, Ogr_layer,
                                  Fi, driver, ncol, colctype, 
                                  colname, doatt, nocat,
                                  noatt, fout);
    
    /* export as polygons */
    return export_areas_single(In, field, donocat,
                               Ogr_featuredefn, Ogr_layer,
                               Fi, driver, ncol, colctype, 
                               colname, doatt, nocat,
                               noatt, fout);
}

int export_areas_single(struct Map_info *In, int field, int donocat,
                        OGRFeatureDefnH Ogr_featuredefn,OGRLayerH Ogr_layer,
                        struct field_info *Fi, dbDriver *driver, int ncol, int *colctype,
                        const char **colname, int doatt, int nocat,
                        int *n_noatt, int *n_nocat)
{
    int i, j;
    int centroid, cat, area, n_areas;
    int n_exported;
    
    struct line_pnts *Points;
    struct line_cats *Cats;

    OGRGeometryH Ogr_geometry;
    OGRFeatureH Ogr_feature;

    Points = Vect_new_line_struct();
    Cats = Vect_new_cats_struct();

    n_exported = 0;

    n_areas = Vect_get_num_areas(In);
    for (i = 1; i <= n_areas; i++) {
        G_percent(i, n_areas, 5);
        
        /* get centroid's category */
        centroid = Vect_get_area_centroid(In, i);
        cat = -1;
        if (centroid > 0) {
            Vect_read_line(In, NULL, Cats, centroid);
            Vect_cat_get(Cats, field, &cat);
        }
        G_debug(3, "area = %d centroid = %d ncats = %d", i, centroid,
                Cats->n_cats);
        if (cat < 0 && !donocat) {
            (*n_nocat)++;
            continue; /* skip areas without category, do not export
                       * not labeled */
        }
        
        /* find correspoding area */
        area = Vect_get_centroid_area(In, centroid);
        if (area == 0)
            continue;

        /* create polygon from area */
        Ogr_geometry = create_polygon(In, area, Points);

        /* add feature */
        Ogr_feature = OGR_F_Create(Ogr_featuredefn);
        OGR_F_SetGeometry(Ogr_feature, Ogr_geometry);
        
        /* output one feature for each category */
        for (j = -1; j < Cats->n_cats; j++) {
            if (j == -1) {
                if (cat >= 0)
                    continue;	/* cat(s) exists */
            }
            else {
                if (Cats->field[j] == field)
                    cat = Cats->cat[j];
                else
                    continue;
            }
            
            mk_att(cat, Fi, driver, ncol, colctype, colname, doatt, nocat,
                   Ogr_feature, n_noatt);
            OGR_L_CreateFeature(Ogr_layer, Ogr_feature);
            
            n_exported++;
        }
        OGR_G_DestroyGeometry(Ogr_geometry);
        OGR_F_Destroy(Ogr_feature);
    }

    Vect_destroy_line_struct(Points);
    Vect_destroy_cats_struct(Cats);

    return n_exported;
}

int export_areas_multi(struct Map_info *In, int field, int donocat,
                       OGRFeatureDefnH Ogr_featuredefn,OGRLayerH Ogr_layer,
                       struct field_info *Fi, dbDriver *driver, int ncol, int *colctype,
                       const char **colname, int doatt, int nocat,
                       int *n_noatt, int *n_nocat)
{
    int i, n_exported, area;
    int cat, lcat, ncats_field, line, type, findex, ipart;

    struct line_pnts *Points;
    struct line_cats *Cats;
    struct ilist *cat_list, *line_list;

    OGRGeometryH Ogr_geometry, Ogr_geometry_part;
    OGRFeatureH Ogr_feature;
    OGRwkbGeometryType wkbtype, wkbtype_part;
    
    Points = Vect_new_line_struct();
    Cats = Vect_new_cats_struct();
    cat_list = Vect_new_list();
    line_list = Vect_new_list();

    n_exported = 0;

    /* check if category index is available for given field */
    findex = Vect_cidx_get_field_index(In, field);
    if (findex == -1)
        G_fatal_error(_("Unable to export multi-features. No category index for layer %d."),
                      field);
    
    /* determine type */
    wkbtype_part = wkbPolygon;
    wkbtype = get_multi_wkbtype(wkbtype_part);
    
    ncats_field = Vect_cidx_get_unique_cats_by_index(In, findex, cat_list);
    G_debug(1, "n_cats = %d for layer %d", ncats_field, field);

    for (i = 0; i < cat_list->n_values; i++) {
        cat = cat_list->value[i];
        /* find all centroids with given category */
        Vect_cidx_find_all(In, field, GV_CENTROID, cat, line_list);

        /* create multi-feature */
        Ogr_geometry = OGR_G_CreateGeometry(wkbtype);

        /* build simple features geometry, go through all parts */
        for (ipart = 0; ipart < line_list->n_values; ipart++) {
            line = line_list->value[ipart];
            G_debug(3, "cat=%d, line=%d -> part=%d", cat, line, ipart);

            /* get centroid's category */
            Vect_read_line(In, NULL, Cats, line);
            /* check for category consistency */
            Vect_cat_get(Cats, field, &lcat);
            if (lcat > 0 && lcat != cat)
                G_fatal_error(_("Unable to create multi-feature. "
                                "Invalid category %d (should be %d)"),
                              lcat, cat);
            
            /* find correspoding area */
            area = Vect_get_centroid_area(In, line);
            if (area == 0)
                continue;
                
            /* create polygon from area */
            Ogr_geometry_part = create_polygon(In, area, Points);

            /* add part */
            OGR_G_AddGeometryDirectly(Ogr_geometry, Ogr_geometry_part);
        }

        if (!OGR_G_IsEmpty(Ogr_geometry)) {
            /* write multi-feature */
            Ogr_feature = OGR_F_Create(Ogr_featuredefn);
            OGR_F_SetGeometry(Ogr_feature, Ogr_geometry);
            
            mk_att(cat, Fi, driver, ncol, colctype, colname, doatt, nocat,
                   Ogr_feature, n_noatt);
            OGR_L_CreateFeature(Ogr_layer, Ogr_feature);

            OGR_F_Destroy(Ogr_feature);

            n_exported++;
        }
        else {
            /* skip empty features */
            G_debug(3, "multi-feature is empty -> skipped");
        }
        
        OGR_G_DestroyGeometry(Ogr_geometry);
    }

    /* check lines without category, if -c flag is given write them as
     * one multi-feature */
    Ogr_geometry = OGR_G_CreateGeometry(wkbtype);
    
    Vect_rewind(In);
    Vect_set_constraint_type(In, GV_CENTROID);
    while(TRUE) {
        type = Vect_read_next_line(In, NULL, Cats);
        if (type < 0)
            break;

        /* get centroid's category */
        Vect_cat_get(Cats, field, &cat);
        if (cat > 0)
            continue; /* skip features with category */
        if (cat < 0 && !donocat) {
            (*n_nocat)++;
            continue; /* skip lines without category, do not export
                       * not labeled */
        }

        /* find correspoding area */
        area = Vect_get_centroid_area(In, line);
        if (area == 0)
                continue;
                
        /* create polygon from area */
        Ogr_geometry_part = create_polygon(In, area, Points);
        
        /* add part */
        OGR_G_AddGeometryDirectly(Ogr_geometry, Ogr_geometry_part);
    }

    if (!OGR_G_IsEmpty(Ogr_geometry)) {
        /* write multi-feature */
        Ogr_feature = OGR_F_Create(Ogr_featuredefn);
        OGR_F_SetGeometry(Ogr_feature, Ogr_geometry);
        
        mk_att(cat, Fi, driver, ncol, colctype, colname, doatt, nocat,
               Ogr_feature, n_noatt);
        OGR_L_CreateFeature(Ogr_layer, Ogr_feature);

        OGR_F_Destroy(Ogr_feature);
        
        n_exported++;
    }
    else {
        /* skip empty features */
        G_debug(3, "multi-feature is empty -> skipped");
    }
    
    OGR_G_DestroyGeometry(Ogr_geometry);
    
    Vect_destroy_line_struct(Points);
    Vect_destroy_cats_struct(Cats);
    Vect_destroy_list(cat_list);
    Vect_destroy_list(line_list);
    
    return n_exported;
}

OGRGeometryH create_polygon(struct Map_info *In, int area,
                            struct line_pnts *Points)
{
    int j, k;
    OGRGeometryH Ogr_geometry, ring;
    
    Vect_get_area_points(In, area, Points);
    
    Ogr_geometry = OGR_G_CreateGeometry(wkbPolygon);
    ring = OGR_G_CreateGeometry(wkbLinearRing);
    
    /* Area */
    for (j = 0; j < Points->n_points; j++) {
        OGR_G_AddPoint(ring, Points->x[j], Points->y[j],
                       Points->z[j]);
    }
    
    OGR_G_AddGeometryDirectly(Ogr_geometry, ring);
    
    /* Isles */
    for (k = 0; k < Vect_get_area_num_isles(In, area); k++) {
        Vect_get_isle_points(In, Vect_get_area_isle(In, area, k),
                             Points);
        
        ring = OGR_G_CreateGeometry(wkbLinearRing);
        for (j = 0; j < Points->n_points; j++) {
            OGR_G_AddPoint(ring, Points->x[j], Points->y[j],
                           Points->z[j]);
        }
        OGR_G_AddGeometryDirectly(Ogr_geometry, ring);
    }

    return Ogr_geometry;
}
