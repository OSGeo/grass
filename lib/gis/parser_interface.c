/*!
 * \file lib/gis/parser_interface.c
 *
 * \brief GIS Library - Argument parsing functions (interface)
 *
 * (C) 2001-2009 by the GRASS Development Team
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author Original author CERL
 * \author Soeren Gebbert added Dec. 2009 WPS process_description document
 */

#include <grass/config.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <stdarg.h>
#include <sys/types.h>

#if defined(HAVE_LANGINFO_H)
#include <langinfo.h>
#endif
#if defined(__MINGW32__) && defined(USE_NLS)
#include <localcharset.h>
#endif
#ifdef HAVE_ICONV_H
#include <iconv.h>
#endif

#include <grass/gis.h>
#include <grass/glocale.h>
#include <grass/spawn.h>

#include "parser_local_proto.h"

#ifdef HAVE_ICONV_H
static const char *src_enc;
#endif

/*!
 * \brief Formats text for XML.
 *
 * \param[in,out] fp file to write to
 * \param str string to write
 */
static void print_escaped_for_xml(FILE *fp, const char *str)
{
#ifdef HAVE_ICONV_H
    iconv_t conv = iconv_open("UTF-8", src_enc);
    char *enc = NULL;

    if (conv != (iconv_t)-1) {
        char *src = (char *)str;
        size_t srclen = strlen(src);
        size_t dstlen = srclen * 4 + 1;
        char *dst = G_alloca(dstlen);
        size_t ret;

        enc = dst;

        ret = iconv(conv, (char **)&src, &srclen, &dst, &dstlen);
        if (ret != (size_t)-1 && srclen == 0) {
            str = enc;
            *dst = '\0';
        }
    }
#endif

    for (; *str; str++) {
        switch (*str) {
        case '&':
            fputs("&amp;", fp);
            break;
        case '<':
            fputs("&lt;", fp);
            break;
        case '>':
            fputs("&gt;", fp);
            break;
        default:
            fputc(*str, fp);
        }
    }

#ifdef HAVE_ICONV_H
    if (enc) {
        G_freea(enc);
    }

    if (conv != (iconv_t)-1)
        iconv_close(conv);
#endif
}

/*!
   \brief Print module usage description in XML format.
 */
void G__usage_xml(void)
{
    struct Option *opt;
    struct Flag *flag;
    char *type;
    char *s, *top;
    int i;
    const char *encoding = NULL;
    int new_prompt = 0;

    new_prompt = G__uses_new_gisprompt();

    /* gettext converts strings to encoding returned by nl_langinfo(CODESET) */

/* check if local_charset() comes from iconv. If so check for iconv library
 * before using it */
#if defined(HAVE_LANGINFO_H)
    encoding = nl_langinfo(CODESET);
#elif defined(_WIN32) && defined(USE_NLS)
    encoding = locale_charset();
#endif

    if (!encoding || strlen(encoding) == 0)
        encoding = "UTF-8";

#ifdef HAVE_ICONV_H
    src_enc = encoding;
    encoding = "UTF-8";
#endif

    if (!st->pgm_name) /* v.dave && r.michael */
        st->pgm_name = G_program_name();
    if (!st->pgm_name)
        st->pgm_name = "??";

    fprintf(stdout, "<?xml version=\"1.0\" encoding=\"%s\"?>\n", encoding);
    fprintf(stdout, "<!DOCTYPE task SYSTEM \"grass-interface.dtd\">\n");

    fprintf(stdout, "<task name=\"%s\">\n", st->pgm_name);

    if (st->module_info.label) {
        fprintf(stdout, "\t<label>\n\t\t");
        print_escaped_for_xml(stdout, st->module_info.label);
        fprintf(stdout, "\n\t</label>\n");
    }

    if (st->module_info.description) {
        fprintf(stdout, "\t<description>\n\t\t");
        print_escaped_for_xml(stdout, st->module_info.description);
        fprintf(stdout, "\n\t</description>\n");
    }

    if (st->module_info.keywords) {
        fprintf(stdout, "\t<keywords>\n\t\t");
        G__print_keywords(stdout, print_escaped_for_xml, FALSE);
        fprintf(stdout, "\n\t</keywords>\n");
    }

    /***** Don't use parameter-groups for now.  We'll reimplement this later
     ***** when we have a concept of several mutually exclusive option
     ***** groups
    if (st->n_opts || st->n_flags)
            fprintf(stdout, "\t<parameter-group>\n");
     *****
     *****
     *****/

    if (st->n_opts) {
        opt = &st->first_option;
        while (opt != NULL) {
            /* TODO: make this a enumeration type? */
            switch (opt->type) {
            case TYPE_INTEGER:
                type = "integer";
                break;
            case TYPE_DOUBLE:
                type = "float";
                break;
            case TYPE_STRING:
                type = "string";
                break;
            default:
                type = "string";
                break;
            }
            fprintf(stdout,
                    "\t<parameter "
                    "name=\"%s\" "
                    "type=\"%s\" "
                    "required=\"%s\" "
                    "multiple=\"%s\">\n",
                    opt->key, type, opt->required == YES ? "yes" : "no",
                    opt->multiple == YES ? "yes" : "no");

            if (opt->label) {
                fprintf(stdout, "\t\t<label>\n\t\t\t");
                print_escaped_for_xml(stdout, opt->label);
                fprintf(stdout, "\n\t\t</label>\n");
            }

            if (opt->description) {
                fprintf(stdout, "\t\t<description>\n\t\t\t");
                print_escaped_for_xml(stdout, opt->description);
                fprintf(stdout, "\n\t\t</description>\n");
            }

            if (opt->key_desc) {
                fprintf(stdout, "\t\t<keydesc>\n");
                top = G_calloc(strlen(opt->key_desc) + 1, 1);
                strcpy(top, opt->key_desc);
                s = strtok(top, ",");
                for (i = 1; s != NULL; i++) {
                    fprintf(stdout, "\t\t\t<item order=\"%d\">", i);
                    print_escaped_for_xml(stdout, s);
                    fprintf(stdout, "</item>\n");
                    s = strtok(NULL, ",");
                }
                fprintf(stdout, "\t\t</keydesc>\n");
                G_free(top);
            }

            if (opt->gisprompt) {
                const char *atts[] = {"age", "element", "prompt", NULL};
                top = G_calloc(strlen(opt->gisprompt) + 1, 1);
                strcpy(top, opt->gisprompt);
                s = strtok(top, ",");
                fprintf(stdout, "\t\t<gisprompt ");
                for (i = 0; s != NULL && atts[i] != NULL; i++) {
                    fprintf(stdout, "%s=\"%s\" ", atts[i], s);
                    s = strtok(NULL, ",");
                }
                fprintf(stdout, "/>\n");
                G_free(top);
            }

            if (opt->def) {
                fprintf(stdout, "\t\t<default>\n\t\t\t");
                print_escaped_for_xml(stdout, opt->def);
                fprintf(stdout, "\n\t\t</default>\n");
            }

            if (opt->options) {
                /* TODO:
                 * add something like
                 *       <range min="xxx" max="xxx"/>
                 * to <values> */
                i = 0;
                fprintf(stdout, "\t\t<values>\n");
                while (opt->opts[i]) {
                    fprintf(stdout, "\t\t\t<value>\n");
                    fprintf(stdout, "\t\t\t\t<name>");
                    print_escaped_for_xml(stdout, opt->opts[i]);
                    fprintf(stdout, "</name>\n");
                    if (opt->descs && opt->opts[i] && opt->descs[i]) {
                        fprintf(stdout, "\t\t\t\t<description>");
                        print_escaped_for_xml(stdout, opt->descs[i]);
                        fprintf(stdout, "</description>\n");
                    }
                    fprintf(stdout, "\t\t\t</value>\n");
                    i++;
                }
                fprintf(stdout, "\t\t</values>\n");
            }
            if (opt->guisection) {
                fprintf(stdout, "\t\t<guisection>\n\t\t\t");
                print_escaped_for_xml(stdout, opt->guisection);
                fprintf(stdout, "\n\t\t</guisection>\n");
            }
            if (opt->guidependency) {
                fprintf(stdout, "\t\t<guidependency>\n\t\t\t");
                print_escaped_for_xml(stdout, opt->guidependency);
                fprintf(stdout, "\n\t\t</guidependency>\n");
            }
            /* TODO:
             * - key_desc?
             * - there surely are some more. which ones?
             */

            opt = opt->next_opt;
            fprintf(stdout, "\t</parameter>\n");
        }
    }

    if (st->n_flags) {
        flag = &st->first_flag;
        while (flag != NULL) {
            fprintf(stdout, "\t<flag name=\"%c\">\n", flag->key);

            if (flag->label) {
                fprintf(stdout, "\t\t<label>\n\t\t\t");
                print_escaped_for_xml(stdout, flag->label);
                fprintf(stdout, "\n\t\t</label>\n");
            }

            if (flag->suppress_required)
                fprintf(stdout, "\t\t<suppress_required/>\n");

            if (flag->description) {
                fprintf(stdout, "\t\t<description>\n\t\t\t");
                print_escaped_for_xml(stdout, flag->description);
                fprintf(stdout, "\n\t\t</description>\n");
            }
            if (flag->guisection) {
                fprintf(stdout, " \t\t<guisection>\n\t\t\t");
                print_escaped_for_xml(stdout, flag->guisection);
                fprintf(stdout, "\n\t\t</guisection>\n");
            }
            flag = flag->next_flag;
            fprintf(stdout, "\t</flag>\n");
        }
    }

    /***** Don't use parameter-groups for now.  We'll reimplement this later
     ***** when we have a concept of several mutually exclusive option
     ***** groups
    if (st->n_opts || st->n_flags)
            fprintf(stdout, "\t</parameter-group>\n");
     *****
     *****
     *****/

    if (new_prompt) {
        /* overwrite */
        fprintf(stdout, "\t<flag name=\"%s\">\n", "overwrite");
        fprintf(stdout, "\t\t<description>\n\t\t\t");
        print_escaped_for_xml(
            stdout, _("Allow output files to overwrite existing files"));
        fprintf(stdout, "\n\t\t</description>\n");
        fprintf(stdout, "\t</flag>\n");
    }

    /* help */
    fprintf(stdout, "\t<flag name=\"%s\">\n", "help");
    fprintf(stdout, "\t\t<description>\n\t\t\t");
    print_escaped_for_xml(stdout, _("Print usage summary"));
    fprintf(stdout, "\n\t\t</description>\n");
    fprintf(stdout, "\t</flag>\n");

    /* verbose */
    fprintf(stdout, "\t<flag name=\"%s\">\n", "verbose");
    fprintf(stdout, "\t\t<description>\n\t\t\t");
    print_escaped_for_xml(stdout, _("Verbose module output"));
    fprintf(stdout, "\n\t\t</description>\n");
    fprintf(stdout, "\t</flag>\n");

    /* quiet */
    fprintf(stdout, "\t<flag name=\"%s\">\n", "quiet");
    fprintf(stdout, "\t\t<description>\n\t\t\t");
    print_escaped_for_xml(stdout, _("Quiet module output"));
    fprintf(stdout, "\n\t\t</description>\n");
    fprintf(stdout, "\t</flag>\n");

    G__describe_option_rules_xml(stdout);

    fprintf(stdout, "</task>\n");
}
