/****************************************************************************
 *
 * MODULE:       menu
 * AUTHOR(S):    Mike Baba,  DBA Systems, Inc. (original contributor)
 *               Markus Neteler <neteler itc.it>,
 *               Roberto Flor <flor itc.it>,
 *               Bernhard Reiter <bernhard intevation.de>,
 *               Glynn Clements <glynn gclements.plus.com>
 * PURPOSE:      main menu system
 * COPYRIGHT:    (C) 1999-2020 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/imagery.h>
#include <grass/glocale.h>
#include <grass/spawn.h>
#include "orthophoto.h"

#define BUF_SIZE 99

int main(int argc, char **argv)
{
    char *p;
    struct Ortho_Image_Group group;
    struct GModule *module;
    struct Option *group_opt, *ortho_opt;
    char *desc_ortho_opt;
    char *moduletorun;
    const char *grname;
    char tosystem[BUF_SIZE] = "";
    size_t len;

    /* initialize grass */
    G_gisinit(argv[0]);

    module = G_define_module();
    G_add_keyword(_("imagery"));
    G_add_keyword(_("orthorectify"));
    G_add_keyword(_("geometry"));
    module->description = _("Menu driver for the photo imagery programs.");

    group_opt = G_define_standard_option(G_OPT_I_GROUP);
    group_opt->required = YES;
    group_opt->description = _("Name of imagery group for ortho-rectification");

    ortho_opt = G_define_option();
    ortho_opt->key = "productname";
    ortho_opt->type = TYPE_STRING;
    ortho_opt->required = YES;
    ortho_opt->description = _("Name of Modules");
    desc_ortho_opt = NULL;
    G_asprintf(&desc_ortho_opt,
               "i.group;%s;"
               "i.ortho.target;%s;"
               "i.ortho.elev;%s;"
               "i.ortho.camera;%s;"
               "g.gui.photo2image;%s;"
               "i.ortho.init;%s;"
               "g.gui.image2target;%s;"
               "i.ortho.rectify;%s;",
               _("1 - Select/Modify imagery group"),
               _("2 - Select/Modify imagery group target"),
               _("3 - Select/Modify target elevation model"),
               _("4 - Select/Modify imagery group camera"),
               _("5 - Compute image-to-photo transformation"),
               _("6 - Initialize exposure station parameters"),
               _("7 - Compute ortho-rectification parameters"),
               _("8 - Ortho-rectify imagery files"));
    ortho_opt->descriptions = desc_ortho_opt;
    ortho_opt->options =
        "i.group,i.ortho.target,i.ortho.elev,i.ortho.camera,g.gui.photo2image,"
        "i.ortho.init,g.gui.image2target,i.ortho.rectify";

    if (G_parser(argc, argv))
        exit(EXIT_FAILURE);

    /* group validity check */

    /*----------------------*/
    len = G_strlcpy(group.name, group_opt->answer, BUF_SIZE);
    if (len >= BUF_SIZE) {
        G_fatal_error(_("Name <%s> is too long"), group_opt->answer);
    }
    /* strip off mapset if it's there: I_() fns only work with current mapset */
    if ((p = strchr(group.name, '@')))
        *p = 0;

    I_put_group(group.name);

    /*-----------------------------*/
    /* END of group validity check */
    grname = group.name;
    moduletorun = ortho_opt->answer;
    /* run the program chosen */
    if (strcmp(moduletorun, "g.gui.photo2image") == 0) {
        (void)G_strlcpy(tosystem, "g.gui.photo2image", BUF_SIZE);
        return system((const char *)tosystem);
    }
    else if (strcmp(moduletorun, "g.gui.image2target") == 0) {
        (void)G_strlcpy(tosystem, "g.gui.image2target", BUF_SIZE);
        return system((const char *)tosystem);
    }
    else {
        if (strcmp(moduletorun, "i.group") == 0)
            (void)G_strlcpy(tosystem, "i.group --ui group=", BUF_SIZE);
        if (strcmp(moduletorun, "i.ortho.target") == 0)
            (void)G_strlcpy(tosystem, "i.ortho.target --ui group=", BUF_SIZE);
        if (strcmp(moduletorun, "i.ortho.elev") == 0)
            (void)G_strlcpy(tosystem, "i.ortho.elev --ui group=", BUF_SIZE);
        if (strcmp(moduletorun, "i.ortho.camera") == 0)
            (void)G_strlcpy(tosystem, "i.ortho.camera --ui group=", BUF_SIZE);
        if (strcmp(moduletorun, "i.ortho.init") == 0)
            (void)G_strlcpy(tosystem, "i.ortho.init --ui group=", BUF_SIZE);
        if (strcmp(moduletorun, "i.ortho.rectify") == 0)
            (void)G_strlcpy(tosystem, "i.ortho.rectify --ui group=", BUF_SIZE);
        strcat(tosystem, grname);
        return system((const char *)tosystem);
    }
}
