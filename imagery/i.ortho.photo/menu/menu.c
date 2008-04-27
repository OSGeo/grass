/****************************************************************************
 *
 * MODULE:       menu
 * AUTHOR(S):    Mike Baba,  DBA Systems, Inc. (original contributor)
 *               Markus Neteler <neteler itc.it>,
 *               Roberto Flor <flor itc.it>,
 *               Bernhard Reiter <bernhard intevation.de>,
 *               Glynn Clements <glynn gclements.plus.com>
 * PURPOSE:      main menu system
 * COPYRIGHT:    (C) 1999-2007 by the GRASS Development Team
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
#include "orthophoto.h"
#include "local_proto.h"

int main (int argc, char **argv)
{
    char title[80];
    char buf[80], *p;
    struct Ortho_Image_Group group;
    struct GModule *module;
    struct Option *group_opt;

    /* must run in a term window */
    G_putenv("GRASS_UI_TERM","1");

    /* initialize grass */
    G_gisinit(argv[0]);

    module = G_define_module();
    module->keywords = _("imagery");
    module->description = _("Menu driver for the photo imagery programs.");

    group_opt = G_define_standard_option(G_OPT_I_GROUP);
    group_opt->description = _("Name of imagery group for ortho-rectification");

    if (G_parser(argc, argv))
        exit(EXIT_FAILURE);


    strncpy(group.name, group_opt->answer, 99);
    group.name[99] = '\0';
    /* strip off mapset if it's there: I_() fns only work with current mapset */
    if ((p = strchr(group.name, '@')))
        *p = 0;

    /* get and check the group reference files */
    if (!I_get_group_ref (group.name, &group.group_ref))
    {
      G_warning("Pre-selected group <%s> not found.",group.name);
      /* clean the wrong name in GROUPFILE*/
      I_put_group("");

      /* ask for new group name */
      if (!I_ask_group_old ("Enter imagery group for ortho-rectification",group.name))
        exit(0);
      I_get_group_ref (group.name, &group.group_ref);
    }

    if (group.group_ref.nfiles <= 0)
        G_fatal_error ("Group [%s] contains no files\n", group.name);
    
    I_put_group(group.name);

    while (1)
    {
        if (!I_get_group(group.name)) { 
           exit(0);
        }
        
	/* print the screen full of options */ 
        sprintf (title, "i.ortho.photo -- \tImagery Group = %s ", group.name);
	G_clear_screen();

	fprintf (stderr, "%s\n\n", title);
	fprintf (stderr, "Initialization Options:\n");
	fprintf (stderr, "\n");
	fprintf (stderr, "   1.     Select/Modify imagery group\n");
	fprintf (stderr, "   2.     Select/Modify imagery group target\n");
	fprintf (stderr, "   3.     Select/Modify target elevation model\n");
	fprintf (stderr, "   4.     Select/Modify imagery group camera\n");
	fprintf (stderr, "\n");
	fprintf (stderr, "Transformation Parameter Computations:\n");
	fprintf (stderr, "\n");
	fprintf (stderr, "   5.     Compute image-to-photo transformation\n");
	fprintf (stderr, "   6.     Initialize exposure station parameters\n");
	fprintf (stderr, "   7.     Compute ortho-rectification parameters\n");
	fprintf (stderr, "\n");
	fprintf (stderr, "Ortho-rectification Option:\n");
	fprintf (stderr, "\n");
	fprintf (stderr, "   8.     Ortho-rectify imagery files\n");
	fprintf (stderr, "\n");
	fprintf (stderr, "RETURN   exit\n");
	fprintf (stderr, "\n> ");

	/* Get the option */
	if (!G_gets(buf))
	    continue;
	if (*buf == 0)    /* exit */
	    exit(0);

	/* run the program choosen */
	G_strip (buf);
	fprintf (stderr, "<%s>\n",buf);
	if (strcmp (buf, "1") == 0)
	    run_system ("i.group"); 
	if (strcmp (buf, "2") == 0)
	    run_etc_imagery ("photo.target", group.name); 
	if (strcmp (buf, "3") == 0)
	    run_etc_imagery ("photo.elev", group.name); 
	if (strcmp (buf, "4") == 0)
	    run_etc_imagery ("photo.camera", group.name);
	if (strcmp (buf, "5") == 0)
	    run_etc_imagery ("photo.2image", group.name); 
	if (strcmp (buf, "6") == 0)
	    run_etc_imagery ("photo.init", group.name); 
	if (strcmp (buf, "7") == 0)
	    run_etc_imagery ("photo.2target", group.name); 
	if (strcmp (buf, "8") == 0)
	    run_etc_imagery ("photo.rectify", group.name); 
    }
}

