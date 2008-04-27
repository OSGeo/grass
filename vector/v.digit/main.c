/* ***************************************************************
 * MODULE:	 v.digit
 * AUTHOR(S):	 Radim Blazek
 * PURPOSE:	 Edit vector
 * COPYRIGHT:	 (C) 2001 by the GRASS Development Team
 *		 This program is free software under the 
 *		 GNU General Public License (>=v2). 
 *		 Read the file COPYING that comes with GRASS
 *		 for details.
 **************************************************************/

#define MAIN
#include <stdio.h>
#include <stdlib.h>
#include <tcl.h>
#include <tk.h>
#include <grass/gis.h>
#include <grass/display.h>
#include <grass/raster.h>
#include <grass/Vect.h>
#include <grass/glocale.h>
#include "global.h"
#include "proto.h"

int Tcl_AppInit(Tcl_Interp* interp)
{
    int ret;

    G_debug (3, "v.digit Tcl_AppInit (...)");
    
    ret = Tcl_Init(interp);
    if (ret != TCL_OK) { return TCL_ERROR; }
    ret = Tk_Init(interp);
    if (ret != TCL_OK) { return TCL_ERROR; }

    Toolbox = interp;

    Tcl_CreateCommand(interp, "c_tool_centre", (Tcl_CmdProc*) c_tool_centre, (ClientData) NULL, 
			      (Tcl_CmdDeleteProc*) NULL);
    Tcl_CreateCommand(interp, "c_next_tool", (Tcl_CmdProc*) c_next_tool, (ClientData) NULL, 
			      (Tcl_CmdDeleteProc*) NULL);
    Tcl_CreateCommand(interp, "c_cancel", (Tcl_CmdProc*) c_cancel, (ClientData) NULL, 
			      (Tcl_CmdDeleteProc*) NULL);
    Tcl_CreateCommand(interp, "c_set_color", (Tcl_CmdProc*) c_set_color, (ClientData) NULL, 
			      (Tcl_CmdDeleteProc*) NULL);
    Tcl_CreateCommand(interp, "c_set_on", (Tcl_CmdProc*) c_set_on, (ClientData) NULL, 
			      (Tcl_CmdDeleteProc*) NULL);
    Tcl_CreateCommand(interp, "c_create_table", (Tcl_CmdProc*) c_create_table, (ClientData) NULL, 
			      (Tcl_CmdDeleteProc*) NULL);
    Tcl_CreateCommand(interp, "c_table_definition", (Tcl_CmdProc*) c_table_definition, (ClientData) NULL, 
			      (Tcl_CmdDeleteProc*) NULL);
    Tcl_CreateCommand(interp, "c_var_set", (Tcl_CmdProc*) c_var_set, (ClientData) NULL, 
			      (Tcl_CmdDeleteProc*) NULL);
    Tcl_CreateCommand(interp, "c_create_bgcmd", (Tcl_CmdProc*) c_create_bgcmd, (ClientData) NULL, 
			      (Tcl_CmdDeleteProc*) NULL);
    Tcl_CreateCommand(interp, "c_set_bgcmd", (Tcl_CmdProc*) c_set_bgcmd, (ClientData) NULL, 
			      (Tcl_CmdDeleteProc*) NULL);
    Tcl_CreateCommand(interp, "c_add_blank_bgcmd", (Tcl_CmdProc*) c_add_blank_bgcmd, (ClientData) NULL, 
			      (Tcl_CmdDeleteProc*) NULL);
    Tcl_CreateCommand(interp, "c_del_cat", (Tcl_CmdProc*) c_del_cat, (ClientData) NULL, 
			      (Tcl_CmdDeleteProc*) NULL);
    Tcl_CreateCommand(interp, "c_add_cat", (Tcl_CmdProc*) c_add_cat, (ClientData) NULL, 
			      (Tcl_CmdDeleteProc*) NULL);
    Tcl_CreateCommand(interp, "c_update_tool", (Tcl_CmdProc*) c_update_tool, (ClientData) NULL, 
			      (Tcl_CmdDeleteProc*) NULL);

    Tcl_CreateCommand(interp, "submit", (Tcl_CmdProc *) submit, (ClientData) NULL,
		              (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "set_value", (Tcl_CmdProc *) set_value, (ClientData) NULL,
		              (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "reset_values", (Tcl_CmdProc *) reset_values, (ClientData) NULL,
		              (Tcl_CmdDeleteProc *) NULL);

    Tcl_SetVar(interp, "map_mapset", Map.mapset, TCL_GLOBAL_ONLY);
    Tcl_SetVar(interp, "map_name", Map.name, TCL_GLOBAL_ONLY);

    G_debug (3, "Starting toolbox.tcl");

    return TCL_OK;
}

int main (int argc, char *argv[])
{
    int    i;
    struct GModule *module;
    struct Option *map_opt, *bgcmd_opt;
    struct Flag *new_f;
    char   *mapset;
    char   **tokens;
    char *fake_argv[4];
    char toolbox[GPATH_MAX];
    
    G_gisinit(argv[0]);

    module = G_define_module(); 
    module->keywords = _("vector, editing, digitization");
    module->description = _("Interactive editing and digitization of vector maps.");

    map_opt = G_define_standard_option(G_OPT_V_MAP);
    
    bgcmd_opt = G_define_option();
    bgcmd_opt->key = "bgcmd";
    bgcmd_opt->type =  TYPE_STRING;
    bgcmd_opt->required = NO;
    bgcmd_opt->multiple = NO;
    bgcmd_opt->answer = "";
    bgcmd_opt->description  = _("Display commands to be used for canvas backdrop (separated by ';')");
    
    new_f = G_define_flag ();
    new_f->key	     = 'n';
    new_f->description     = _("Create new file if it does not exist.");
    
    if (G_parser (argc, argv))
	exit(EXIT_FAILURE); 

    G_debug (2, "Variable = %p", Variable );
 
    /* Read background commands */
    if ( bgcmd_opt->answer ) {
	tokens = G_tokenize (bgcmd_opt->answer, ";");
	for (i = 0; tokens[i] != NULL ; i++) {
	    G_debug (2, "cmd %d : %s", i, tokens[i]);
	    bg_add ( tokens[i] ); 
	}
	G_free_tokens (tokens);
    }
    
    for ( i = 0; i < nbgcmd; i++ ) G_debug (2, "cmd %d : %s", i, Bgcmd[i].cmd);
    
    Tool_next = TOOL_NOTHING;
    G_get_window(&GRegion);
    G_debug (1, "Region: N = %f S = %f E = %f W = %f", GRegion.north,
	GRegion.south, GRegion.east, GRegion.west);
    
    /* Open map */
    mapset = G_find_vector2 (map_opt->answer, G_mapset()); 
    if ( mapset == NULL ) {
       if ( new_f->answer ) {
	   G_message(_("New empty map created."));
	   Vect_open_new (&Map, map_opt->answer, 0 );
	   Vect_build ( &Map, NULL );
	   Vect_close (&Map);
	   Vect_open_update (&Map, map_opt->answer, G_mapset());
       } else {
	   G_fatal_error(_("Map <%s> does not exist in current mapset. Add flag -n to create a new map."), map_opt->answer);
       }
    } else {
	Vect_set_open_level(2);
	Vect_open_update (&Map, map_opt->answer, mapset);
    }
    Vect_set_category_index_update ( &Map );
    Vect_hist_command ( &Map );

    G_debug (1, "Map opened");

    /* Init maximum categories */
    cat_init ();

    /* Init symbology for lines and nodes */
    symb_lines_init (); 
    symb_nodes_init (); 

    G_debug (3, "Starting Tk_Main.");
    
    /* Open toolbox */
    sprintf(toolbox, "%s/etc/v.digit/toolbox.tcl", G_gisbase());
    fake_argv[0] = argv[0];
    fake_argv[1] = "-f";
    fake_argv[2] = toolbox;
    fake_argv[3] = NULL;
    Tk_Main(3, fake_argv, Tcl_AppInit);
    
    /* Not reached */
    exit(EXIT_SUCCESS) ;
}

