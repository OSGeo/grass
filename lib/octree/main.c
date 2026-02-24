#include "octree.h"
#include <grass/gis.h>
#include <grass/vector.h>
#include <omp.h>
#include <stdio.h>

// Main function for the GRASS module
int main(int argc, char *argv[])
{
    struct GModule *module;
    struct Option *input, *output;
    struct Map_info In, Out;

    G_gisinit(argv[0]);
    module = G_define_module();
    module->description = "Builds an octree from a vector point map.";

    input = G_define_standard_option(G_OPT_V_INPUT);
    output = G_define_standard_option(G_OPT_V_OUTPUT);

    if (G_parser(argc, argv))
        exit(EXIT_FAILURE);

    // Open the input vector map
    if (Vect_open_old(&In, input->answer, "") < 0)
        G_fatal_error("Unable to open vector map <%s>", input->answer);

    // Determine the bounding box of the input points
    BOUND_BOX box;
    Vect_get_map_box(&In, &box);

    // Create the root octree node
    OctreeNode *root =
        create_octree_node(box.W, box.E, box.S, box.N, box.B, box.T);

    // Read points and insert them into the octree
    struct line_pnts *points = Vect_new_line_struct();
    struct line_cats *cats = Vect_new_cats_struct();

    int type;
#pragma omp parallel for private(points, cats, type)
    for (int line = 1; line <= Vect_get_num_lines(&In); line++) {
        type = Vect_read_line(&In, points, cats, line);
        if (!(type & GV_POINT))
            continue;

        Point3D p = {points->x[0], points->y[0], points->z[0]};
#pragma omp critical
        insert_point(root, p);
    }

    Vect_close(&In);

    // If an output is specified, write some kind of output (e.g., octree
    // statistics or a visual representation)
    if (output->answer) {
        Vect_open_new(&Out, output->answer, 0);
        // Here you would add code to write out the octree structure or stats
        Vect_build(&Out);
        Vect_close(&Out);
    }

    // Clean up
    free_octree(root);
    return EXIT_SUCCESS;
}
