#include "global.h"

#define GET_FLAG(flag) (flags && strchr(flags, flag))

#define MAX_COLUMNS    13
#define NUM_TOOLS      21

#define COLUMN_TOOL    0
#define COLUMN_FLAGS   1
#define COLUMN_INPUT   2
#define COLUMN_MOVE    3
#define COLUMN_IDS     4
#define COLUMN_CATS    5
#define COLUMN_COORDS  6
#define COLUMN_BBOX    7
#define COLUMN_POLYGON 8
#define COLUMN_WHERE   9
#define COLUMN_QUERY   10
#define COLUMN_SNAP    11
#define COLUMN_ZBULK   12

static char *read_column(char *, char, char **);

int batch_edit(struct Map_info *Map, struct Map_info **BgMap, int nbgmaps,
               const char *file, char sep, struct SelectParams *selparams)
{
    char *col_names[MAX_COLUMNS] = {
        "tool", "flags",   "input", "move",  "ids",  "cats", "coords",
        "bbox", "polygon", "where", "query", "snap", "zbulk"};
    int col_nums[MAX_COLUMNS] = {
        COLUMN_TOOL,  COLUMN_FLAGS,  COLUMN_INPUT, COLUMN_MOVE,    COLUMN_IDS,
        COLUMN_CATS,  COLUMN_COORDS, COLUMN_BBOX,  COLUMN_POLYGON, COLUMN_WHERE,
        COLUMN_QUERY, COLUMN_SNAP,   COLUMN_ZBULK};
    char *tool_names[NUM_TOOLS] = {
        "add",       "delete",    "copy",        "move",      "flip",
        "catadd",    "catdel",    "merge",       "break",     "snap",
        "connect",   "extend",    "extendstart", "extendend", "chtype",
        "vertexadd", "vertexdel", "vertexmove",  "areadel",   "zbulk",
        "select"};
    enum mode tool_modes[NUM_TOOLS] = {
        MODE_ADD,           MODE_DEL,         MODE_COPY,     MODE_MOVE,
        MODE_FLIP,          MODE_CATADD,      MODE_CATDEL,   MODE_MERGE,
        MODE_BREAK,         MODE_SNAP,        MODE_CONNECT,  MODE_EXTEND,
        MODE_EXTEND_START,  MODE_EXTEND_END,  MODE_CHTYPE,   MODE_VERTEX_ADD,
        MODE_VERTEX_DELETE, MODE_VERTEX_MOVE, MODE_AREA_DEL, MODE_ZBULK,
        MODE_SELECT};
    FILE *fp;
    char buf[1024];
    int line = 0, first = 1;
    int cols[MAX_COLUMNS], ncols = 0;
    double *thresh = selparams->thresh;
    struct EditParams editparams;
    int total_ret = 0;

    editparams.thresh = thresh;

    if (strcmp(file, "-") != 0) {
        if (!(fp = fopen(file, "r")))
            G_fatal_error(_("Unable to open file <%s>"), file);
    }
    else
        fp = stdin;

    while (fgets(buf, 1024, fp)) {
        char *p, *pnext;
        char *flags, *input;
        enum mode action_mode;
        struct ilist *List;
        int ret = 0;
        int i;

        line++;

        selparams->ids = selparams->cats = selparams->coords = selparams->bbox =
            selparams->polygon = selparams->where = selparams->query = NULL;
        editparams.input = NULL;
        editparams.move = editparams.cats = editparams.coords =
            editparams.snap = editparams.zbulk = editparams.bbox = NULL;
        flags = input = NULL;

        /* remove newline */
        if ((p = strchr(buf, '\n'))) {
            *p = 0;
            if ((p = strchr(buf, '\r')))
                *p = 0;
        }
        else if ((p = strchr(buf, '\r')))
            *p = 0;

        /* find the first non-whitespace character */
        p = strchr(buf, 0);
        while (--p >= buf && (*p == ' ' || *p == '\t'))
            ;

        /* an empty line starts a new table */
        if (p < buf) {
            first = 1;
            continue;
        }

        p = buf;

        /* read batch columns */
        if (first) {
            int bit_cols = 0, bit_col;

            ncols = 0;

            while ((p = read_column(p, sep, &pnext))) {
                int known_col = 0;

                for (i = 0; i < MAX_COLUMNS; i++) {
                    if (strcmp(p, col_names[i]) == 0) {
                        bit_col = 1 << col_nums[i];
                        if (bit_cols & bit_col)
                            G_fatal_error(_("Duplicate batch column '%s' not "
                                            "allowed in line %d"),
                                          col_names[i], line);
                        bit_cols |= bit_col;
                        cols[ncols++] = col_nums[i];
                        known_col = 1;
                    }
                }
                if (!known_col)
                    G_fatal_error(_("Unknown batch column '%s' in line %d"), p,
                                  line);

                if (!pnext)
                    break;
                p = pnext;
            }

            if (!p)
                G_fatal_error(_("Illegal batch column in line %d"), line);

            if (!(bit_cols & 1 << COLUMN_TOOL))
                G_fatal_error(
                    _("Required batch column '%s' missing in line %d"),
                    col_names[COLUMN_TOOL], line);

            first = 0;
            continue;
        }

        G_message(_("Batch line %d..."), line);

        i = 0;
        while ((p = read_column(p, sep, &pnext))) {
            int j;

            if (i >= ncols)
                G_fatal_error(_("Too many batch columns in line %d"), line);

            if (*p) {
                switch (cols[i]) {
                case COLUMN_TOOL:
                    for (j = 0; j < NUM_TOOLS; j++)
                        if (strcmp(p, tool_names[j]) == 0) {
                            action_mode = tool_modes[j];
                            break;
                        }
                    if (j == NUM_TOOLS)
                        G_fatal_error(_("Unsupported tool '%s' in line %d"), p,
                                      line);
                    break;
                case COLUMN_FLAGS:
                    flags = p;
                    break;
                case COLUMN_INPUT:
                    input = p;
                    break;
                case COLUMN_MOVE:
                    editparams.move = p;
                    break;
                case COLUMN_IDS:
                    selparams->ids = p;
                    break;
                case COLUMN_CATS:
                    editparams.cats = selparams->cats = p;
                    break;
                case COLUMN_COORDS:
                    editparams.coords = selparams->coords = p;
                    break;
                case COLUMN_BBOX:
                    editparams.bbox = selparams->bbox = p;
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
                    editparams.snap = p;
                    break;
                case COLUMN_ZBULK:
                    editparams.zbulk = p;
                    break;
                }
            }

            i++;
            if (!pnext)
                break;
            p = pnext;
        }

        if (!p)
            G_fatal_error(_("Illegal batch column in line %d"), line);

        if (i < ncols)
            G_fatal_error(_("Too few batch columns in line %d"), line);

        editparams.close = action_mode == MODE_ADD && GET_FLAG('c');
        editparams.header = action_mode == MODE_ADD && GET_FLAG('n');
        editparams.move_first =
            action_mode == MODE_VERTEX_MOVE && GET_FLAG('1');
        editparams.extend_parallel =
            (action_mode == MODE_EXTEND || action_mode == MODE_EXTEND_START ||
             action_mode == MODE_EXTEND_END) &&
            GET_FLAG('p');

        List = Vect_new_list();

        selparams->reverse = GET_FLAG('r');
        if (action_mode == MODE_COPY && BgMap && BgMap[0])
            List = select_lines(BgMap[0], selparams->bglayer, action_mode,
                                selparams, List);
        else if (action_mode != MODE_ADD)
            List = select_lines(Map, selparams->layer, action_mode, selparams,
                                List);

        if (action_mode != MODE_ADD && action_mode != MODE_SELECT &&
            List->n_values < 1)
            G_warning(_("No features selected, nothing to edit"));
        else {
            if (action_mode == MODE_ADD) {
                if (!input)
                    G_fatal_error(_("'%s' tool must have '%s' column"), "add",
                                  "input");

                if (!(editparams.input = fopen(input, "r")))
                    G_fatal_error(_("Unable to open file <%s>"), input);
            }

            ret = edit(Map, selparams->layer, BgMap, nbgmaps, List, action_mode,
                       &editparams, line);

            if (action_mode == MODE_ADD && editparams.input)
                fclose(editparams.input);
        }

        G_message(" ");

        Vect_destroy_list(List);

        if (action_mode != MODE_SELECT && ret > 0) {
            Vect_build_partial(Map, GV_BUILD_NONE);
            Vect_build(Map);
            total_ret += ret;
        }
    }

    if (fp != stdin)
        fclose(fp);

    G_message(n_("%d feature edited", "%d features edited", total_ret),
              total_ret);

    return total_ret;
}

/**
   \brief Read a column from pcol string based on
   https://www.rfc-editor.org/rfc/rfc4180; Does not support multi-line columns

   \param[in] pcol pointer to the start of a new column; this buffer is modified
   \param[in] sep separater character
   \param[out] pnext pointer to the next column or NULL if last column

   \return pointer to column
   \return NULL if illegal column
*/
static char *read_column(char *pcol, char sep, char **pnext)
{
    if (*pcol == '"') {
        char *p = ++pcol;

        while ((*pnext = strchr(p, '"')) && *(*pnext + 1) == '"')
            p = *pnext + 2;

        if (*pnext) {
            char s = *(*pnext + 1);

            if (s == sep || !s) {
                /* remove closing quote */
                **pnext = 0;
                if (s == sep)
                    /* skip to next column */
                    *pnext += 2;
                else
                    /* last column */
                    *pnext = NULL;
            }
            else
                /* extra characters after last column; illegal column */
                pcol = NULL;

            /* convert "" to " */
            if ((p = pcol)) {
                while (*p) {
                    if (*p == '"') {
                        char *q;

                        for (q = p; *(q + 1); q++)
                            *q = *(q + 1);
                        *q = 0;
                    }
                    p++;
                }
            }
        }
        else
            /* closing quote is missing; illegal column */
            pcol = NULL;
    }
    else if ((*pnext = strchr(pcol, sep)))
        *(*pnext)++ = 0;
    /* else last column */

    return pcol;
}
