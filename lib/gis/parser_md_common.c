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

// This can eventually go to a file with functions related to Option,
// but for now it is here until parser.c is refactored.
/**
 * \brief Get number of tuple items if option is a tuple
 *
 * Note that parser code generally does not consider tuples with only one
 * item, so this function never returns 1.
 *
 * The number of items is determined by counting commas in the option key
 * description.
 *
 * \param opt Option definition
 * \return Number of items or zero if not a tuple
 */
int G__option_num_tuple_items(const struct Option *opt)
{
    // If empty, it cannot be considered a tuple.
    if (!opt->key_desc)
        return 0;

    int n_items = 1;
    const char *ptr;

    for (ptr = opt->key_desc; *ptr != '\0'; ptr++)
        if (*ptr == ',')
            n_items++;

    // Only one item is not considered a tuple.
    if (n_items == 1)
        return 0;
    // Only two and more items are a tuple.
    return n_items;
}
