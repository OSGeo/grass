/*!
 * \file lib/vector/Vlib/net_build.c
 *
 * \brief Vector library - related fns for vector network building
 *
 * Higher level functions for reading/writing/manipulating vectors.
 *
 * (C) 2001-2009, 2014 by the GRASS Development Team
 *
 * This program is free software under the GNU General Public License
 * (>=v2).  Read the file COPYING that comes with GRASS for details.
 *
 * \author Radim Blazek
 * \author Stepan Turek stepan.turek seznam.cz (turns support)
 */

#include <grass/dbmi.h>
#include <grass/vector.h>
#include <grass/glocale.h>

/*!
   \brief Build network graph with turntable.

   Internal format for edge costs is integer, costs are multiplied
   before conversion to int by 1000 and for lengths LL without geo flag by
   1000000. The same multiplication factor is used for nodes. Costs in database
   column may be 'integer' or 'double precision' number >= 0 or -1 for infinity
   i.e. arc or node is closed and cannot be traversed If record in table is not
   found for arcs, costs for arc are set to 0. If record in table is not found
   for node, costs for node are set to 0.

   \param Map vector map
   \param ltype line type for arcs
   \param afield arc costs field (if 0, use length)
   \param nfield node costs field (if 0, do not use node costs)
   \param tfield field where turntable is attached
   \param tucfield field with unique categories used in the turntable
   \param afcol column with forward costs for arc
   \param abcol column with backward costs for arc (if NULL, back costs =
          forward costs)
   \param ncol column with costs for nodes (if NULL, do not use
          node costs)
   \param geo use geodesic calculation for length (LL)
   \param algorithm not used (in future code for algorithm)

   \return 0 on success, 1 on error
 */

int Vect_net_ttb_build_graph(struct Map_info *Map, int ltype, int afield,
                             int nfield, int tfield, int tucfield,
                             const char *afcol, const char *abcol,
                             const char *ncol, int geo, int algorithm UNUSED)
{
    /* TODO very long function, split into smaller ones */
    int i, j, from, to, line, nlines, nnodes, ret, type, cat, skipped, cfound;
    struct line_pnts *Points;
    struct line_cats *Cats;
    double dcost, bdcost, ll;
    int cost, bcost;
    dglGraph_s *gr;
    dglInt32_t opaqueset[16] = {360000, 0, 0, 0, 0, 0, 0, 0,
                                0,      0, 0, 0, 0, 0, 0, 0};
    struct field_info *Fi = NULL;
    dbDriver *driver = NULL;
    dbDriver *ttbdriver = NULL;
    dbHandle handle;
    dbString stmt;
    dbColumn *Column = NULL;
    dbCatValArray fvarr, bvarr;
    int fctype = 0, bctype = 0, nrec, nturns;

    int ln_cat, nnode_lns, i_line, line_id, i_virt_edge;
    struct line_cats *ln_Cats;
    double x, y, z;
    struct bound_box box;
    struct boxlist *List;

    dglInt32_t dgl_cost;

    /*TODO attributes of turntable should be stored in one place */
    const char *tcols[] = {"cat", "ln_from", "ln_to", "cost", "isec", NULL};
    dbCatValArray tvarrs[5];
    int tctype[5] = {0};
    int tucfield_idx;

    int t, f;
    int node_pt_id, turn_cat, tucfound;
    int isec;

    /* TODO int costs -> double (waiting for dglib) */
    G_debug(1,
            "Vect_net_ttb_build_graph(): "
            "ltype = %d, afield = %d, nfield = %d, tfield = %d, tucfield = %d ",
            ltype, afield, nfield, tfield, tucfield);
    G_debug(1, "    afcol = %s, abcol = %s, ncol = %s", afcol, abcol, ncol);

    G_message(_("Building graph..."));

    Map->dgraph.line_type = ltype;

    Points = Vect_new_line_struct();
    Cats = Vect_new_cats_struct();

    ll = 0;
    if (G_projection() == 3)
        ll = 1; /* LL */

    if (afcol == NULL && ll && !geo)
        Map->dgraph.cost_multip = 1000000;
    else
        Map->dgraph.cost_multip = 1000;

    nlines = Vect_get_num_lines(Map);
    nnodes = Vect_get_num_nodes(Map);

    gr = &(Map->dgraph.graph_s);

    /* Allocate space for costs, later replace by functions reading costs from
     * graph */
    Map->dgraph.edge_fcosts = (double *)G_malloc((nlines + 1) * sizeof(double));
    Map->dgraph.edge_bcosts = (double *)G_malloc((nlines + 1) * sizeof(double));
    Map->dgraph.node_costs = (double *)G_malloc((nnodes + 1) * sizeof(double));

    /* Set to -1 initially */
    for (i = 1; i <= nlines; i++) {
        Map->dgraph.edge_fcosts[i] = -1; /* forward */
        Map->dgraph.edge_bcosts[i] = -1; /* backward */
    }
    for (i = 1; i <= nnodes; i++) {
        Map->dgraph.node_costs[i] = 0;
    }

    dglInitialize(gr, (dglByte_t)1, sizeof(dglInt32_t), (dglInt32_t)0,
                  opaqueset);

    if (gr == NULL)
        G_fatal_error(_("Unable to build network graph"));

    db_init_handle(&handle);
    db_init_string(&stmt);

    if (abcol != NULL && afcol == NULL)
        G_fatal_error(_("Forward costs column not specified"));

    /* --- Add arcs --- */
    /* Open db connection */

    /* Get field info */
    if (tfield < 1)
        G_fatal_error(_("Turntable field < 1"));
    Fi = Vect_get_field(Map, tfield);
    if (Fi == NULL)
        G_fatal_error(_("Database connection not defined for layer %d"),
                      tfield);

    /* Open database */
    ttbdriver = db_start_driver_open_database(Fi->driver, Fi->database);
    if (ttbdriver == NULL)
        G_fatal_error(_("Unable to open database <%s> by driver <%s>"),
                      Fi->database, Fi->driver);

    i = 0;
    while (tcols[i]) {
        /* Load costs to array */
        if (db_get_column(ttbdriver, Fi->table, tcols[i], &Column) != DB_OK)
            G_fatal_error(_("Turntable column <%s> not found in table <%s>"),
                          tcols[i], Fi->table);

        tctype[i] = db_sqltype_to_Ctype(db_get_column_sqltype(Column));
        db_free_column(Column);

        if ((tctype[i] == DB_C_TYPE_INT || tctype[i] == DB_C_TYPE_DOUBLE) &&
            !strcmp(tcols[i], "cost"))
            ;
        else if (tctype[i] == DB_C_TYPE_INT)
            ;
        else
            G_fatal_error(
                _("Data type of column <%s> not supported (must be numeric)"),
                tcols[i]);

        db_CatValArray_init(&tvarrs[i]);
        nturns = db_select_CatValArray(ttbdriver, Fi->table, Fi->key, tcols[i],
                                       NULL, &tvarrs[i]);
        ++i;
    }

    Vect_destroy_field_info(Fi);

    G_debug(1, "forward costs: nrec = %d", nturns);

    /* Set node attributes */
    G_message("Register nodes");
    if (ncol != NULL) {

        G_debug(2, "Set nodes' costs");
        if (nfield < 1)
            G_fatal_error("Node field < 1");

        G_message(_("Setting node costs..."));

        Fi = Vect_get_field(Map, nfield);
        if (Fi == NULL)
            G_fatal_error(_("Database connection not defined for layer %d"),
                          nfield);

        driver = db_start_driver_open_database(Fi->driver, Fi->database);
        if (driver == NULL)
            G_fatal_error(_("Unable to open database <%s> by driver <%s>"),
                          Fi->database, Fi->driver);

        /* Load costs to array */
        if (db_get_column(driver, Fi->table, ncol, &Column) != DB_OK)
            G_fatal_error(_("Column <%s> not found in table <%s>"), ncol,
                          Fi->table);

        fctype = db_sqltype_to_Ctype(db_get_column_sqltype(Column));
        db_free_column(Column);

        if (fctype != DB_C_TYPE_INT && fctype != DB_C_TYPE_DOUBLE)
            G_fatal_error(
                _("Data type of column <%s> not supported (must be numeric)"),
                ncol);

        db_CatValArray_init(&fvarr);

        nrec = db_select_CatValArray(driver, Fi->table, Fi->key, ncol, NULL,
                                     &fvarr);
        G_debug(1, "node costs: nrec = %d", nrec);
        Vect_destroy_field_info(Fi);

        tucfield_idx = Vect_cidx_get_field_index(Map, tucfield);
    }

    List = Vect_new_boxlist(0);
    ln_Cats = Vect_new_cats_struct();

    G_message("Building turns graph...");

    i_virt_edge = -1;
    for (i = 1; i <= nnodes; i++) {
        /* TODO: what happens if we set attributes of non existing node (skipped
         * lines, nodes without lines) */

        /* select points at node */
        Vect_get_node_coor(Map, i, &x, &y, &z);
        box.E = box.W = x;
        box.N = box.S = y;
        box.T = box.B = z;
        Vect_select_lines_by_box(Map, &box, GV_POINT, List);

        G_debug(2, "  node = %d nlines = %d", i, List->n_values);
        cfound = 0;
        dcost = 0;
        tucfound = 0;

        for (j = 0; j < List->n_values; j++) {
            line = List->id[j];
            G_debug(2, "  line (%d) = %d", j, line);
            type = Vect_read_line(Map, NULL, Cats, line);
            if (!(type & GV_POINT))
                continue;
            /* get node column costs */
            if (ncol != NULL && !cfound &&
                Vect_cat_get(Cats, nfield,
                             &cat)) { /* point with category of field found */
                /* Set costs */
                if (fctype == DB_C_TYPE_INT) {
                    ret = db_CatValArray_get_value_int(&fvarr, cat, &cost);
                    dcost = cost;
                }
                else { /* DB_C_TYPE_DOUBLE */
                    ret = db_CatValArray_get_value_double(&fvarr, cat, &dcost);
                }
                if (ret != DB_OK) {
                    G_warning(
                        _("Database record for node %d (cat = %d) not found "
                          "(cost set to 0)"),
                        i, cat);
                }
                cfound = 1;
                Map->dgraph.node_costs[i] = dcost;
            }

            /* add virtual nodes and lines, which represents the intersections
               there are added two nodes for every intersection, which are
               linked with the nodes (edges in primal graph). the positive node
               - when we are going from the intersection the negative node -
               when we are going to the intersection

               TODO There are more possible approaches in virtual nodes
               management. We can also add and remove them dynamically as they
               are needed for analysis when Vect_net_ttb_shortest_path is called
               (problem of flattening graph).
               Currently this static solution was chosen, because it cost
               time only when graph is build. However it costs more memory
               space. For Dijkstra algorithm this expansion should not be
               serious problem because we can only get into positive node or go
               from the negative node.

             */

            ret = Vect_cat_get(Cats, tucfield, &cat);
            if (!tucfound && ret) { /* point with category of field found */
                /* find lines which belongs to the intersection */
                nnode_lns = Vect_get_node_n_lines(Map, i);

                for (i_line = 0; i_line < nnode_lns; i_line++) {

                    line_id = Vect_get_node_line(Map, i, i_line);
                    Vect_read_line(Map, NULL, ln_Cats, abs(line_id));
                    Vect_cat_get(ln_Cats, tucfield, &ln_cat);

                    if (line_id < 0)
                        ln_cat *= -1;
                    f = cat * 2;

                    if (ln_cat < 0)
                        t = ln_cat * -2 + 1;
                    else
                        t = ln_cat * 2;

                    G_debug(
                        5,
                        "Add arc %d for virtual node from %d to %d cost = %d",
                        i_virt_edge, f, t, 0);

                    /* positive, start virtual node */
                    ret = dglAddEdge(gr, (dglInt32_t)f, (dglInt32_t)t,
                                     (dglInt32_t)0, (dglInt32_t)(i_virt_edge));
                    if (ret < 0)
                        G_fatal_error(_("Cannot add network arc for virtual "
                                        "node connection."));

                    t = cat * 2 + 1;
                    i_virt_edge--;

                    if (-ln_cat < 0)
                        f = ln_cat * 2 + 1;
                    else
                        f = ln_cat * -2;

                    G_debug(
                        5,
                        "Add arc %d for virtual node from %d to %d cost = %d",
                        i_virt_edge, f, t, 0);

                    /* negative, destination virtual node */
                    ret = dglAddEdge(gr, (dglInt32_t)f, (dglInt32_t)t,
                                     (dglInt32_t)0, (dglInt32_t)(i_virt_edge));
                    if (ret < 0)
                        G_fatal_error(_("Cannot add network arc for virtual "
                                        "node connection."));

                    i_virt_edge--;
                }
                tucfound++;
            }
            else if (ret)
                tucfound++;
        }

        if (tucfound > 1)
            G_warning(_("There exists more than one point of node <%d> with "
                        "unique category field  <%d>.\n"
                        "The unique categories layer is not valid therefore "
                        "you will probably get incorrect results."),
                      tucfield, i);

        if (ncol != NULL && !cfound)
            G_debug(
                2,
                "Category of field %d  is not attached to any points in node %d"
                "(costs set to 0)",
                nfield, i);
    }

    Vect_destroy_cats_struct(ln_Cats);

    for (i = 1; i <= nturns; i++) {
        /* select points at node */

        /* TODO use cursors */
        db_CatValArray_get_value_int(&tvarrs[0], i, &turn_cat);

        db_CatValArray_get_value_int(&tvarrs[1], i, &from);
        db_CatValArray_get_value_int(&tvarrs[2], i, &to);

        db_CatValArray_get_value_int(&tvarrs[4], i, &isec);
        dcost = 0.0;
        if (ncol != NULL) {
            /* TODO optimization do not do it for every turn in intersection
             * again  */
            if (Vect_cidx_find_next(Map, tucfield_idx, isec, GV_POINT, 0, &type,
                                    &node_pt_id) == -1) {
                G_warning(
                    _("Unable to find point representing intersection <%d> in "
                      "unique categories field <%d>.\n"
                      "Cost for the intersection was set to 0.\n"
                      "The unique categories layer is not valid therefore you "
                      "will probably get incorrect results."),
                    isec, tucfield);
            }
            else {
                Vect_read_line(Map, Points, Cats, node_pt_id);

                node_pt_id = Vect_find_node(Map, *Points->x, *Points->y,
                                            *Points->z, 0.0, WITHOUT_Z);

                if (node_pt_id == 0) {
                    G_warning(
                        _("Unable to find node for point representing "
                          "intersection <%d> in unique categories field <%d>.\n"
                          "Cost for the intersection was set to 0.\n"
                          "The unique categories layer is not valid therefore "
                          "you will probably get incorrect results."),
                        isec, tucfield);
                }
                else {
                    G_debug(2, "  node = %d", node_pt_id);
                    dcost = Map->dgraph.node_costs[node_pt_id];
                }
            }
        }

        G_debug(2, "Set node's cost to %f", dcost);

        if (dcost >= 0) {
            /* Set costs from turntable */
            if (tctype[3] == DB_C_TYPE_INT) {
                ret = db_CatValArray_get_value_int(&tvarrs[3], i, &cost);
                dcost = cost;
            }
            else /* DB_C_TYPE_DOUBLE */
                ret = db_CatValArray_get_value_double(&tvarrs[3], i, &dcost);

            if (ret != DB_OK) {
                G_warning(
                    _("Database record for turn with cat = %d is not found. "
                      "(The turn was skipped."),
                    i);
                continue;
            }

            if (dcost >= 0) {

                if (ncol != NULL)
                    cost = (Map->dgraph.node_costs[node_pt_id] + dcost) *
                           (dglInt32_t)Map->dgraph.cost_multip;
                else
                    cost = dcost * (dglInt32_t)Map->dgraph.cost_multip;

                /* dglib does not like negative id's of nodes  */
                if (from < 0)
                    f = from * -2 + 1;
                else
                    f = from * 2;

                if (to < 0)
                    t = to * -2 + 1;
                else
                    t = to * 2;

                G_debug(5, "Add arc/turn %d for turn from %d to %d cost = %d",
                        turn_cat, f, t, cost);

                ret = dglAddEdge(gr, (dglInt32_t)f, (dglInt32_t)t,
                                 (dglInt32_t)cost, (dglInt32_t)(turn_cat));

                if (ret < 0)
                    G_fatal_error(
                        _("Cannot add network arc representing turn."));
            }
        }
    }

    Vect_destroy_boxlist(List);

    i = 0;
    while (tcols[i]) {
        db_CatValArray_free(&tvarrs[i]);
        ++i;
    }

    if (ncol != NULL) {
        db_close_database_shutdown_driver(driver);
        db_CatValArray_free(&fvarr);
    }

    /* Open db connection */
    if (afcol != NULL) {
        /* Get field info */
        if (afield < 1)
            G_fatal_error(_("Arc field < 1"));
        Fi = Vect_get_field(Map, afield);
        if (Fi == NULL)
            G_fatal_error(_("Database connection not defined for layer %d"),
                          afield);

        /* Open database */
        driver = db_start_driver_open_database(Fi->driver, Fi->database);
        if (driver == NULL)
            G_fatal_error(_("Unable to open database <%s> by driver <%s>"),
                          Fi->database, Fi->driver);

        /* Load costs to array */
        if (db_get_column(driver, Fi->table, afcol, &Column) != DB_OK)
            G_fatal_error(_("Column <%s> not found in table <%s>"), afcol,
                          Fi->table);

        fctype = db_sqltype_to_Ctype(db_get_column_sqltype(Column));
        db_free_column(Column);

        if (fctype != DB_C_TYPE_INT && fctype != DB_C_TYPE_DOUBLE)
            G_fatal_error(
                _("Data type of column <%s> not supported (must be numeric)"),
                afcol);

        db_CatValArray_init(&fvarr);
        nrec = db_select_CatValArray(driver, Fi->table, Fi->key, afcol, NULL,
                                     &fvarr);
        G_debug(1, "forward costs: nrec = %d", nrec);

        if (abcol != NULL) {
            if (db_get_column(driver, Fi->table, abcol, &Column) != DB_OK)
                G_fatal_error(_("Column <%s> not found in table <%s>"), abcol,
                              Fi->table);

            bctype = db_sqltype_to_Ctype(db_get_column_sqltype(Column));
            db_free_column(Column);

            if (bctype != DB_C_TYPE_INT && bctype != DB_C_TYPE_DOUBLE)
                G_fatal_error(_("Data type of column <%s> not supported (must "
                                "be numeric)"),
                              abcol);

            db_CatValArray_init(&bvarr);
            nrec = db_select_CatValArray(driver, Fi->table, Fi->key, abcol,
                                         NULL, &bvarr);
            G_debug(1, "backward costs: nrec = %d", nrec);
        }
        Vect_destroy_field_info(Fi);
    }

    skipped = 0;

    G_message(_("Registering arcs..."));

    for (i = 1; i <= nlines; i++) {
        G_percent(i, nlines, 1); /* must be before any continue */

        type = Vect_read_line(Map, Points, Cats, i);
        if (!(type & ltype & (GV_LINE | GV_BOUNDARY)))
            continue;

        Vect_get_line_nodes(Map, i, &from, &to);

        dcost = bdcost = 0;

        cfound = Vect_cat_get(Cats, tucfield, &cat);
        if (!cfound)
            continue;

        if (cfound > 1)
            G_warning(_("Line with id <%d> has more unique categories defined "
                        "in field <%d>.\n"
                        "The unique categories layer is not valid therefore "
                        "you will probably get incorrect results."),
                      i, tucfield);

        if (afcol != NULL) {
            if (!(Vect_cat_get(Cats, afield, &cat))) {
                G_debug(2,
                        "Category of field %d not attached to the line %d -> "
                        "cost was set to 0",
                        afield, i);
                skipped += 2; /* Both directions */
            }
            else {
                if (fctype == DB_C_TYPE_INT) {
                    ret = db_CatValArray_get_value_int(&fvarr, cat, &cost);
                    dcost = cost;
                }
                else { /* DB_C_TYPE_DOUBLE */
                    ret = db_CatValArray_get_value_double(&fvarr, cat, &dcost);
                }
                if (ret != DB_OK) {
                    G_warning(_("Database record for line %d (cat = %d, "
                                "forward/both direction(s)) not found "
                                "(cost was set to 0)"),
                              i, cat);
                }

                if (abcol != NULL) {
                    if (bctype == DB_C_TYPE_INT) {
                        ret = db_CatValArray_get_value_int(&bvarr, cat, &bcost);
                        bdcost = bcost;
                    }
                    else { /* DB_C_TYPE_DOUBLE */
                        ret = db_CatValArray_get_value_double(&bvarr, cat,
                                                              &bdcost);
                    }
                    if (ret != DB_OK) {
                        G_warning(_("Database record for line %d (cat = %d, "
                                    "backward direction) not found"
                                    "(cost was set to 0)"),
                                  i, cat);
                    }
                }
                else
                    bdcost = dcost;

                Vect_cat_get(Cats, tucfield, &cat);
            }
        }
        else {
            if (ll) {
                if (geo)
                    dcost = Vect_line_geodesic_length(Points);
                else
                    dcost = Vect_line_length(Points);
            }
            else
                dcost = Vect_line_length(Points);

            bdcost = dcost;
        }

        cost = (dglInt32_t)Map->dgraph.cost_multip * dcost;
        dgl_cost = cost;

        cat = cat * 2;

        G_debug(5, "Setinng node %d cost: %d", cat, cost);
        dglNodeSet_Attr(gr, dglGetNode(gr, (dglInt32_t)cat), &dgl_cost);

        Map->dgraph.edge_fcosts[i] = dcost;

        cost = (dglInt32_t)Map->dgraph.cost_multip * bdcost;
        dgl_cost = cost;

        ++cat;

        G_debug(5, "Setinng node %d cost: %d", cat, cost);
        dglNodeSet_Attr(gr, dglGetNode(gr, (dglInt32_t)cat), &dgl_cost);

        Map->dgraph.edge_bcosts[i] = bdcost;
    }

    if (afcol != NULL && skipped > 0)
        G_debug(2, "%d lines missing category of field %d skipped", skipped,
                afield);

    if (afcol != NULL) {
        db_close_database_shutdown_driver(driver);
        db_CatValArray_free(&fvarr);

        if (abcol != NULL) {
            db_CatValArray_free(&bvarr);
        }
    }
    db_close_database_shutdown_driver(ttbdriver);

    Vect_destroy_line_struct(Points);
    Vect_destroy_cats_struct(Cats);

    G_message(_("Flattening the graph..."));
    ret = dglFlatten(gr);
    if (ret < 0)
        G_fatal_error(_("GngFlatten error"));

    /* init SP cache */
    /* disable to debug dglib cache */
    dglInitializeSPCache(gr, &(Map->dgraph.spCache));

    G_message(_("Graph was built"));

    return 0;
}

/*!
   \brief Build network graph.

   Internal format for edge costs is integer, costs are multiplied
   before conversion to int by 1000 and for lengths LL without geo flag by
   1000000. The same multiplication factor is used for nodes. Costs in database
   column may be 'integer' or 'double precision' number >= 0 or -1 for infinity
   i.e. arc or node is closed and cannot be traversed If record in table is not
   found for arcs, arc is skip. If record in table is not found for node, costs
   for node are set to 0.

   \param Map vector map
   \param ltype line type for arcs
   \param afield arc costs field (if 0, use length)
   \param nfield node costs field (if 0, do not use node costs)
   \param afcol column with forward costs for arc
   \param abcol column with backward costs for arc (if NULL, back costs =
   forward costs), \param ncol column with costs for nodes (if NULL, do not use
   node costs), \param geo use geodesic calculation for length (LL), \param
   version graph version to create (1, 2, 3)

   \return 0 on success, 1 on error
 */
int Vect_net_build_graph(struct Map_info *Map, int ltype, int afield,
                         int nfield, const char *afcol, const char *abcol,
                         const char *ncol, int geo, int version)
{
    /* TODO long function, split into smaller ones */
    int i, j, from, to, line, nlines, nnodes, ret, type, cat, skipped, cfound;
    int dofw, dobw;
    struct line_pnts *Points;
    struct line_cats *Cats;
    double dcost, bdcost, ll;
    int cost, bcost;
    dglGraph_s *gr;
    dglInt32_t dgl_cost;
    dglInt32_t opaqueset[16] = {360000, 0, 0, 0, 0, 0, 0, 0,
                                0,      0, 0, 0, 0, 0, 0, 0};
    struct field_info *Fi = NULL;
    dbDriver *driver = NULL;
    dbHandle handle;
    dbString stmt;
    dbColumn *Column = NULL;
    dbCatValArray fvarr, bvarr;
    int fctype = 0, bctype = 0, nrec;

    /* TODO int costs -> double (waiting for dglib) */
    G_debug(1, "Vect_net_build_graph(): ltype = %d, afield = %d, nfield = %d",
            ltype, afield, nfield);
    G_debug(1, "    afcol = %s, abcol = %s, ncol = %s", afcol, abcol, ncol);

    G_message(_("Building graph..."));

    Map->dgraph.line_type = ltype;

    Points = Vect_new_line_struct();
    Cats = Vect_new_cats_struct();

    ll = 0;
    if (G_projection() == 3)
        ll = 1; /* LL */

    if (afcol == NULL && ll && !geo)
        Map->dgraph.cost_multip = 1000000;
    else
        Map->dgraph.cost_multip = 1000;

    nlines = Vect_get_num_lines(Map);
    nnodes = Vect_get_num_nodes(Map);

    gr = &(Map->dgraph.graph_s);

    /* Allocate space for costs, later replace by functions reading costs from
     * graph */
    Map->dgraph.edge_fcosts = (double *)G_malloc((nlines + 1) * sizeof(double));
    Map->dgraph.edge_bcosts = (double *)G_malloc((nlines + 1) * sizeof(double));
    Map->dgraph.node_costs = (double *)G_malloc((nnodes + 1) * sizeof(double));
    /* Set to -1 initially */
    for (i = 1; i <= nlines; i++) {
        Map->dgraph.edge_fcosts[i] = -1; /* forward */
        Map->dgraph.edge_bcosts[i] = -1; /* backward */
    }
    for (i = 1; i <= nnodes; i++) {
        Map->dgraph.node_costs[i] = 0;
    }

    if (version < 1 || version > 3)
        version = 1;

    if (ncol != NULL)
        dglInitialize(gr, (dglByte_t)version, sizeof(dglInt32_t), (dglInt32_t)0,
                      opaqueset);
    else
        dglInitialize(gr, (dglByte_t)version, (dglInt32_t)0, (dglInt32_t)0,
                      opaqueset);

    if (gr == NULL)
        G_fatal_error(_("Unable to build network graph"));

    db_init_handle(&handle);
    db_init_string(&stmt);

    if (abcol != NULL && afcol == NULL)
        G_fatal_error(_("Forward costs column not specified"));

    /* --- Add arcs --- */
    /* Open db connection */
    if (afcol != NULL) {
        /* Get field info */
        if (afield < 1)
            G_fatal_error(_("Arc field < 1"));
        Fi = Vect_get_field(Map, afield);
        if (Fi == NULL)
            G_fatal_error(_("Database connection not defined for layer %d"),
                          afield);

        /* Open database */
        driver = db_start_driver_open_database(Fi->driver, Fi->database);
        if (driver == NULL)
            G_fatal_error(_("Unable to open database <%s> by driver <%s>"),
                          Fi->database, Fi->driver);

        /* Load costs to array */
        if (db_get_column(driver, Fi->table, afcol, &Column) != DB_OK)
            G_fatal_error(_("Column <%s> not found in table <%s>"), afcol,
                          Fi->table);

        fctype = db_sqltype_to_Ctype(db_get_column_sqltype(Column));
        db_free_column(Column);

        if (fctype != DB_C_TYPE_INT && fctype != DB_C_TYPE_DOUBLE)
            G_fatal_error(
                _("Data type of column <%s> not supported (must be numeric)"),
                afcol);

        db_CatValArray_init(&fvarr);
        nrec = db_select_CatValArray(driver, Fi->table, Fi->key, afcol, NULL,
                                     &fvarr);
        G_debug(1, "forward costs: nrec = %d", nrec);

        if (abcol != NULL) {
            if (db_get_column(driver, Fi->table, abcol, &Column) != DB_OK)
                G_fatal_error(_("Column <%s> not found in table <%s>"), abcol,
                              Fi->table);

            bctype = db_sqltype_to_Ctype(db_get_column_sqltype(Column));
            db_free_column(Column);

            if (bctype != DB_C_TYPE_INT && bctype != DB_C_TYPE_DOUBLE)
                G_fatal_error(_("Data type of column <%s> not supported (must "
                                "be numeric)"),
                              abcol);

            db_CatValArray_init(&bvarr);
            nrec = db_select_CatValArray(driver, Fi->table, Fi->key, abcol,
                                         NULL, &bvarr);
            G_debug(1, "backward costs: nrec = %d", nrec);
        }
        Vect_destroy_field_info(Fi);
    }

    skipped = 0;

    G_message(_("Registering arcs..."));

    for (i = 1; i <= nlines; i++) {
        G_percent(i, nlines, 1); /* must be before any continue */
        dofw = dobw = 1;
        type = Vect_read_line(Map, Points, Cats, i);
        if (!(type & ltype & (GV_LINE | GV_BOUNDARY)))
            continue;

        Vect_get_line_nodes(Map, i, &from, &to);

        if (afcol != NULL) {
            if (!(Vect_cat_get(Cats, afield, &cat))) {
                G_debug(2,
                        "Category of field %d not attached to the line %d -> "
                        "line skipped",
                        afield, i);
                skipped += 2; /* Both directions */
                continue;
            }
            else {
                if (fctype == DB_C_TYPE_INT) {
                    ret = db_CatValArray_get_value_int(&fvarr, cat, &cost);
                    dcost = cost;
                }
                else { /* DB_C_TYPE_DOUBLE */
                    ret = db_CatValArray_get_value_double(&fvarr, cat, &dcost);
                }
                if (ret != DB_OK) {
                    G_warning(_("Database record for line %d (cat = %d, "
                                "forward/both direction(s)) not found "
                                "(forward/both direction(s) of line skipped)"),
                              i, cat);
                    dofw = 0;
                }

                if (abcol != NULL) {
                    if (bctype == DB_C_TYPE_INT) {
                        ret = db_CatValArray_get_value_int(&bvarr, cat, &bcost);
                        bdcost = bcost;
                    }
                    else { /* DB_C_TYPE_DOUBLE */
                        ret = db_CatValArray_get_value_double(&bvarr, cat,
                                                              &bdcost);
                    }
                    if (ret != DB_OK) {
                        G_warning(_("Database record for line %d (cat = %d, "
                                    "backward direction) not found"
                                    "(direction of line skipped)"),
                                  i, cat);
                        dobw = 0;
                    }
                }
                else {
                    if (dofw)
                        bdcost = dcost;
                    else
                        dobw = 0;
                }
            }
        }
        else {
            if (ll) {
                if (geo)
                    dcost = Vect_line_geodesic_length(Points);
                else
                    dcost = Vect_line_length(Points);
            }
            else
                dcost = Vect_line_length(Points);

            bdcost = dcost;
        }
        if (dofw && dcost != -1) {
            cost = (dglInt32_t)Map->dgraph.cost_multip * dcost;
            G_debug(5, "Add arc %d from %d to %d cost = %d", i, from, to, cost);
            ret = dglAddEdge(gr, (dglInt32_t)from, (dglInt32_t)to,
                             (dglInt32_t)cost, (dglInt32_t)i);
            Map->dgraph.edge_fcosts[i] = dcost;
            if (ret < 0)
                G_fatal_error("Cannot add network arc");
        }

        G_debug(5, "bdcost = %f edge_bcosts = %f", bdcost,
                Map->dgraph.edge_bcosts[i]);
        if (dobw && bdcost != -1) {
            bcost = (dglInt32_t)Map->dgraph.cost_multip * bdcost;
            G_debug(5, "Add arc %d from %d to %d bcost = %d", -i, to, from,
                    bcost);
            ret = dglAddEdge(gr, (dglInt32_t)to, (dglInt32_t)from,
                             (dglInt32_t)bcost, (dglInt32_t)-i);
            Map->dgraph.edge_bcosts[i] = bdcost;
            if (ret < 0)
                G_fatal_error(_("Cannot add network arc"));
        }
    }

    if (afcol != NULL && skipped > 0)
        G_debug(2, "%d lines missing category of field %d skipped", skipped,
                afield);

    if (afcol != NULL) {
        db_close_database_shutdown_driver(driver);
        db_CatValArray_free(&fvarr);

        if (abcol != NULL) {
            db_CatValArray_free(&bvarr);
        }
    }

    /* Set node attributes */
    G_debug(2, "Register nodes");
    if (ncol != NULL) {
        double x, y, z;
        struct bound_box box;
        struct boxlist *List;

        List = Vect_new_boxlist(0);

        G_debug(2, "Set nodes' costs");
        if (nfield < 1)
            G_fatal_error("Node field < 1");

        G_message(_("Setting node costs..."));

        Fi = Vect_get_field(Map, nfield);
        if (Fi == NULL)
            G_fatal_error(_("Database connection not defined for layer %d"),
                          nfield);

        driver = db_start_driver_open_database(Fi->driver, Fi->database);
        if (driver == NULL)
            G_fatal_error(_("Unable to open database <%s> by driver <%s>"),
                          Fi->database, Fi->driver);

        /* Load costs to array */
        if (db_get_column(driver, Fi->table, ncol, &Column) != DB_OK)
            G_fatal_error(_("Column <%s> not found in table <%s>"), ncol,
                          Fi->table);

        fctype = db_sqltype_to_Ctype(db_get_column_sqltype(Column));
        db_free_column(Column);

        if (fctype != DB_C_TYPE_INT && fctype != DB_C_TYPE_DOUBLE)
            G_fatal_error(
                _("Data type of column <%s> not supported (must be numeric)"),
                ncol);

        db_CatValArray_init(&fvarr);
        nrec = db_select_CatValArray(driver, Fi->table, Fi->key, ncol, NULL,
                                     &fvarr);
        G_debug(1, "node costs: nrec = %d", nrec);
        Vect_destroy_field_info(Fi);

        for (i = 1; i <= nnodes; i++) {
            /* TODO: what happens if we set attributes of not existing node
             * (skipped lines, nodes without lines) */

            /* select points at node */
            Vect_get_node_coor(Map, i, &x, &y, &z);
            box.E = box.W = x;
            box.N = box.S = y;
            box.T = box.B = z;
            Vect_select_lines_by_box(Map, &box, GV_POINT, List);

            G_debug(2, "  node = %d nlines = %d", i, List->n_values);
            cfound = 0;
            dcost = 0;

            for (j = 0; j < List->n_values; j++) {
                line = List->id[j];
                G_debug(2, "  line (%d) = %d", j, line);
                type = Vect_read_line(Map, NULL, Cats, line);
                if (!(type & GV_POINT))
                    continue;
                if (Vect_cat_get(
                        Cats, nfield,
                        &cat)) { /* point with category of field found */
                    /* Set costs */
                    if (fctype == DB_C_TYPE_INT) {
                        ret = db_CatValArray_get_value_int(&fvarr, cat, &cost);
                        dcost = cost;
                    }
                    else { /* DB_C_TYPE_DOUBLE */
                        ret = db_CatValArray_get_value_double(&fvarr, cat,
                                                              &dcost);
                    }
                    if (ret != DB_OK) {
                        G_warning(_("Database record for node %d (cat = %d) "
                                    "not found "
                                    "(cost set to 0)"),
                                  i, cat);
                    }
                    cfound = 1;
                    break;
                }
            }
            if (!cfound) {
                G_debug(
                    2,
                    "Category of field %d not attached to any points in node %d"
                    "(costs set to 0)",
                    nfield, i);
            }
            if (dcost == -1) { /* closed */
                cost = -1;
            }
            else {
                cost = (dglInt32_t)Map->dgraph.cost_multip * dcost;
            }

            dgl_cost = cost;
            G_debug(3, "Set node's cost to %d", cost);

            dglNodeSet_Attr(gr, dglGetNode(gr, (dglInt32_t)i), &dgl_cost);

            Map->dgraph.node_costs[i] = dcost;
        }
        db_close_database_shutdown_driver(driver);
        db_CatValArray_free(&fvarr);

        Vect_destroy_boxlist(List);
    }

    G_message(_("Flattening the graph..."));
    ret = dglFlatten(gr);
    if (ret < 0)
        G_fatal_error(_("GngFlatten error"));

    /* init SP cache */
    /* disable to debug dglib cache */
    dglInitializeSPCache(gr, &(Map->dgraph.spCache));

    G_message(_("Graph was built"));

    return 0;
}
