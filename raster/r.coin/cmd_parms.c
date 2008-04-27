/****************************************************************************
 *
 * MODULE:       r.coin
 *
 * AUTHOR(S):    Michael O'Shea - CERL
 *               Michael Shapiro - CERL
 *
 * PURPOSE:      Calculates the coincidence of two raster map layers.
 *
 * COPYRIGHT:    (C) 2006 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 ***************************************************************************/

#include <stdlib.h>
#include <string.h>
#include "coin.h"
#include <grass/gis.h>
#include <grass/glocale.h>


int 
command_version (int argc, char *argv[])
{
	struct GModule *module;
    struct
    {
	struct Option *map1, *map2, *units;
    } parm;
    struct
    {
	struct Flag *w;
    } flag;

    /* please, remove before GRASS 7 released */
    struct Flag *q_flag;

    module = G_define_module();
    module->keywords = _("raster");
    module->description =
	_("Tabulates the mutual occurrence (coincidence) "
	  "of categories for two raster map layers.");

    parm.map1 = G_define_option();
    parm.map1->key         = "map1";
    parm.map1->required    = YES;
    parm.map1->type        = TYPE_STRING;
    parm.map1->gisprompt   = "old,cell,raster" ;
    parm.map1->description = _("Name of first raster map");

    parm.map2 = G_define_option();
    parm.map2->key         = "map2";
    parm.map2->required    = YES;
    parm.map2->type        = TYPE_STRING;
    parm.map2->gisprompt   = "old,cell,raster" ;
    parm.map2->description = _("Name of second raster map");

    parm.units = G_define_option();
    parm.units->key   = "units";
    parm.units->required = YES;
    parm.units->type  = TYPE_STRING;
    parm.units->label = _("Unit of measure");
    parm.units->description = _("c(ells), p(ercent), x(percent of category [column]), "
	"y(percent of category [row]), a(cres), h(ectares), "
	"k(square kilometers), m(square miles)");
    parm.units->options = "c,p,x,y,a,h,k,m";

    flag.w = G_define_flag();
    flag.w->key = 'w';
    flag.w->description = _("Wide report, 132 columns (default: 80)");

    /* please, remove before GRASS 7 released */
    q_flag = G_define_flag() ;
    q_flag->key         = 'q' ;  
    q_flag->description = _("Run quietly") ;


    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    /* please, remove before GRASS 7 released */
    if(q_flag->answer) {
        G_putenv("GRASS_VERBOSE","0");
        G_warning(_("The '-q' flag is superseded and will be removed "
            "in future. Please use '--quiet' instead."));
    }


    strcpy (map1name, parm.map1->answer);
    strcpy (map2name, parm.map2->answer);
    mapset1 = G_find_cell2 (map1name, "");
    if(!mapset1)
	G_fatal_error (_("Raster map <%s> not found"), map1name);
    mapset2 = G_find_cell2 (map2name, "");
    if(!mapset2)
        G_fatal_error (_("Raster map <%s> not found"), map2name);

    make_coin();
    print_coin (*parm.units->answer, flag.w->answer?132:80, 0);

  exit(EXIT_SUCCESS);
}
