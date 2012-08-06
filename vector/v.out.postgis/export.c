#include <grass/gis.h>
#include <grass/glocale.h>

#include "local_proto.h"

int export_lines(struct Map_info *In, int field, struct Map_info *Out)
{
    int type, nlines;

    struct line_pnts *Points;
    struct line_cats *Cats;

    Points  = Vect_new_line_struct();
    Cats    = Vect_new_cats_struct();

    nlines = 0;
    while(TRUE) {
        type = Vect_read_next_line(In, Points, Cats);
        if (type == -2)
            break; /* eof */
        
        G_debug(3, "Export line %d", ++nlines);

        if (-1 == Vect_write_line(Out, type, Points, Cats))
            G_fatal_error(_("Unable to export feature %d. Exiting."), nlines);
        
        G_progress(nlines, 1e3);
    }
    G_progress(1, 1);

    if (nlines < 1)
        G_warning(_("Nothing exported"));
    
    Vect_destroy_line_struct(Points);
    Vect_destroy_cats_struct(Cats);
    
    return nlines;
}

int export_areas(struct Map_info *In, int field, struct Map_info *Out)
{
    int cat, i, isle;
    int area, nareas, nisles, nisles_alloc;

    struct line_pnts *Points, **IPoints;
    struct line_cats *Cats;

    Points = Vect_new_line_struct();
    Cats   = Vect_new_cats_struct();
    
    IPoints = NULL;
    nisles_alloc = 0;
    
    nareas = Vect_get_num_areas(In);
    for(area = 1; area <= nareas; area++) {
        Vect_reset_cats(Cats);
        
        G_percent(area, nareas, 3);
        G_debug(3, "Export area %d", area);

        /* get outer ring geometry */
        Vect_get_area_points(In, area, Points);
        
        /* get category */
        cat = Vect_get_area_cat(In, area, field);
        if (cat < 0) {
            G_warning(_("No centroid found for area %d. "
                        "Area not exported."),
                      area);
            continue;
        }
        G_debug(3, " -> cat %d", cat);
        Vect_cat_set(Cats, field, cat);

        nisles = Vect_get_area_num_isles(In, area);
        if (nisles > nisles_alloc) {
            /* reallocate space for isles */
            IPoints = (struct line_pnts **) G_realloc(IPoints,
                                                      nisles *
                                                      sizeof(struct line_pnts *));
            for (i = nisles_alloc; i < nisles; i++)
                IPoints[i] = Vect_new_line_struct();
            nisles_alloc = nisles;
        }
        G_debug(3, " -> nisles=%d", nisles);
        
        /* get isles geometry */
        for (i = 0; i < nisles; i++) {
            isle = Vect_get_area_isle(In, area, i);
            Vect_get_isle_points(In, isle, IPoints[i]);
        }

        V2_write_area_pg(Out, Points, Cats,
                         (const struct line_pnts **) IPoints, nisles);

        /* TODO */
    }
    
    Vect_destroy_line_struct(Points);
    Vect_destroy_cats_struct(Cats);
    for (i = 0; i < nisles_alloc; i++)
        Vect_destroy_line_struct(IPoints[i]);
    
    return nareas;
}
