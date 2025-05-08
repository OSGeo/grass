/*!
   \file vector/neta/utils.c

   \brief Network Analysis library - utils

   Utils subroutines.

   (C) 2009-2010 by Daniel Bundala, and the GRASS Development Team

   This program is free software under the GNU General Public License
   (>=v2). Read the file COPYING that comes with GRASS for details.

   \author Daniel Bundala (Google Summer of Code 2009)
 */

#include <stdio.h>
#include <stdlib.h>
#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/glocale.h>
#include <grass/dbmi.h>
#include <grass/neta.h>

/*!
   \brief Writes point

   Writes GV_POINT to Out at the position of the node in <em>In</em>.

   \param In pointer to Map_info structure (input vector map)
   \param[in,out] Out pointer to Map_info structure (output vector map)
   \param node node id
   \param Cats pointer to line_cats structures
 */
void NetA_add_point_on_node(struct Map_info *In, struct Map_info *Out, int node,
                            struct line_cats *Cats)
{
    static struct line_pnts *Points;
    double x, y, z;

    Points = Vect_new_line_struct();
    Vect_get_node_coor(In, node, &x, &y, &z);
    Vect_reset_line(Points);
    Vect_append_point(Points, x, y, z);
    Vect_write_line(Out, GV_POINT, Points, Cats);
    Vect_destroy_line_struct(Points);
}

/* Returns the list of all points with the given category and field */
/*void NetA_get_points_by_category(struct Map_info *In, int field, int cat,
 * struct ilist *point_list)
 * {
 * int i, nlines;
 * struct line_cats *Cats;
 * Cats = Vect_new_cats_struct();
 * Vect_get_num_lines(In);
 * for(i=1;i<=nlines;i++){
 * int type = Vect_read_line(In, NULL, Cats, i);
 * if(type!=GV_POINT)continue;
 * }
 *
 * Vect_destroy_cats_struct(Cats);
 * }
 */

/*!
   \brief Finds node

   Find the node corresponding to each point in the point_list

   \param In pointer to Map_info structure
   \param point_list list of points (their ids)
 */
void NetA_points_to_nodes(struct Map_info *In, struct ilist *point_list)
{
    int i, node;
    struct line_pnts *Points = Vect_new_line_struct();

    for (i = 0; i < point_list->n_values; i++) {
        /* Vect_get_line_nodes(In, point_list->value[i], &node, NULL); */
        node =
            Vect_find_node(In, Points->x[0], Points->y[0], Points->z[0], 0, 0);
        point_list->value[i] = node;
    }
    Vect_destroy_line_struct(Points);
}

/*!
   \brief Get node cost

   For each node in the map, finds the category of the point on it (if
   there is any) and stores the value associated with this category in
   the array node_costs. If there is no point with a category,
   node_costs=0.

   node_costs are multiplied by the graph's cost multiplier and
   truncated to integers (as is done in Vect_net_build_graph)

   \param In pointer to Map_info structure
   \param layer layer number
   \param column name of column
   \param[out] node_costs list of node costs

   \returns 1 on success
   \return 0 on failure
 */
int NetA_get_node_costs(struct Map_info *In, int layer, char *column,
                        int *node_costs)
{
    int i, nlines, nnodes;
    dbCatValArray vals;
    struct line_cats *Cats;
    struct line_pnts *Points;

    dbDriver *driver;
    struct field_info *Fi;

    Fi = Vect_get_field(In, layer);
    if (Fi == NULL)
        G_fatal_error(_("Database connection not defined for layer %d"), layer);

    driver = db_start_driver_open_database(Fi->driver, Fi->database);
    if (driver == NULL)
        G_fatal_error(_("Unable to open database <%s> by driver <%s>"),
                      Fi->database, Fi->driver);

    nlines = Vect_get_num_lines(In);
    nnodes = Vect_get_num_nodes(In);
    for (i = 1; i <= nnodes; i++)
        node_costs[i] = 0;

    db_CatValArray_init(&vals);
    int nvals =
        db_select_CatValArray(driver, Fi->table, Fi->key, column, NULL, &vals);

    db_close_database_shutdown_driver(driver);
    Vect_destroy_field_info(Fi);

    if (nvals == -1)
        return 0;

    Cats = Vect_new_cats_struct();
    Points = Vect_new_line_struct();
    for (i = 1; i <= nlines; i++) {
        int type = Vect_read_line(In, Points, Cats, i);

        if (type == GV_POINT) {
            int node, cat;
            double value;

            if (!Vect_cat_get(Cats, layer, &cat))
                continue;
            Vect_get_line_nodes(In, i, &node, NULL);
            if (db_CatValArray_get_value_double(&vals, cat, &value) == DB_OK) {
                if (value < 0)
                    node_costs[node] = -1;
                else
                    node_costs[node] = value * In->dgraph.cost_multip;
            }
        }
    }

    Vect_destroy_cats_struct(Cats);
    Vect_destroy_line_struct(Points);
    db_CatValArray_free(&vals);
    return 1;
}

/*!
   \brief Get list of nodes from varray

   Returns the list of all nodes on features selected by varray.
   nodes_to_features contains the index of a feature adjacent to each
   node or -1 if no such feature specified by varray
   exists. Nodes_to_features might be NULL, in which case it is left
   uninitialised. Nodes_to_features will be wrong if several lines
   connect to the same node.

   \param map pointer to Map_info structure
   \param varray pointer to varray structure
   \param[out] nodes list of node ids
   \param[out] nodes_to_features maps nodes to features
 */
void NetA_varray_to_nodes(struct Map_info *map, struct varray *varray,
                          struct ilist *nodes, int *nodes_to_features)
{
    int nlines, nnodes, i;
    struct line_pnts *Points = Vect_new_line_struct();

    nlines = Vect_get_num_lines(map);
    nnodes = Vect_get_num_nodes(map);
    if (nodes_to_features)
        for (i = 1; i <= nnodes; i++)
            nodes_to_features[i] = -1;

    for (i = 1; i <= nlines; i++) {
        if (varray->c[i]) {
            int type = Vect_read_line(map, Points, NULL, i);

            if (type == GV_POINT) {
                int node;

                node = Vect_find_node(map, Points->x[0], Points->y[0],
                                      Points->z[0], 0, 0);
                if (node) {
                    Vect_list_append(nodes, node);
                    if (nodes_to_features)
                        nodes_to_features[node] = i;
                }
                else
                    G_warning(_("Point %d is not connected!"), i);
            }
            else {
                int node1, node2;

                Vect_get_line_nodes(map, i, &node1, &node2);
                Vect_list_append(nodes, node1);
                Vect_list_append(nodes, node2);
                if (nodes_to_features)
                    nodes_to_features[node1] = nodes_to_features[node2] = i;
            }
        }
    }
    Vect_destroy_line_struct(Points);
}

/*!
   \brief Initialize varray

   \param In pointer to Map_info structure
   \param layer layer number
   \param mask_type ?
   \param where where statement
   \param cat ?
   \param[out] pointer to varray structure

   \return number of items set
   \return -1 on error
 */
int NetA_initialise_varray(struct Map_info *In, int layer, int mask_type,
                           char *where, char *cat, struct varray **varray)
{
    int n, ni;

    if (layer < 1)
        G_fatal_error(_("'%s' must be > 0"), "layer");

    n = Vect_get_num_lines(In);
    *varray = Vect_new_varray(n);
    ni = 0;

    /* parse filter option and select appropriate lines */
    if (where) {
        if (cat)
            G_warning(_("'where' and 'cats' parameters were supplied, cat will "
                        "be ignored"));
        ni = Vect_set_varray_from_db(In, layer, where, mask_type, 1, *varray);
        if (ni == -1) {
            G_warning(_("Unable to load data from database"));
        }
        return ni;
    }
    else if (cat) {
        ni = Vect_set_varray_from_cat_string(In, layer, cat, mask_type, 1,
                                             *varray);
        if (ni == -1) {
            G_warning(_("Problem loading category values"));
        }
        return ni;
    }
    else { /* all features of given layer */
        int i, cat;
        int ltype; /* line type */
        struct line_cats *Cats;

        Cats = Vect_new_cats_struct();

        for (i = 1; i <= n; i++) {
            ltype = Vect_read_line(In, NULL, Cats, i);

            if (!(ltype & mask_type))
                continue; /* is not specified type */

            if (Vect_cat_get(Cats, layer, &cat)) {
                (*varray)->c[i] = 1;
                ni++;
            }
        }
        Vect_destroy_cats_struct(Cats);

        return ni;
    }
}
