/****************************************************************************
 *
 * MODULE:       g.findfile
 * AUTHOR(S):    Michael Shapiro CERL (original contributor)
 *               Markus Neteler <neteler itc.it>,
 *               Bernhard Reiter <bernhard intevation.de>, 
 *               Glynn Clements <glynn gclements.plus.com>,
 *                Jan-Oliver Wagner <jan intevation.de>
 * PURPOSE:      
 * COPYRIGHT:    (C) 1999-2006 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <grass/gis.h>
#include <grass/glocale.h>

int main( int   argc, char *argv[])
{
	char file[1024], name[GNAME_MAX], *mapset;
	char *search_mapset;
	struct GModule *module;
	struct Option *opt1 ;
	struct Option *opt2 ;
	struct Option *opt3 ;

	module = G_define_module();
	module->keywords = _("general");
	module->description =
		"Searches for GRASS data base files "
		"and sets variables for the shell.";

	G_gisinit (argv[0]);

	/* Define the different options */

	opt1 = G_define_option() ;
	opt1->key        = "element";
	opt1->type       = TYPE_STRING;
	opt1->required   = YES;
	opt1->description= "Name of an element" ;

	opt2 = G_define_option() ;
	opt2->key        = "mapset";
	opt2->type       = TYPE_STRING;
	opt2->required   = NO;
	opt2->description= "Name of a mapset" ;
	opt2->answer     = "";

	opt3 = G_define_option() ;
	opt3->key        = "file";
	opt3->type       = TYPE_STRING;
	opt3->required   = YES;
	opt3->description= "Name of an existing map" ;

	if (G_parser(argc, argv))
		exit(EXIT_FAILURE);

	search_mapset = opt2->answer ;
	if(strcmp (".", search_mapset) == 0)
	    search_mapset = G_mapset();

	if ( opt2->answer && strlen(opt2->answer) > 0 ){
	   char **map_mapset = G_tokenize(opt3->answer, "@");
	   
	   if ( G_number_of_tokens(map_mapset) > 1 ){
		if( strcmp(map_mapset[1],opt2->answer) )
		    G_fatal_error(_("Parameter 'file' contains reference to <%s> mapset, but mapset parameter <%s> does not correspond"), map_mapset[1], opt2->answer);
		else
		    strcpy (name, opt3->answer);
	   }
	   if ( G_number_of_tokens(map_mapset) == 1 )
	      strcpy (name, opt3->answer);
	   G_free_tokens(map_mapset);
	} else
	   strcpy (name, opt3->answer);

	mapset = G_find_file2 (opt1->answer, name, search_mapset);
	if (mapset)
	{
		fprintf (stdout,"name='%s'\n",name);
		fprintf (stdout,"mapset='%s'\n",mapset);
		fprintf (stdout,"fullname='%s'\n",G_fully_qualified_name(name,mapset));
		G__file_name (file, opt1->answer, name, mapset);
		fprintf (stdout,"file='%s'\n",file);
	}
	else
	{
		fprintf (stdout,"name=\n");
		fprintf (stdout,"mapset=\n");
		fprintf (stdout,"fullname=\n");
		fprintf (stdout,"file=\n");
	}
	exit(mapset==NULL);
}



