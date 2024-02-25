#include "global.h"

#define GET_FLAG(flags, flag) ((flags) && strchr(flags, flag))

#define MAX_COLUMNS           12
#define NUM_TOOLS             20

#define COLUMN_TOOL           0
#define COLUMN_FLAGS          1
#define COLUMN_MOVE           2
#define COLUMN_IDS            3
#define COLUMN_CATS           4
#define COLUMN_COORDS         5
#define COLUMN_BBOX           6
#define COLUMN_POLYGON        7
#define COLUMN_WHERE          8
#define COLUMN_QUERY          9
#define COLUMN_SNAP           10
#define COLUMN_ZBULK          11

static int get_snap(char *, double *thresh);

int batch_edit(struct Map_info *Map, struct Map_info **BgMap, int nbgmaps,
               const char *file, struct SelectParams *selparams, double *thresh)
{
    char *col_names[MAX_COLUMNS] = {"tool",  "flags",  "move", "ids",
                                    "cats",  "coords", "bbox", "polygon",
                                    "where", "query",  "snap", "zbulk"};
    int col_nums[MAX_COLUMNS] = {COLUMN_TOOL,  COLUMN_FLAGS,   COLUMN_MOVE,
                                 COLUMN_IDS,   COLUMN_CATS,    COLUMN_COORDS,
                                 COLUMN_BBOX,  COLUMN_POLYGON, COLUMN_WHERE,
                                 COLUMN_QUERY, COLUMN_SNAP,    COLUMN_ZBULK};
    char *tool_names[NUM_TOOLS] = {
        "delete",    "copy",        "move",      "flip",   "catadd",
        "catdel",    "merge",       "break",     "snap",   "connect",
        "extend",    "extendstart", "extendend", "chtype", "vertexadd",
        "vertexdel", "vertexmove",  "areadel",   "zbulk",  "select"};
    enum mode tool_modes[NUM_TOOLS] = {
        MODE_DEL,         MODE_COPY,     MODE_MOVE,       MODE_FLIP,
        MODE_CATADD,      MODE_CATDEL,   MODE_MERGE,      MODE_BREAK,
        MODE_SNAP,        MODE_CONNECT,  MODE_EXTEND,     MODE_EXTEND_START,
        MODE_EXTEND_END,  MODE_CHTYPE,   MODE_VERTEX_ADD, MODE_VERTEX_DELETE,
        MODE_VERTEX_MOVE, MODE_AREA_DEL, MODE_ZBULK,      MODE_SELECT};
    FILE *fp;
    char buf[1024];
    int line = 0, first = 1;
    int cols[MAX_COLUMNS], ncols = 0;
    int total_ret = 0;

    if (strcmp(file, "-") != 0) {
        if (!(fp = fopen(file, "r")))
            G_fatal_error(_("Unable to open file <%s>"), file);
    }
    else
        fp = stdin;

    while (fgets(buf, 1024, fp)) {
        char *p, *psep;
        int last = 0;
        enum mode action_mode;
        char *flags, *move, *snap, *zbulk;
        struct ilist *List;
        struct line_pnts *coord = NULL;
        struct cat_list *Clist = NULL;
        int ret = 0;
        int i;

        line++;

        selparams->ids = selparams->cats = selparams->coords = selparams->bbox =
            selparams->polygon = selparams->where = selparams->query = NULL;
        flags = move = snap = zbulk = NULL;

        /* remove newline */
        if ((p = strchr(buf, '\n'))) {
            *p = 0;
            if ((p = strchr(buf, '\r')))
                *p = 0;
        }
        else if ((p = strchr(buf, '\r')))
            *p = 0;

        /* remove comments starting with # */
        if ((p = strchr(buf, '#'))) {
            *p-- = 0;
            while (p >= buf && (*p == ' ' || *p == '\t'))
                *p-- = 0;
        }

        /* skip empty line */
        if (!*buf)
            continue;

        p = buf;

        /* read batch columns */
        if (first) {
            int bit_cols = 0, bit_col;

            while (!last &&
                   ((psep = strchr(p, '|')) || (psep = strchr(p, 0)))) {
                int known_col = 0;

                last = !*psep;
                *psep = 0;
                for (i = 0; i < MAX_COLUMNS; i++) {
                    if (strcmp(p, col_names[i]) == 0) {
                        bit_col = 1 << col_nums[i];
                        if (bit_cols & bit_col)
                            G_fatal_error(
                                _("Duplicate batch column '%s' not allowed"),
                                col_names[i]);
                        bit_cols |= bit_col;
                        cols[ncols++] = col_nums[i];
                        known_col = 1;
                    }
                }
                if (!known_col)
                    G_fatal_error(_("Unknown batch column '%s'"), p);

                p = psep + 1;
            }

            if (!(bit_cols & 1 << COLUMN_TOOL))
                G_fatal_error(_("Required batch column '%s' missing"),
                              col_names[COLUMN_TOOL]);
            first = 0;
            continue;
        }

        G_message(_("Batch line %d..."), line);

        i = 0;
        while (!last && ((psep = strchr(p, '|')) || (psep = strchr(p, 0)))) {
            int j;

            last = !*psep;
            *psep = 0;

            if (i >= ncols)
                G_fatal_error(_("Too many batch columns in line %d"), line);

            switch (cols[i]) {
            case COLUMN_TOOL:
                for (j = 0; j < NUM_TOOLS; j++)
                    if (strcmp(p, tool_names[j]) == 0) {
                        action_mode = tool_modes[j];
                        break;
                    }
                if (j == NUM_TOOLS)
                    G_fatal_error(_("Unsupported tool '%s'"), p);
                break;
            case COLUMN_FLAGS:
                flags = p;
                break;
            case COLUMN_MOVE:
                move = p;
                break;
            case COLUMN_IDS:
                selparams->ids = p;
                break;
            case COLUMN_CATS:
                selparams->cats = p;
                break;
            case COLUMN_COORDS:
                selparams->coords = p;
                break;
            case COLUMN_BBOX:
                selparams->bbox = p;
                break;
            case COLUMN_POLYGON:
                selparams->polygon = p;
                break;
            case COLUMN_WHERE:
                selparams->where = p;
                break;
            case COLUMN_QUERY:
                selparams->query = p;
                break;
            case COLUMN_SNAP:
                snap = p;
                break;
            case COLUMN_ZBULK:
                zbulk = p;
                break;
            }

            i++;
            p = psep + 1;
        }

        if (i < ncols)
            G_fatal_error(_("Too few batch columns in line %d"), line);

        List = Vect_new_list();

        selparams->reverse = GET_FLAG(flags, 'r');
        if (action_mode == MODE_COPY && BgMap && BgMap[0])
            List = select_lines(BgMap[0], action_mode, selparams, thresh, List);
        else
            List = select_lines(Map, action_mode, selparams, thresh, List);

        if (selparams->cats && *selparams->cats) {
            Clist = Vect_new_cat_list();
            if (Vect_str_to_cat_list(selparams->cats, Clist))
                G_fatal_error(_("Unable to get category list <%s>"),
                              selparams->cats);
        }

        if (selparams->coords && *selparams->coords) {
            coord = Vect_new_line_struct();
            str_to_coordinates(selparams->coords, coord);
        }

        switch (action_mode) {
        case MODE_DEL:
            ret = Vedit_delete_lines(Map, List);
            G_message(n_("%d feature deleted", "%d features deleted", ret),
                      ret);
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

            if (sscanf(p, "%lf,%lf,%lf", &move_x, &move_y, &move_z) != 3)
                G_fatal_error(_("'%s' tool must have '%s' column"), "move",
                              "move");
            G_verbose_message(_("Threshold value for snapping is %.2f"),
                              thresh[THRESH_SNAP]);
            ret = Vedit_move_lines(Map, BgMap, nbgmaps, List, move_x, move_y,
                                   move_z, get_snap(snap, thresh),
                                   thresh[THRESH_SNAP]);
            G_message(n_("%d feature moved", "%d features moved", ret), ret);
            break;
        }
        case MODE_FLIP:
            ret = Vedit_flip_lines(Map, List);
            G_message(n_("%d line flipped", "%d lines flipped", ret), ret);
            break;
        case MODE_CATADD:
            ret = Vedit_modify_cats(Map, List, selparams->layer, 0, Clist);
            G_message(n_("%d feature modified", "%d features modified", ret),
                      ret);
            break;
        case MODE_CATDEL:
            ret = Vedit_modify_cats(Map, List, selparams->layer, 1, Clist);
            G_message(n_("%d feature modified", "%d features modified", ret),
                      ret);
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
            ret = Vedit_extend_lines(Map, List, 0, GET_FLAG(flags, 'p'),
                                     thresh[THRESH_SNAP]);
            G_message(n_("%d line extended", "%d lines extended", ret), ret);
            break;
        case MODE_EXTEND_START:
            G_verbose_message(_("Threshold value for snapping is %.2f"),
                              thresh[THRESH_SNAP]);
            ret = Vedit_extend_lines(Map, List, 1, GET_FLAG(flags, 'p'),
                                     thresh[THRESH_SNAP]);
            G_message(n_("%d line extended", "%d lines extended", ret), ret);
            break;
        case MODE_EXTEND_END:
            G_verbose_message(_("Threshold value for snapping is %.2f"),
                              thresh[THRESH_SNAP]);
            ret = Vedit_extend_lines(Map, List, 2, GET_FLAG(flags, 'p'),
                                     thresh[THRESH_SNAP]);
            G_message(n_("%d line extended", "%d lines extended", ret), ret);
            break;
        case MODE_CHTYPE:
            if ((ret = Vedit_chtype_lines(Map, List)) > 0)
                G_message(
                    n_("%d feature converted", "%d features converted", ret),
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

            if (sscanf(p, "%lf,%lf,%lf", &move_x, &move_y, &move_z) != 3)
                G_fatal_error(_("'%s' tool must have '%s' column"),
                              "vertexmove", "move");
            G_verbose_message(_("Threshold value for snapping is %.2f"),
                              thresh[THRESH_SNAP]);
            ret = Vedit_move_vertex(
                Map, BgMap, nbgmaps, List, coord, thresh[THRESH_COORDS],
                thresh[THRESH_SNAP], move_x, move_y, move_z,
                GET_FLAG(flags, '1'), get_snap(snap, thresh));
            G_message(n_("%d vertex moved", "%d vertices moved", ret), ret);
            break;
        }
        case MODE_AREA_DEL:
            for (i = 0; i < List->n_values; i++) {
                if (Vect_get_line_type(Map, List->value[i]) != GV_CENTROID) {
                    G_warning(
                        _("Select feature %d is not centroid, ignoring..."),
                        List->value[i]);
                    continue;
                }

                ret += Vedit_delete_area_centroid(Map, List->value[i]);
            }
            G_message(n_("%d area removed", "%d areas removed", ret), ret);

            break;
        case MODE_ZBULK: {
            double start, step;
            double x1, y1, x2, y2;

            if (!Vect_is_3d(Map)) {
                Vect_close(Map);
                G_fatal_error(_("Vector map <%s> is not 3D. Tool '%s' requires "
                                "3D vector map. Please convert the vector map "
                                "to 3D using e.g. %s."),
                              Map->name, "zbulk", "v.extrude");
            }

            if (sscanf(zbulk, "%lf,%lf", &start, &step) != 2)
                G_fatal_error(_("'%s' tool must have '%s' column"), "zbulk",
                              "zbulk");

            if (!selparams->bbox || sscanf(selparams->bbox, "%lf,%lf,%lf,%lf",
                                           &x1, &y1, &x2, &y2) != 4)
                G_fatal_error(_("ZBulk must have bbox"));

            ret = Vedit_bulk_labeling(Map, List, x1, y1, x2, y2, start, step);

            G_message(n_("%d line labeled", "%d lines labeled", ret), ret);
            break;
        }
        case MODE_SELECT:
            ret = print_selected(List);
            break;
        case MODE_BATCH:
            /* this mode */
        case MODE_CREATE:
        case MODE_ADD:
        case MODE_NONE:
            /* unsupported tools */
            break;
        }

        if (action_mode != MODE_SELECT && ret) {
            Vect_build_partial(Map, GV_BUILD_NONE);
            Vect_build(Map);
            total_ret += ret;
        }

        G_message(" ");

        Vect_destroy_list(List);

        if (coord)
            Vect_destroy_line_struct(coord);

        if (Clist)
            Vect_destroy_cat_list(Clist);
    }

    if (fp != stdin)
        fclose(fp);

    G_message(n_("%d feature edited", "%d features edited", total_ret),
              total_ret);

    return total_ret;
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
