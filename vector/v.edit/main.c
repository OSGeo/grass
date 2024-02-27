/****************************************************************
 *
 * MODULE:     v.edit
 *
 * PURPOSE:    Non-interactive editing vector map.
 *
 * AUTHOR(S):  Wolf Bergenheim
 *             Jachym Cepicky
 *             Major updates by Martin Landa <landa.martin gmail.com>
 *             Extend tools and batch editing by Huidae Cho <grass4u gmail.com>
 *
 * COPYRIGHT:  (C) 2006-2024 by the GRASS Development Team
 *
 *             This program is free software under the GNU General
 *             Public License (>=v2). Read the file COPYING that comes
 *             with GRASS for details.
 *
 * TODO:       3D support (done for move and vertexmove)
 ****************************************************************/

#include "global.h"

int main(int argc, char *argv[])
{
    struct GModule *module;
    struct GParams params;
    struct Map_info Map;
    struct Map_info **BgMap; /* background vector maps */
    int nbgmaps;             /* number of registrated background maps */
    enum mode action_mode;
    FILE *ascii;
    struct SelectParams selparams;
    struct EditParams editparams;
    int i;
    int ret;
    double thresh[3];
    struct ilist *List;

    ascii = NULL;
    List = NULL;
    BgMap = NULL;
    nbgmaps = 0;

    G_gisinit(argv[0]);

    module = G_define_module();
    module->overwrite = TRUE;
    G_add_keyword(_("vector"));
    G_add_keyword(_("geometry"));
    G_add_keyword(_("editing"));
    G_add_keyword(_("line"));
    G_add_keyword(_("node"));
    G_add_keyword(_("point"));
    G_add_keyword(_("vertex"));
    G_add_keyword(_("level1"));

    module->description = _("Edits a vector map, allows adding, deleting "
                            "and modifying selected vector features.");

    if (!parser(argc, argv, &params, &action_mode))
        exit(EXIT_FAILURE);

    /* open input file */
    if (params.in->answer) {
        if (strcmp(params.in->answer, "-") != 0) {
            ascii = fopen(params.in->answer, "r");
            if (ascii == NULL)
                G_fatal_error(_("Unable to open file <%s>"), params.in->answer);
        }
        else
            ascii = stdin;
    }
    if (!ascii && action_mode == MODE_ADD)
        G_fatal_error(_("Required parameter <%s> not set"), params.in->key);

    if (action_mode == MODE_CREATE) {
        int overwrite, map_type;

        overwrite = G_check_overwrite(argc, argv);
        if (G_find_vector2(params.map->answer, G_mapset()) &&
            (!G_find_file("", "OGR", G_mapset()) &&
             !G_find_file("", "PG", G_mapset()))) {
            if (!overwrite)
                G_fatal_error(_("Vector map <%s> already exists"),
                              params.map->answer);
        }

        /* 3D vector maps? */
        putenv("GRASS_VECTOR_EXTERNAL_IMMEDIATE=1");
        ret = Vect_open_new(&Map, params.map->answer, WITHOUT_Z);
        if (ret == -1)
            G_fatal_error(_("Unable to create vector map <%s>"),
                          params.map->answer);
        Vect_set_error_handler_io(NULL, &Map);

        /* native or external data source ? */
        map_type = Vect_maptype(&Map);
        if (map_type != GV_FORMAT_NATIVE) {
            int type;

            type = Vect_option_to_types(params.type);
            if (type != GV_POINT && !(type & GV_LINES))
                G_fatal_error(
                    "%s: point,line,boundary",
                    _("Supported feature types for non-native formats:"));
            /* create OGR or PostGIS layer */
            if (Vect_write_line(&Map, type, NULL, NULL) < 0)
                G_fatal_error(_("Unable to create vector map <%s>"),
                              params.map->answer);
        }

        G_debug(1, "Map created");

        if (ascii) /* also add new vector features */
            action_mode = MODE_ADD;
    }
    else {                           /* open selected vector file */
        if (action_mode == MODE_ADD) /* write */
            ret = Vect_open_update2(&Map, params.map->answer, G_mapset(),
                                    params.fld->answer);
        else /* read-only -- select features */
            ret = Vect_open_old2(&Map, params.map->answer, G_mapset(),
                                 params.fld->answer);

        if (ret < 2)
            G_fatal_error(
                _("Unable to open vector map <%s> on topological level. "
                  "Try to rebuild vector topology by v.build."),
                params.map->answer);
    }

    G_debug(1, "Map opened");

    /* open background maps */
    if (params.bmaps->answer) {
        i = 0;

        while (params.bmaps->answers[i]) {
            const char *bmap = params.bmaps->answers[i];
            const char *mapset = G_find_vector2(bmap, "");

            if (!mapset)
                G_fatal_error(_("Vector map <%s> not found"), bmap);

            if (strcmp(G_fully_qualified_name(params.map->answer, G_mapset()),
                       G_fully_qualified_name(bmap, mapset)) == 0) {
                G_fatal_error(
                    _("Unable to open vector map <%s> as the background map. "
                      "It is given as vector map to be edited."),
                    bmap);
            }
            nbgmaps++;
            BgMap = (struct Map_info **)G_realloc(
                BgMap, nbgmaps * sizeof(struct Map_info *));
            BgMap[nbgmaps - 1] =
                (struct Map_info *)G_malloc(sizeof(struct Map_info));
            if (Vect_open_old(BgMap[nbgmaps - 1], bmap, "") == -1)
                G_fatal_error(_("Unable to open vector map <%s>"), bmap);
            G_verbose_message(_("Background vector map <%s> registered"), bmap);
            i++;
        }
    }

    i = 0;
    while (params.maxdist->answers[i]) {
        switch (i) {
        case THRESH_COORDS:
            thresh[THRESH_COORDS] =
                max_distance(atof(params.maxdist->answers[THRESH_COORDS]));
            thresh[THRESH_SNAP] = thresh[THRESH_QUERY] = thresh[THRESH_COORDS];
            break;
        case THRESH_SNAP:
            thresh[THRESH_SNAP] =
                max_distance(atof(params.maxdist->answers[THRESH_SNAP]));
            break;
        case THRESH_QUERY:
            thresh[THRESH_QUERY] = atof(params.maxdist->answers[THRESH_QUERY]);
            break;
        default:
            break;
        }
        i++;
    }

    if (action_mode == MODE_BATCH) {
        char *sep = G_option_to_separator(params.sep);

        if (*(sep + 1))
            G_fatal_error(_("Field separator must be a single character"));
        else if (*sep == '\n')
            G_fatal_error(_("Field separator cannot be a newline"));

        if (Vect_open_update2(&Map, params.map->answer, G_mapset(),
                              params.fld->answer) < 0)
            G_fatal_error(_("Unable to open vector map <%s>"),
                          params.map->answer);
        Vect_set_error_handler_io(&Map, NULL);

        selparams.layer = Vect_get_field_number(&Map, params.fld->answer);
        if (BgMap && BgMap[0])
            selparams.bglayer =
                Vect_get_field_number(BgMap[0], params.fld->answer);
        selparams.type = Vect_option_to_types(params.type);
        selparams.thresh = thresh;

        batch_edit(&Map, BgMap, nbgmaps, params.batch->answer, *sep,
                   &selparams);
    }
    else {
        if (action_mode != MODE_CREATE && action_mode != MODE_ADD) {
            /* select lines */
            if (action_mode == MODE_COPY && BgMap && BgMap[0])
                selparams.bglayer =
                    Vect_get_field_number(BgMap[0], params.fld->answer);
            else
                selparams.layer =
                    Vect_get_field_number(&Map, params.fld->answer);
            selparams.type = Vect_option_to_types(params.type);
            selparams.reverse = params.reverse->answer;
            selparams.ids = params.id->answer;
            selparams.cats = params.cat->answer;
            selparams.coords = params.coord->answer;
            selparams.bbox = params.bbox->answer;
            selparams.polygon = params.poly->answer;
            selparams.where = params.where->answer;
            selparams.query = params.query->answer;
            selparams.thresh = thresh;

            List = Vect_new_list();
            if (action_mode == MODE_COPY && BgMap && BgMap[0])
                List = select_lines(BgMap[0], selparams.bglayer, action_mode,
                                    &selparams, List);
            else
                List = select_lines(&Map, selparams.layer, action_mode,
                                    &selparams, List);
        }

        if ((action_mode != MODE_CREATE && action_mode != MODE_ADD &&
             action_mode != MODE_SELECT)) {
            if (List->n_values < 1) {
                G_warning(_("No features selected, nothing to edit"));
                action_mode = MODE_NONE;
                ret = 0;
            }
            else {
                /* reopen the map for updating */
                if (action_mode == MODE_ZBULK && !Vect_is_3d(&Map)) {
                    Vect_close(&Map);
                    G_fatal_error(
                        _("Vector map <%s> is not 3D. Tool '%s' requires "
                          "3D vector map. "
                          "Please convert the vector map "
                          "to 3D using e.g. %s."),
                        params.map->answer, params.tool->answer, "v.extrude");
                }
                Vect_close(&Map);

                if (Vect_open_update2(&Map, params.map->answer, G_mapset(),
                                      params.fld->answer) < 0)
                    G_fatal_error(_("Unable to open vector map <%s>"),
                                  params.map->answer);
            }
        }

        if (action_mode != MODE_NONE) {
            editparams.input = ascii;
            editparams.move = params.move->answer;
            editparams.cats = params.cat->answer;
            editparams.coords = params.coord->answer;
            editparams.snap = params.snap->answer;
            editparams.zbulk = params.zbulk->answer;
            editparams.bbox = params.bbox->answer;
            editparams.thresh = thresh;
            editparams.close = params.close->answer ? 1 : 0;
            editparams.header = params.header->answer ? 1 : 0;
            editparams.move_first = params.move_first->answer ? 1 : 0;
            editparams.extend_parallel = params.extend_parallel->answer ? 1 : 0;

            /* perform requested editation */
            ret = edit(&Map, selparams.layer, BgMap, nbgmaps, List, action_mode,
                       &editparams, 0);

            /* build topology only if requested or if tool!=select */
            if (action_mode != MODE_SELECT && ret > 0 &&
                params.topo->answer != 1) {
                Vect_build_partial(&Map, GV_BUILD_NONE);
                Vect_build(&Map);
            }
        }
    }

    if (List)
        Vect_destroy_list(List);

    if (ascii && ascii != stdout)
        fclose(ascii);

    Vect_hist_command(&Map);
    Vect_close(&Map);

    G_debug(1, "Map closed");

    /* close background maps */
    for (i = 0; i < nbgmaps; i++) {
        Vect_close(BgMap[i]);
        G_free((void *)BgMap[i]);
    }
    G_free((void *)BgMap);

    G_done_msg(" ");

    exit(ret > -1 ? EXIT_SUCCESS : EXIT_FAILURE);
}
