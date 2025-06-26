#include "global.h"

static int get_snap(char *, double *thresh);

int edit(struct Map_info *Map, int layer, struct Map_info **BgMap, int nbgmaps,
         struct ilist *List, enum mode action_mode,
         struct EditParams *editparams, int line)
{
    double *thresh = editparams->thresh;
    struct cat_list *Clist = NULL;
    struct line_pnts *coord = NULL;
    int ret = 0;

    if (action_mode == MODE_CATADD || action_mode == MODE_CATDEL) {
        Clist = Vect_new_cat_list();
        if (editparams->cats && Vect_str_to_cat_list(editparams->cats, Clist))
            G_fatal_error(_("Unable to get category list <%s>"),
                          editparams->cats);
    }

    if ((action_mode == MODE_BREAK || action_mode == MODE_VERTEX_ADD ||
         action_mode == MODE_VERTEX_DELETE ||
         action_mode == MODE_VERTEX_MOVE) &&
        editparams->coords) {
        coord = Vect_new_line_struct();
        str_to_coordinates(editparams->coords, coord);
    }

    switch (action_mode) {
    case MODE_ADD: {
        int num_lines;
        int snap_mode = get_snap(editparams->snap, thresh);

        if (!editparams->header)
            Vect_read_ascii_head(editparams->input, Map);

        num_lines = Vect_get_num_lines(Map);
        ret = Vect_read_ascii(editparams->input, Map);

        if (ret > 0) {
            int iline;
            struct ilist *List_added;

            G_message(n_("%d feature added", "%d features added", ret), ret);

            List_added = Vect_new_list();
            for (iline = num_lines + 1; iline <= Vect_get_num_lines(Map);
                 iline++)
                Vect_list_append(List_added, iline);

            G_verbose_message(_("Threshold value for snapping is %.2f"),
                              thresh[THRESH_SNAP]);
            if (snap_mode != NO_SNAP) { /* apply snapping */
                /* snap to vertex ? */
                Vedit_snap_lines(Map, BgMap, nbgmaps, List_added,
                                 thresh[THRESH_SNAP],
                                 snap_mode == SNAP ? FALSE : TRUE);
            }
            if (editparams->close) { /* close boundaries */
                int nclosed =
                    close_lines(Map, GV_BOUNDARY, thresh[THRESH_SNAP]);
                G_message(
                    n_("%d boundary closed", "%d boundaries closed", nclosed),
                    nclosed);
            }
            Vect_destroy_list(List_added);
        }
        break;
    }
    case MODE_DEL:
        ret = Vedit_delete_lines(Map, List);
        G_message(n_("%d feature deleted", "%d features deleted", ret), ret);
        break;
    case MODE_COPY:
        if (BgMap && BgMap[0]) {
            if (nbgmaps > 1)
                G_warning(
                    _("Multiple background maps were given. Selected "
                      "features will be copied only from vector map <%s>."),
                    Vect_get_full_name(BgMap[0]));

            ret = Vedit_copy_lines(Map, BgMap[0], List);
        }
        else
            ret = Vedit_copy_lines(Map, NULL, List);
        G_message(n_("%d feature copied", "%d features copied", ret), ret);
        break;
    case MODE_MOVE: {
        double move_x, move_y, move_z;

        if (sscanf(editparams->move, "%lf,%lf,%lf", &move_x, &move_y,
                   &move_z) != 3)
            G_fatal_error(_("'%s' tool must have '%s' column"), "move", "move");
        G_verbose_message(_("Threshold value for snapping is %.2f"),
                          thresh[THRESH_SNAP]);
        ret = Vedit_move_lines(Map, BgMap, nbgmaps, List, move_x, move_y,
                               move_z, get_snap(editparams->snap, thresh),
                               thresh[THRESH_SNAP]);
        G_message(n_("%d feature moved", "%d features moved", ret), ret);
        break;
    }
    case MODE_FLIP:
        ret = Vedit_flip_lines(Map, List);
        G_message(n_("%d line flipped", "%d lines flipped", ret), ret);
        break;
    case MODE_CATADD:
        ret = Vedit_modify_cats(Map, List, layer, 0, Clist);
        G_message(n_("%d feature modified", "%d features modified", ret), ret);
        break;
    case MODE_CATDEL:
        ret = Vedit_modify_cats(Map, List, layer, 1, Clist);
        G_message(n_("%d feature modified", "%d features modified", ret), ret);
        break;
    case MODE_MERGE:
        ret = Vedit_merge_lines(Map, List);
        G_message(n_("%d line merged", "%d lines merged", ret), ret);
        break;
    case MODE_BREAK:
        if (coord)
            ret = Vedit_split_lines(Map, List, coord, thresh[THRESH_COORDS],
                                    NULL);
        else
            ret = Vect_break_lines_list(Map, List, NULL, GV_LINES, NULL);
        G_message(n_("%d line broken", "%d lines broken", ret), ret);
        break;
    case MODE_SNAP:
        G_verbose_message(_("Threshold value for snapping is %.2f"),
                          thresh[THRESH_SNAP]);
        ret = snap_lines(Map, List, thresh[THRESH_SNAP]);
        break;
    case MODE_CONNECT:
        G_verbose_message(_("Threshold value for snapping is %.2f"),
                          thresh[THRESH_SNAP]);
        ret = Vedit_connect_lines(Map, List, thresh[THRESH_SNAP]);
        G_message(n_("%d line connected", "%d lines connected", ret), ret);
        break;
    case MODE_EXTEND:
        G_verbose_message(_("Threshold value for snapping is %.2f"),
                          thresh[THRESH_SNAP]);
        ret = Vedit_extend_lines(Map, List, 0, editparams->extend_parallel,
                                 thresh[THRESH_SNAP]);
        G_message(n_("%d line extended", "%d lines extended", ret), ret);
        break;
    case MODE_EXTEND_START:
        G_verbose_message(_("Threshold value for snapping is %.2f"),
                          thresh[THRESH_SNAP]);
        ret = Vedit_extend_lines(Map, List, 1, editparams->extend_parallel,
                                 thresh[THRESH_SNAP]);
        G_message(n_("%d line extended", "%d lines extended", ret), ret);
        break;
    case MODE_EXTEND_END:
        G_verbose_message(_("Threshold value for snapping is %.2f"),
                          thresh[THRESH_SNAP]);
        ret = Vedit_extend_lines(Map, List, 2, editparams->extend_parallel,
                                 thresh[THRESH_SNAP]);
        G_message(n_("%d line extended", "%d lines extended", ret), ret);
        break;
    case MODE_CHTYPE:
        if ((ret = Vedit_chtype_lines(Map, List)) > 0)
            G_message(n_("%d feature converted", "%d features converted", ret),
                      ret);
        else
            G_message(_("No feature modified"));
        break;
    case MODE_VERTEX_ADD:
        ret = Vedit_add_vertex(Map, List, coord, thresh[THRESH_COORDS]);
        G_message(n_("%d vertex added", "%d vertices added", ret), ret);

        break;
    case MODE_VERTEX_DELETE:
        ret = Vedit_remove_vertex(Map, List, coord, thresh[THRESH_COORDS]);
        G_message(n_("%d vertex removed", "%d vertices removed", ret), ret);
        break;
    case MODE_VERTEX_MOVE: {
        double move_x, move_y, move_z;

        if (sscanf(editparams->move, "%lf,%lf,%lf", &move_x, &move_y,
                   &move_z) != 3)
            G_fatal_error(_("'%s' tool must have '%s' column"), "vertexmove",
                          "move");
        G_verbose_message(_("Threshold value for snapping is %.2f"),
                          thresh[THRESH_SNAP]);
        ret = Vedit_move_vertex(Map, BgMap, nbgmaps, List, coord,
                                thresh[THRESH_COORDS], thresh[THRESH_SNAP],
                                move_x, move_y, move_z, editparams->move_first,
                                get_snap(editparams->snap, thresh));
        G_message(n_("%d vertex moved", "%d vertices moved", ret), ret);
        break;
    }
    case MODE_AREA_DEL: {
        int i;
        for (i = 0; i < List->n_values; i++) {
            if (Vect_get_line_type(Map, List->value[i]) != GV_CENTROID) {
                G_warning(_("Select feature %d is not centroid, ignoring..."),
                          List->value[i]);
                continue;
            }

            ret += Vedit_delete_area_centroid(Map, List->value[i]);
        }
        G_message(n_("%d area removed", "%d areas removed", ret), ret);
        break;
    }
    case MODE_ZBULK: {
        double start, step;
        double x1, y1, x2, y2;

        /* in batch editing (line > 0), check if Map is 3D; in non-batch
         * editing (line == 0), this check is done earlier in main.c */
        if (line && !Vect_is_3d(Map)) {
            Vect_close(Map);
            G_fatal_error(_("Vector map <%s> is not 3D. Tool '%s' requires "
                            "3D vector map. Please convert the vector map "
                            "to 3D using e.g. %s."),
                          Map->name, "zbulk", "v.extrude");
        }

        if (sscanf(editparams->zbulk, "%lf,%lf", &start, &step) != 2)
            G_fatal_error(_("'%s' tool must have '%s' column"), "zbulk",
                          "zbulk");

        if (!editparams->bbox || sscanf(editparams->bbox, "%lf,%lf,%lf,%lf",
                                        &x1, &y1, &x2, &y2) != 4)
            G_fatal_error(_("ZBulk must have bbox"));

        ret = Vedit_bulk_labeling(Map, List, x1, y1, x2, y2, start, step);
        G_message(n_("%d line labeled", "%d lines labeled", ret), ret);
        break;
    }
    case MODE_SELECT:
        ret = print_selected(List);
        break;
    case MODE_CREATE:
    case MODE_NONE:
    case MODE_BATCH:
        break;
    }

    if (Clist)
        Vect_destroy_cat_list(Clist);

    if (coord)
        Vect_destroy_line_struct(coord);

    return ret;
}

static int get_snap(char *snap, double *thresh)
{
    int snap_mode = NO_SNAP;

    if (snap) {
        if (strcmp(snap, "node") == 0)
            snap_mode = SNAP;
        else if (strcmp(snap, "vertex") == 0)
            snap_mode = SNAPVERTEX;
        else if (strcmp(snap, "no") != 0)
            G_fatal_error(_("Unsupported snap '%s'"), snap);
        if (snap_mode != NO_SNAP && thresh[THRESH_SNAP] <= 0) {
            G_warning(
                _("Threshold for snapping must be > 0. No snapping applied."));
            snap_mode = NO_SNAP;
        }
    }

    return snap_mode;
}
