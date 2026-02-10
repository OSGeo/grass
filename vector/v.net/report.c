#include <stdlib.h>
#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/glocale.h>
#include "proto.h"
#include <grass/gjson.h>

int report(struct Map_info *In, int afield, int nfield, int action,
           const char *format)
{
    int i, j, line, nlines, ltype, node, nnodes;
    int cat_line, cat_node[2];

    struct line_cats *Cats, *Cats2;
    struct line_pnts *Points;
    struct bound_box box;

    double x, y, z;

    Cats = Vect_new_cats_struct();
    Cats2 = Vect_new_cats_struct();
    Points = Vect_new_line_struct();

    nlines = Vect_get_num_lines(In);

    if (action == TOOL_REPORT) {
        struct boxlist *List;
        G_JSON_Value *root_value = NULL;
        G_JSON_Array *root_array = NULL;
        if (format && strcmp(format, "json") == 0) {
            root_value = G_json_value_init_array();
            root_array = G_json_array(root_value);
        }

        List = Vect_new_boxlist(0);

        /* For all lines find categories for points on nodes */
        for (i = 1; i <= nlines; i++) {
            ltype = Vect_read_line(In, NULL, Cats, i);
            if (!(ltype & GV_LINES))
                continue;

            cat_line = 0;
            if (!Vect_cat_get(Cats, afield, &cat_line))
                G_warning(_("Line %d has no category"), i);

            cat_node[0] = cat_node[1] = -1;
            for (j = 0; j < 2; j++) {
                if (j == 0)
                    Vect_get_line_nodes(In, i, &node, NULL);
                else
                    Vect_get_line_nodes(In, i, NULL, &node);

                Vect_get_node_coor(In, node, &x, &y, &z);

                box.E = box.W = x;
                box.N = box.S = y;
                box.T = box.B = z;
                Vect_select_lines_by_box(In, &box, GV_POINT, List);

                nnodes = List->n_values;
                if (nnodes > 0) {
                    line = List->id[nnodes - 1]; /* last in list */
                    Vect_read_line(In, NULL, Cats, line);
                    Vect_cat_get(Cats, nfield, &(cat_node[j]));
                }

                if (nnodes == 0) {
                    /* this is ok, not every node needs to be
                     * represented by a point */
                    G_debug(4, "No point here: %g %g %.g line category: %d", x,
                            y, z, cat_line);
                }
                else if (nnodes > 1)
                    G_warning(_("%d points found: %g %g %g line category: %d"),
                              nnodes, x, y, z, cat_line);
            }
            if (root_array) {
                G_JSON_Value *item_value = G_json_value_init_object();
                G_JSON_Object *item_obj = G_json_object(item_value);

                G_json_object_set_number(item_obj, "line_cat", cat_line);
                G_json_object_set_number(item_obj, "start_node_cat",
                                         cat_node[0]);
                G_json_object_set_number(item_obj, "end_node_cat", cat_node[1]);

                G_json_array_append_value(root_array, item_value);
            }
            else {
                fprintf(stdout, "%d %d %d\n", cat_line, cat_node[0],
                        cat_node[1]);
            }
        }

        if (root_value) {
            char *json_str = G_json_serialize_to_string_pretty(root_value);
            if (json_str) {
                fprintf(stdout, "%s\n", json_str);
            }
            G_json_free_serialized_string(json_str);
            G_json_value_free(root_value);
        }

        Vect_destroy_boxlist(List);
    }
    else { /* node report */
        int elem, nelem, type, k, l;
        struct ilist *List;

        List = Vect_new_list();

        G_JSON_Value *root_val = (format && strcmp(format, "json") == 0)
                                     ? G_json_value_init_array()
                                     : NULL;
        G_JSON_Array *root_arr = G_json_array(root_val);

        for (i = 1; i <= nlines; i++) {

            if (Vect_get_line_type(In, i) != GV_POINT)
                continue;

            Vect_read_line(In, Points, Cats, i);

            box.E = box.W = Points->x[0];
            box.N = box.S = Points->y[0];
            box.T = box.B = Points->z[0];

            nnodes = Vect_select_nodes_by_box(In, &box, List);

            if (nnodes > 1) {
                G_warning(_("Duplicate nodes at x=%g y=%g z=%g "), Points->x[0],
                          Points->y[0], Points->z[0]);
            }
            if (nnodes > 0) {
                node = List->value[0];
                nelem = Vect_get_node_n_lines(In, node);

                /* Loop through all cats of point */
                for (j = 0; j < Cats->n_cats; j++) {
                    if (Cats->field[j] == nfield) {
                        int count = 0;
                        G_JSON_Value *item_val =
                            root_arr ? G_json_value_init_object() : NULL;
                        G_JSON_Value *lines_val =
                            root_arr ? G_json_value_init_array() : NULL;

                        if (root_arr) {
                            G_json_object_set_number(
                                G_json_value_get_object(item_val), "node_cat",
                                Cats->cat[j]);
                        }
                        else {
                            fprintf(stdout, "%d ", Cats->cat[j]);
                        }
                        /* Loop through all lines */
                        for (k = 0; k < nelem; k++) {
                            elem = abs(Vect_get_node_line(In, node, k));
                            type = Vect_read_line(In, NULL, Cats2, elem);
                            if (!(type & GV_LINES))
                                continue;

                            /* Loop through all cats of line */
                            for (l = 0; l < Cats2->n_cats; l++) {
                                if (Cats2->field[l] == afield) {
                                    if (root_arr)
                                        G_json_array_append_number(
                                            G_json_array(lines_val),
                                            Cats2->cat[l]);
                                    else {
                                        if (count > 0)
                                            fprintf(stdout, ",");
                                        fprintf(stdout, "%d", Cats2->cat[l]);
                                        count++;
                                    }
                                }
                            }
                        }
                        if (root_arr) {
                            G_json_object_set_value(
                                G_json_value_get_object(item_val), "lines",
                                lines_val);
                            G_json_array_append_value(root_arr, item_val);
                        }
                        else
                            fprintf(stdout, "\n");
                    }
                }
            }
        }
        if (root_val) {
            char *s = G_json_serialize_to_string_pretty(root_val);
            if (s) {
                fprintf(stdout, "%s\n", s);
                G_json_free_serialized_string(s);
            }
            G_json_value_free(root_val);
        }
        Vect_destroy_list(List);
    }
    Vect_destroy_cats_struct(Cats);
    Vect_destroy_cats_struct(Cats2);
    Vect_destroy_line_struct(Points);

    return 0;
}
