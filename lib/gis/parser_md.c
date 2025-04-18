/*!
   \file lib/gis/parser_md.c

   \brief GIS Library - Argument parsing functions (Markdown output)

   (C) 2012-2025 by the GRASS Development Team

   This program is free software under the GNU General Public License
   (>=v2). Read the file COPYING that comes with GRASS for details.

   \author Martin Landa
   \author Vaclav Petras
 */
#include <stdio.h>
#include <string.h>

#include <grass/gis.h>
#include <grass/glocale.h>

#include "parser_local_proto.h"

/*!
   \brief Print module usage description in Markdown format.
 */
void G__usage_markdown(void)
{
    if (!st->pgm_name)
        st->pgm_name = G_program_name();
    if (!st->pgm_name)
        st->pgm_name = "??";

    /* print metadata used by man/build*.py */
    fprintf(stdout, "---\n");
    fprintf(stdout, "name: %s\n", st->pgm_name);
    fprintf(stdout, "description: %s\n", st->module_info.description);
    fprintf(stdout, "keywords: [ ");
    G__print_keywords(stdout, NULL, FALSE);
    fprintf(stdout, " ]");
    fprintf(stdout, "\n---\n\n");

    /* main header */
    fprintf(stdout, "# %s\n\n", st->pgm_name);

    /* header */
    if (st->module_info.label)
        fprintf(stdout, "%s\n", st->module_info.label);

    if (st->module_info.description) {
        if (st->module_info.label)
            fprintf(stdout, "\n");
        fprintf(stdout, "%s\n", st->module_info.description);
    }

    const char *tab_indent = "    ";

    /* short version */
    fprintf(stdout, "\n=== \"Command line\"\n\n");
    G__md_print_cli_short_version(stdout, tab_indent);
    fprintf(stdout, "\n=== \"Python (grass.script)\"\n\n");
    G__md_print_python_short_version(stdout, tab_indent);

    fprintf(stdout, "\n## %s\n", _("Parameters"));

    /* long version */
    fprintf(stdout, "\n=== \"Command line\"\n\n");
    G__md_print_cli_long_version(stdout, tab_indent);
    fprintf(stdout, "\n=== \"Python (grass.script)\"\n\n");
    G__md_print_python_long_version(stdout, tab_indent);
}
