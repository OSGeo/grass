/****************************************************************************
 *
 * MODULE:       i.signatures
 * AUTHOR(S):    Maris Nartiss - maris.gis gmail.com
 * PURPOSE:      Manages signature files of imagery classifiers
 *
 * COPYRIGHT:    (C) 2023 by Maris Nartiss and the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>

#include <grass/gis.h>
#include <grass/imagery.h>
#include <grass/glocale.h>

void print_inline(I_SIGFILE_TYPE, const char *);

void print_plain(const char *type, I_SIGFILE_TYPE sigtype, const char *mapset)
{
    int count;
    char **list;

    count = I_signatures_list_by_type(sigtype, mapset, &list);
    if (count == 0)
        G_warning(_("No signature files of type \"%s\" found"), type);
    else {
        printf("%s:\n", type);
        for (int i = 0; i < count; i++) {
            printf("    %s\n", list[i]);
        }
    }
    I_free_signatures_list(count, &list);
}

void print_json(const char *type, I_SIGFILE_TYPE sigtype, const char *mapset)
{
    printf("{\n");
    if (type == NULL) {
        printf("\"sig\": [");
        print_inline(I_SIGFILE_TYPE_SIG, mapset);
        printf("],\n");
        printf("\"sigset\": [");
        print_inline(I_SIGFILE_TYPE_SIGSET, mapset);
        printf("]\n");
    }
    else if (sigtype == I_SIGFILE_TYPE_SIG) {
        printf("\"sig\": [");
        print_inline(I_SIGFILE_TYPE_SIG, mapset);
        printf("]\n");
    }
    else if (sigtype == I_SIGFILE_TYPE_SIGSET) {
        printf("\"sigset\": [");
        print_inline(I_SIGFILE_TYPE_SIGSET, mapset);
        printf("]\n");
    }
    printf("}\n");
}

void print_inline(I_SIGFILE_TYPE sigtype, const char *mapset)
{
    int count, first;
    char **list;

    count = I_signatures_list_by_type(sigtype, mapset, &list);
    first = 1;
    for (int i = 0; i < count; i++) {
        if (first)
            first = 0;
        else
            printf(", ");
        printf("\"%s\"", list[i]);
    }
    I_free_signatures_list(count, &list);
}

int main(int argc, char *argv[])
{
    int print = 1;
    struct GModule *module;
    struct {
        struct Option *type, *format, *mapset, *remove, *rename, *copy;
    } parms;
    I_SIGFILE_TYPE sigtype;
    char *from, *to;

    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("imagery"));
    G_add_keyword(_("classification"));
    G_add_keyword(_("signatures"));
    module->description = _("Manage imagery classification signature files");

    parms.type = G_define_option();
    parms.type->key = "type";
    parms.type->type = TYPE_STRING;
    parms.type->key_desc = "name";
    parms.type->required = NO;
    parms.type->options = "sig,sigset";
    parms.type->guidependency = "remove,rename,copy";
    parms.type->description = _("Type of signature file");
    parms.type->guisection = _("Main");

    parms.format = G_define_option();
    parms.format->key = "format";
    parms.format->type = TYPE_STRING;
    parms.format->required = NO;
    parms.format->label = _("Output format");
    parms.format->options = "plain,json";
    parms.format->descriptions = "plain;Plain text output;"
                                 "json;JSON (JavaScript Object Notation);";
    parms.format->answer = "plain";
    parms.format->guisection = _("Print");

    parms.mapset = G_define_standard_option(G_OPT_M_MAPSET);
    parms.mapset->multiple = YES;
    parms.mapset->label = _("Name of mapset to list");
    parms.mapset->description = _("Default: current search path");
    parms.mapset->guisection = _("Print");

    parms.remove = G_define_option();
    parms.remove->key = "remove";
    parms.remove->key_desc = "name";
    parms.remove->type = TYPE_STRING;
    parms.remove->multiple = YES;
    parms.remove->gisprompt = "old,signatures,sigfile";
    parms.remove->description = _("Name of file(s) to remove");
    parms.remove->guisection = _("Files");

    parms.rename = G_define_option();
    parms.rename->key = "rename";
    parms.rename->key_desc = "from,to";
    parms.rename->type = TYPE_STRING;
    parms.rename->multiple = YES;
    parms.rename->gisprompt = "old,signatures,sigfile";
    parms.rename->description = _("Name of file to rename");
    parms.rename->guisection = _("Files");

    parms.copy = G_define_option();
    parms.copy->key = "copy";
    parms.copy->key_desc = "from,to";
    parms.copy->type = TYPE_STRING;
    parms.copy->multiple = YES;
    parms.copy->gisprompt = "old,signatures,sigfile";
    parms.copy->description = _("Name of file to copy");
    parms.copy->guisection = _("Files");

    G_option_requires(parms.copy, parms.type, NULL);
    G_option_requires(parms.rename, parms.type, NULL);
    G_option_requires(parms.remove, parms.type, NULL);

    if (G_parser(argc, argv))
        exit(EXIT_FAILURE);

    if (parms.type->answer == NULL)
        sigtype = -1;
    else if (strcmp(parms.type->answer, "sig") == 0)
        sigtype = I_SIGFILE_TYPE_SIG;
    else if (strcmp(parms.type->answer, "sigset") == 0)
        sigtype = I_SIGFILE_TYPE_SIGSET;

    if (parms.copy->answers) {
        int i = 0;
        while (parms.copy->answers[i]) {
            char sname[GNAME_MAX], smapset[GMAPSET_MAX];
            from = parms.copy->answers[i++];
            to = parms.copy->answers[i++];
            if (!G_name_is_fully_qualified(from, sname, smapset)) {
                strcpy(sname, from);
                strcpy(smapset, G_mapset());
            }
            I_signatures_copy(sigtype, sname, smapset, to);
        }
        print = 0;
    }
    if (parms.remove->answers) {
        int i = 0;
        while (parms.remove->answers[i]) {
            I_signatures_remove(sigtype, parms.remove->answers[i++]);
        }
        print = 0;
    }
    if (parms.rename->answers) {
        int i = 0;
        while (parms.rename->answers[i]) {
            from = parms.rename->answers[i++];
            to = parms.rename->answers[i++];
            I_signatures_rename(sigtype, from, to);
        }
        print = 0;
    }

    /* If no other actionable options were provided, we just print */
    if (print) {
        if (parms.type->answer)
            if (strcmp(parms.format->answer, "plain") == 0)
                print_plain(parms.type->answer, sigtype, parms.mapset->answer);
            else
                print_json(parms.type->answer, sigtype, parms.mapset->answer);
        else {
            if (strcmp(parms.format->answer, "plain") == 0) {
                print_plain("sig", I_SIGFILE_TYPE_SIG, parms.mapset->answer);
                print_plain("sigset", I_SIGFILE_TYPE_SIGSET,
                            parms.mapset->answer);
            }
            else
                print_json(NULL, sigtype, parms.mapset->answer);
        }
    }

    exit(EXIT_SUCCESS);
}
