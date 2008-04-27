/****************************************************************************
 *
 * MODULE:       front end
 * AUTHOR(S):    Charles Ehlschlaeger, CERL (original contributor)
 *               Brad Douglas <rez touchofmadness.com>, Hamish Bowman <hamish_nospam yahoo.com>
 * PURPOSE:      Watershed determination
 * COPYRIGHT:    (C) 1999-2006 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/glocale.h>

int write_hist(char *, char *, char *, int);

int main(int argc, char *argv[])
{
	int err, ret;
	char command[512] ;
	struct Option *opt1 ;
	struct Option *opt2 ;
	struct Option *opt3 ;
	struct Option *opt4 ;
	struct Option *opt5 ;
	struct Option *opt6 ;
	struct Option *opt7 ;
	struct Option *opt8 ;
	struct Option *opt9 ;
	struct Option *opt10 ;
	struct Option *opt11 ;
	struct Option *opt12 ;
	struct Option *opt13 ;
	struct Option *opt14 ;
	struct Option *opt15 ;
	struct Flag *flag1 ;
	struct Flag *flag2 ;
	struct GModule *module;

	G_gisinit(argv[0]) ;
	
	/* Set description */
	module              = G_define_module();
	module->keywords = _("raster");
	module->description = _("Watershed basin analysis program.");

	opt1 = G_define_standard_option(G_OPT_R_ELEV);
	opt1->description    = _("Input map: elevation on which entire analysis is based"); 
	opt1->guisection     = _("Input_options");

	opt2 = G_define_option() ; 
	opt2->key            = "depression" ; 
	opt2->description    = _("Input map: locations of real depressions") ; 
	opt2->required       = NO ; 
	opt2->type           = TYPE_STRING ;
	opt2->gisprompt      = "old,cell,raster" ;
	opt2->guisection     = _("Input_options");

	opt3 = G_define_option() ; 
	opt3->key            = "flow" ; 
	opt3->description    = _("Input map: amount of overland flow per cell") ;
	opt3->required       = NO ; 
	opt3->type           = TYPE_STRING ; 
	opt3->gisprompt      = "old,cell,raster" ;
	opt3->guisection     = _("Input_options");

	opt4 = G_define_option() ; 
	opt4->key            = "disturbed.land" ; 
	opt4->description    = _("Input map or value: percent of disturbed land, for USLE") ; 
	opt4->required       = NO ; 
	opt4->type           = TYPE_STRING ; 
	opt4->gisprompt      = "old,cell,raster" ;
	opt4->guisection     = _("Input_options");

	opt5 = G_define_option() ; 
	opt5->key            = "blocking" ; 
	opt5->description    = _("Input map: terrain blocking overland surface flow, for USLE") ; 
	opt5->required       = NO ; 
	opt5->type           = TYPE_STRING ; 
	opt5->gisprompt      = "old,cell,raster" ;
	opt5->guisection     = _("Input_options");

	opt6 = G_define_option() ; 
	opt6->key            = "threshold" ; 
	opt6->description    = _("Input value: minimum size of exterior watershed basin") ; 
	opt6->required       = NO ; 
	opt6->type           = TYPE_INTEGER ; 
	opt6->gisprompt      = "new,cell,raster" ;
	opt6->guisection     = _("Input_options");

	opt7 = G_define_option() ; 
	opt7->key            = "max.slope.length" ; 
	opt7->description    = _("Input value: maximum length of surface flow, for USLE") ; 
	opt7->required       = NO ; 
	opt7->type           = TYPE_DOUBLE ; 
	opt7->gisprompt      = "new,cell,raster" ;
	opt7->guisection     = _("Input_options");

	opt8 = G_define_option() ; 
	opt8->key            = "accumulation" ; 
	opt8->description    = _("Output map: number of cells that drain through each cell"); 
	opt8->required       = NO ; 
	opt8->type           = TYPE_STRING ; 
	opt8->gisprompt      = "new,cell,raster" ;
	opt8->guisection     = _("Output_options");

	opt9 = G_define_option() ; 
	opt9->key            = "drainage" ; 
	opt9->description    = _("Output map: drainage direction") ; 
	opt9->required       = NO ; 
	opt9->type           = TYPE_STRING ; 
	opt9->gisprompt      = "new,cell,raster" ;
	opt9->guisection     = _("Output_options");

	opt10 = G_define_option() ; 
	opt10->key            = "basin" ; 
	opt10->description    = _("Output map: unique label for each watershed basin") ; 
	opt10->required       = NO ; 
	opt10->type           = TYPE_STRING ; 
	opt10->gisprompt      = "new,cell,raster" ;
	opt10->guisection     = _("Output_options");

	opt11 = G_define_option() ; 
	opt11->key            = "stream" ; 
	opt11->description    = _("Output map: stream segments") ; 
	opt11->required       = NO ; 
	opt11->type           = TYPE_STRING ; 
	opt11->gisprompt      = "new,cell,raster" ;
	opt11->guisection     = _("Output_options");

	opt12 = G_define_option() ; 
	opt12->key            = "half.basin" ; 
	opt12->description    = _("Output map: each half-basin is given a unique value"); 
	opt12->required       = NO ; 
	opt12->type           = TYPE_STRING ; 
	opt12->gisprompt      = "new,cell,raster" ;
	opt12->guisection     = _("Output_options");

	opt13 = G_define_option() ; 
	opt13->key            = "visual" ; 
	opt13->description    = _("Output map: useful for visual display of results") ; 
	opt13->required       = NO ; 
	opt13->type           = TYPE_STRING ; 
	opt13->gisprompt      = "new,cell,raster" ;
	opt13->guisection     = _("Output_options");

	opt14 = G_define_option() ; 
	opt14->key            = "length.slope" ; 
	opt14->description    = _("Output map: slope length and steepness (LS) factor for USLE"); 
	opt14->required       = NO ; 
	opt14->type           = TYPE_STRING ; 
	opt14->gisprompt      = "new,cell,raster" ;
	opt14->guisection     = _("Output_options");

	opt15 = G_define_option() ; 
	opt15->key            = "slope.steepness" ; 
	opt15->description    = _("Output map: slope steepness (S) factor for USLE") ; 
	opt15->required       = NO ; 
	opt15->type           = TYPE_STRING ; 
	opt15->gisprompt      = "new,cell,raster" ;
	opt15->guisection     = _("Output_options");

	flag1 = G_define_flag() ;
	flag1->key            = 'm' ;
	flag1->description    = _("Enable disk swap memory option: Operation is slow");

	flag2 = G_define_flag() ;
	flag2->key            = '4' ;
	flag2->description    = _("Allow only horizontal and vertical flow of water") ;

	if (G_parser(argc, argv))
	    exit(EXIT_FAILURE) ;

/* Check option combinations */

	/* Check for some output map */
	if (   (opt8->answer == NULL)
		&& (opt9->answer == NULL)
		&& (opt10->answer == NULL)
		&& (opt11->answer == NULL)
		&& (opt12->answer == NULL)
		&& (opt13->answer == NULL)
		&& (opt14->answer == NULL)
		&& (opt15->answer == NULL))
	{
		G_fatal_error(_("Sorry, you must choose an output map."));
	}

	err = 0 ;
	/* basin and basin.thresh */
		err += (opt10->answer != NULL && opt6->answer == NULL) ;
	/* stream and basin.thresh */
		err += (opt11->answer != NULL && opt6->answer == NULL) ;
	/* half.basin and basin.thresh */
		err += (opt12->answer != NULL && opt6->answer == NULL) ;
	/* slope and basin.thresh */
		err += (opt15->answer != NULL && opt6->answer == NULL) ;
	
	if (err)
	{
		G_message(_("Sorry, if any of the following options are set:\n"
		        "    basin, stream, half.basin, slope, or lS\n"
		        "    you MUST provide a value for the basin "
                        "threshold parameter."));
		G_usage() ;
		exit(EXIT_FAILURE);
	}

	/* Build command line */
	sprintf (command, "%s/etc/", G_gisbase()) ;

	if (flag1->answer)
	    strcat(command,"r.watershed.seg");
	else
	    strcat(command,"r.watershed.ram");

	if (flag2->answer)
		strcat(command," -4") ;

	if (opt1->answer)
	{
		strcat(command, " el=") ; strcat(command, "\"") ;
		strcat(command, opt1->answer) ; strcat(command, "\"") ;
	}
	
	if (opt2->answer)
	{
		strcat(command, " de=") ; strcat(command, "\"") ;
		strcat(command, opt2->answer) ; strcat(command, "\"") ;
	}
	
	if (opt3->answer)
	{
		strcat(command, " ov=") ; strcat(command, "\"") ;
		strcat(command, opt3->answer) ; strcat(command, "\"") ;
	}
	
	if (opt4->answer)
	{
		strcat(command, " r=") ; strcat(command, "\"") ;
		strcat(command, opt4->answer) ; strcat(command, "\"") ;
	}
	
	if (opt5->answer)
	{
		strcat(command, " ob=") ; strcat(command, "\"") ;
		strcat(command, opt5->answer) ; strcat(command, "\"") ;
	}
	
	if (opt6->answer)
	{
		strcat(command, " t=") ;
		strcat(command, opt6->answer) ;
	}
	
	if (opt7->answer)
	{
		strcat(command, " ms=") ;
		strcat(command, opt7->answer) ;
	}
	
	if (opt8->answer)
	{
		strcat(command, " ac=") ; strcat(command, "\"") ;
		strcat(command, opt8->answer) ; strcat(command, "\"") ;
	}
	
	if (opt9->answer)
	{
		strcat(command, " dr=") ; strcat(command, "\"") ;
		strcat(command, opt9->answer) ; strcat(command, "\"") ;
	}
	
	if (opt10->answer)
	{
		strcat(command, " ba=") ; strcat(command, "\"") ;
		strcat(command, opt10->answer) ; strcat(command, "\"") ;
	}
	
	if (opt11->answer)
	{
		strcat(command, " se=") ; strcat(command, "\"") ;
		strcat(command, opt11->answer) ; strcat(command, "\"") ;
	}
	
	if (opt12->answer)
	{
		strcat(command, " ha=") ; strcat(command, "\"") ;
		strcat(command, opt12->answer) ; strcat(command, "\"") ;
	}
	
	if (opt13->answer)
	{
		strcat(command, " di=") ; strcat(command, "\"") ;
		strcat(command, opt13->answer) ; strcat(command, "\"") ;
	}
	
	if (opt14->answer)
	{
		strcat(command, " LS=") ; strcat(command, "\"") ;
		strcat(command, opt14->answer) ; strcat(command, "\"") ;
	}
	
	if (opt15->answer)
	{
		strcat(command, " S=") ; strcat(command, "\"") ;
		strcat(command, opt15->answer) ; strcat(command, "\"") ;
	}

	G_debug(1, "Mode: %s", flag1->answer ? "Segmented" : "All in RAM");
	G_debug(1, "Running: %s", command);

	ret = system(command);


	/* record map metadata/history info */
	if(opt8->answer)
	    write_hist(opt8->answer,
		"Watershed accumulation: overland flow that traverses each cell",
		 opt1->answer, flag1->answer);
	if(opt9->answer)
	    write_hist(opt9->answer,
		"Watershed drainage direction (divided by 45deg)",
		 opt1->answer, flag1->answer);
	if(opt10->answer)
	    write_hist(opt10->answer,
		"Watershed basins",
		 opt1->answer, flag1->answer);
	if(opt11->answer)
	    write_hist(opt11->answer,
		"Watershed stream segments", opt1->answer, flag1->answer);
	if(opt12->answer)
	    write_hist(opt12->answer,
		"Watershed half-basins",
		 opt1->answer, flag1->answer);
	if(opt13->answer)
	    write_hist(opt13->answer,
		"Watershed visualization map (filtered accumulation map)",
		 opt1->answer, flag1->answer);
	if(opt14->answer)
	    write_hist(opt14->answer,
		"Watershed slope length and steepness (LS) factor",
		 opt1->answer, flag1->answer);
	if(opt15->answer)
	    write_hist(opt15->answer,
		"Watershed slope steepness (S) factor",
		 opt1->answer, flag1->answer);

	exit(ret);
}

/* record map history info*/
int write_hist(char *map_name, char *title, char *source_name, int mode) {
    struct History history;

    G_put_cell_title(map_name, title);

    G_short_history(map_name, "raster", &history);
    strncpy(history.datsrc_1, source_name, RECORD_LEN);
    history.datsrc_1[RECORD_LEN-1] = '\0'; /* strncpy() doesn't null terminate if maxfill */
    sprintf(history.edhist[0],
	"Processing mode: %s", mode ? "Segmented" : "All in RAM");
    history.edlinecnt = 1;
    G_command_history(&history);

    return G_write_history(map_name, &history);
}
