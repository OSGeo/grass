/***************************************************************
 *
 * MODULE:       v.info
 *
 * AUTHOR(S):    CERL, updated to 5.7 by Markus Neteler
 *               Update to 7.0 by Martin Landa <landa.martin gmail.com> (2009)
 *               Support for level 1 by Markus Metz (2009)
 *
 * PURPOSE:      Print vector map info
 *
 * COPYRIGHT:    (C) 2002-2009, 2011 by the GRASS Development Team
 *
 *               This program is free software under the GNU General
 *               Public License (>=v2).  Read the file COPYING that
 *               comes with GRASS for details.
 *
 **************************************************************/
#include <stdlib.h>
#include <grass/gis.h>
#include <grass/vector.h>
#include <grass/glocale.h>

#include "local_proto.h"

int main(int argc, char *argv[])
{
    struct GModule *module;

    char *input_opt, *field_opt;
    int hist_flag, col_flag, print_content;

    enum OutputFormat format;

    G_JSON_Value *root_value;
    G_JSON_Object *root_object;

    struct Map_info Map;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("vector"));
    G_add_keyword(_("metadata"));
    G_add_keyword(_("topology"));
    G_add_keyword(_("extent"));
    G_add_keyword(_("history"));
    G_add_keyword(_("attribute columns"));
    G_add_keyword(_("level1"));

    module->description = _("Outputs basic information about a vector map.");

    G_debug(1, "LFS is %s", sizeof(off_t) == 8 ? "available" : "not available");

    parse_args(argc, argv, &input_opt, &field_opt, &hist_flag, &col_flag,
               &print_content, &format);

    /* try to open head-only on level 2 */
    if (Vect_open_old_head2(&Map, input_opt, "", field_opt) < 2) {
        /* force level 1, open fully
         * NOTE: number of points, lines, boundaries, centroids, faces, kernels
         * is still available */
        Vect_close(&Map);
        Vect_set_open_level(1); /* no topology */
        if (Vect_open_old2(&Map, input_opt, "", field_opt) < 1)
            G_fatal_error(_("Unable to open vector map <%s>"),
                          Vect_get_full_name(&Map));

        /* level one info not needed for history, title, columns */
        if (!hist_flag && !col_flag)
            level_one_info(&Map);
    }

    if (hist_flag || col_flag) {
        if (hist_flag) {
            print_history(&Map, format);
        }
        else if (col_flag) {
            print_columns(&Map, input_opt, field_opt, format);
        }
        Vect_close(&Map);

        return (EXIT_SUCCESS);
    }

    if (format == JSON) {
        root_value = G_json_value_init_object();
        root_object = G_json_value_get_object(root_value);
    }

    if ((print_content & PRINT_CONTENT_TEXT)) {
        print_shell(&Map, field_opt, format, root_object);
    }
    if ((print_content & PRINT_CONTENT_REGION)) {
        print_region(&Map, format, root_object);
    }
    if ((print_content & PRINT_CONTENT_TOPO)) {
        print_topo(&Map, format, root_object);
    }
    if (print_content == PRINT_CONTENT_UNSET && format == PLAIN) {
        print_info(&Map);
    }

    if (format == JSON) {
        char *serialized_string = G_json_serialize_to_string_pretty(root_value);
        if (serialized_string == NULL) {
            G_fatal_error(_("Failed to initialize pretty JSON string."));
        }
        puts(serialized_string);
        G_json_free_serialized_string(serialized_string);
        G_json_value_free(root_value);
    }

    Vect_close(&Map);

    return (EXIT_SUCCESS);
}
