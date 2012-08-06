#include <grass/gis.h>
#include <grass/glocale.h>

#include "local_proto.h"

static int export_nodes(struct Map_info *, struct Map_info *);

int export_topo(struct Map_info *In, int field, struct Map_info *Out)
{
    int nfeat;

    nfeat = 0;

    /* export GRASS nodes -> PostGIS nodes */
    nfeat += export_nodes(In, Out);
    
    /*
      export GRASS points -> PostGIS nodes
      export GRASS lines/boundaries -> PostGIS edges
     
      skip centroids
    */
    G_message(_("Exporting points/lines/boundaries..."));
    nfeat += export_lines(In, field, Out);
    
    /* export GRASS areas as PostGIS faces */

    /* export GRASS isles as PostGIS faces */

    /* export GRASS centroids as nodes (in faces) */
    
    return nfeat;
}

int export_nodes(struct Map_info *In, struct Map_info *Out)
{
    int node, nnodes, with_z;
    double x, y, z;
    
    struct line_pnts *Points;
    struct line_cats *Cats;
    
    nnodes = Vect_get_num_nodes(In);
    if (nnodes < 1)
        return 0;
    
    with_z = Vect_is_3d(In);

    Points = Vect_new_line_struct();
    Cats    = Vect_new_cats_struct();

    G_message(_("Exporting nodes..."));
    Vect_append_point(Points, 0., 0., 0.);
    for (node = 1; node <= nnodes; node++) {
        G_debug(3, "Exporting GRASS node %d", node);
        
        G_percent(node, nnodes, 5);
        Vect_get_node_coor(In, node, &x, &y, &z);
        Points->x[0] = x;
        Points->y[0] = y;
        if (with_z)
            Points->z[0] = z;

        if (-1 == Vect_write_line(Out, GV_POINT, Points, Cats))
            G_fatal_error(_("Unable to export node %d. Exiting."), node);
    }
        
    Vect_destroy_line_struct(Points);
    Vect_destroy_cats_struct(Cats);

    return nnodes;
}

