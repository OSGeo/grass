#include <stdio.h>

#include <grass/gis.h>
#include <grass/glocale.h>
#include <grass/vector.h>

#include "octree.h"

int main(int argc, char *argv[])
{
    struct GModule *module;
    struct Option *input;
    struct Map_info In;

    G_gisinit(argv[0]);
    module = G_define_module();
    module->description = _("Builds an octree from a vector point map.");

    input = G_define_standard_option(G_OPT_V_INPUT);

    if (G_parser(argc, argv))
        exit(EXIT_FAILURE);

    if (Vect_open_old(&In, input->answer, "") < 0)
        G_fatal_error(_("Unable to open vector map <%s>"), input->answer);

    BOUND_BOX box;
    Vect_get_map_box(&In, &box);

    OctreeNode *root =
        create_octree_node(box.W, box.E, box.S, box.N, box.B, box.T);

    struct line_pnts *points = Vect_new_line_struct();
    struct line_cats *cats = Vect_new_cats_struct();

    int type;
    int inserted = 0;
    int skipped = 0;

    for (int line = 1; line <= Vect_get_num_lines(&In); line++) {
        type = Vect_read_line(&In, points, cats, line);
        if (!(type & GV_POINT))
            continue;

        Point3D p = {points->x[0], points->y[0], points->z[0]};

        if (insert_point(root, p) == 0)
            inserted++;
        else
            skipped++;
    }

    Vect_destroy_line_struct(points);
    Vect_destroy_cats_struct(cats);
    Vect_close(&In);

    G_message(_("Inserted %d points into octree (%d skipped)"), inserted,
              skipped);

    free_octree(root);

    return EXIT_SUCCESS;
}
