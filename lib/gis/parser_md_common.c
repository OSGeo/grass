/*!
   \file lib/gis/parser_md_common.c

   \brief GIS Library - Argument parsing functions (Markdown output)

   (C) 2023-2025 by the GRASS Development Team

   This program is free software under the GNU General Public License
   (>=v2). Read the file COPYING that comes with GRASS for details.

   \author Martin Landa
 */
#include <stdio.h>
#include <string.h>

#include <grass/gis.h>
#include <grass/glocale.h>

#include "parser_local_proto.h"

/*!
 * \brief Format text for Markdown output
 */
#define do_escape(c, escaped) \
    case c:                   \
        fputs(escaped, f);    \
        break

void G__md_print_escaped(FILE *f, const char *str)
{
    const char *s;

    for (s = str; *s; s++) {
        switch (*s) {
            do_escape('\n', "\\\n");
            do_escape('\t', "&nbsp;&nbsp;&nbsp;&nbsp;");
            do_escape('<', "&lt;");
            do_escape('>', "&gt;");
            do_escape('*', "\\*");
        default:
            fputc(*s, f);
        }
    }
}

void G__md_print_escaped_for_options(FILE *f, const char *str)
{
    const char *s;

    for (s = str; *s; s++) {
        switch (*s) {
            do_escape('\n', "\n\n");
            do_escape(',', ", ");
        default:
            fputc(*s, f);
        }
    }
}

#undef do_escape
