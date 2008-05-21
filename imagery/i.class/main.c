/****************************************************************************
 *
 * MODULE:       i.class
 * AUTHOR(S):    David Satnik, Central Washington University
 *                      (original contributor)
 *               Markus Neteler <neteler itc.it>,
 *               Bernhard Reiter <bernhard intevation.de>, 
 *               Brad Douglas <rez touchofmadness.com>, 
 *               Glynn Clements <glynn gclements.plus.com>, 
 *               Hamish Bowman <hamish_nospam yahoo.com>, 
 *               Jan-Oliver Wagner <jan intevation.de>
 * PURPOSE:      define training areas for supervised classification
 * COPYRIGHT:    (C) 1999-2007 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/
#define MAIN
#define GLOBAL

#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/glocale.h>
#include "globals.h"
#include "local_proto.h"


/* function prototypes */
static int check_files (char *, char *, char *, char *);


int main (int argc, char *argv[])
{
    char *mapset;
    char group[GNAME_MAX], grp_mapset[GMAPSET_MAX];

    struct Cell_head cellhd;
    struct GModule *module;
    struct Option *bg_map, *img_grp, *img_subgrp, *out_sig, *in_sig;

    /* must run in a term window */
    G_putenv("GRASS_UI_TERM","1");

    /* Initialize the gis library */
    G_gisinit(argv[0]);

    module = G_define_module();
    module->keywords = _("imagery");
    module->label =
	_("Generates spectral signatures for an image by allowing the user "
	  "to outline regions of interest.");
    module->description =
	_("The resulting signature file can be used as input for "
	  "i.maxlik or as a seed signature file for i.cluster.");

    bg_map = G_define_standard_option(G_OPT_R_MAP);
    bg_map->description =
        _("Name of raster map to be displayed");

    img_grp = G_define_standard_option (G_OPT_I_GROUP);

    img_subgrp = G_define_standard_option (G_OPT_I_GROUP);
    img_subgrp->key = "subgroup";
    img_subgrp->description  = _("Name of input imagery subgroup");

    out_sig = G_define_standard_option(G_OPT_F_OUTPUT);
    out_sig->key         = "outsig";
    out_sig->required    = YES;
    out_sig->description = _("File to contain result signatures");

    in_sig = G_define_standard_option(G_OPT_F_INPUT);
    in_sig->key         = "insig";
    in_sig->required    = NO;
    in_sig->description = _("File containing input signatures (seed)");

    if (G_parser(argc, argv) < 0)
	exit(EXIT_FAILURE);


  /* must have a graphics terminal selected */
  if (R_open_driver() != 0)
      G_fatal_error (_("No graphics device selected"));

  /* check to see if a MASK is set */
  if (G_maskfd() >= 0)
        G_fatal_error (_("You have a mask set. Unset mask and run again"));


    /* check if current mapset:  (imagery libs are very lacking in this dept)
	- abort if not,
	- remove @mapset part if it is
    */
    if(G__name_is_fully_qualified(img_grp->answer, group, grp_mapset)) {
	if(strcmp(grp_mapset, G_mapset()))
	    G_fatal_error(_("Group must exist in the current mapset"));
    }
    else {
	strcpy(group, img_grp->answer); /* FIXME for buffer overflow (have the parser check that?) */
    }

    /* get group/subgroup, and signature files */
    check_files (group, img_subgrp->answer, out_sig->answer, in_sig->answer);


  /* initialize the Region structure */
    init_region (Region);

  /* initialize the graphics */
  g_init();

  /* set up signal handling */
  set_signals();

  /* put out a title */
  display_title(VIEW_MAP1);

    mapset = G_find_cell (bg_map->answer, "");
        if (G_get_cellhd (bg_map->answer, mapset, &cellhd) != 0)
            G_fatal_error (_("Raster map <%s> not found"), bg_map->answer); 

  G_adjust_window_to_box (&cellhd, &VIEW_MAP1->cell.head, VIEW_MAP1->nrows, VIEW_MAP1->ncols);
    Configure_view (VIEW_MAP1, bg_map->answer, mapset, cellhd.ns_res, cellhd.ew_res);

  /* configure the MASK view right over the top of the map1 view */
  G_adjust_window_to_box(&cellhd, &VIEW_MASK1->cell.head, VIEW_MASK1->nrows,
			 VIEW_MASK1->ncols);
  Configure_view(VIEW_MASK1, "MASK", G_mapset(), cellhd.ns_res, cellhd.ew_res);

  draw_cell(VIEW_MAP1,OVER_WRITE);

  /* Initialize the text terminal */
  Begin_curses();
  Curses_clear_window (PROMPT_WINDOW);

  Region.saved_npoints = 0;
  
  G_set_error_routine (error);
	
  driver();

    write_signatures ();
    End_curses ();

    exit (EXIT_SUCCESS);
}


void quit (void)
{
  write_signatures();
  End_curses();
  R_close_driver();

    exit (EXIT_SUCCESS);
}


int error (const char *msg, int fatal) 
{
  char buf[200]; 
  int x,y,button;

  Curses_clear_window (PROMPT_WINDOW);
  Curses_write_window (PROMPT_WINDOW,1,1, "LOCATION:\n");
  Curses_write_window (PROMPT_WINDOW,1,12,G_location());
  Curses_write_window (PROMPT_WINDOW,2,1, "MAPSET:\n");
  Curses_write_window (PROMPT_WINDOW,2,12,G_location());

  if (fatal)
    sprintf (buf, "ERROR: %s", msg);
  else
    sprintf (buf, "WARNING: %s (click mouse to continue)", msg);
  Menu_msg (buf);
  
    if (fatal) {
        write_signatures ();
        End_curses ();

        exit (EXIT_FAILURE);
    }

  Mouse_pointer (&x, &y, &button);
  Curses_clear_window (PROMPT_WINDOW);

  return 0;
}


static int check_files (char *img_group, char *img_subgroup,
                        char *out_sig, char *in_sig)
{
    int n, any;

    I_init_group_ref (&Refer);

    I_get_subgroup_ref (img_group, img_subgroup, &Refer);

    any = 0;
    for (n = 0; n < Refer.nfiles; n++)
    {
        if (G_find_cell (Refer.file[n].name, Refer.file[n].mapset) == NULL)
        {
            if (!any)
                G_warning (_("** The following raster maps in subgroup "
                              "[%s] do not exist:"), img_subgroup);
            any = 1;
            G_message("       %s@%s", Refer.file[n].name, Refer.file[n].mapset);
        }
    }

    if (Refer.nfiles <= 0) {
        G_warning (_("Subgroup [%s] does not have any files"), img_subgroup);
        G_fatal_error (_("The subgroup must have at least 2 files to run"));
    } else if (Refer.nfiles == 1) {
        G_warning (_("Subgroup [%s] only has 1 file"), img_subgroup);
        G_fatal_error (_("The subgroup must have at least 2 files to run"));
    }

    if (G_get_cellhd(Refer.file[0].name,Refer.file[0].mapset,&Band_cellhd)!=0)
      G_fatal_error(_("Unable to read cell header for first band file"));

    /* allocate space for signature routines */
    init_sig_routines((size_t)Refer.nfiles);

    G_message(_("\nRESULT SIGNATURE"));

    if (!(outsig_fd = I_fopen_signature_file_new (img_group, img_subgroup, out_sig)))
        G_fatal_error (_("Unable to open output signature file '%s'"), out_sig );

    if (in_sig)
    {
        FILE *insig_fd;

        G_message(_("\nSEED SIGNATURES"));
        I_init_signatures (&Sigs, Refer.nfiles);

        if (!(insig_fd = I_fopen_signature_file_old (img_group, img_subgroup, in_sig)))
            G_warning (_("Unable to read signature file [%s]"), in_sig);

        if ((n = I_read_signatures (insig_fd, &Sigs)) < 0)
            G_warning (_("** Unable to read signature file [%s] **"), in_sig);

        fclose (insig_fd);

        if (Sigs.nsigs <= 255)
            return 0;

        G_warning (_("%s has too many signatures"), in_sig);
        I_free_signatures (&Sigs);
    }

    return 0;
}
