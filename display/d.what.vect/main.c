/****************************************************************************
 *
 * MODULE:       d.what.vect
 * AUTHOR(S):    Jim Hinthorne, Central Washington U.(original contributor)
 *               Dennis Finch, National Park Service,
 *               Radim Blazek <radim.blazek gmail.com>, 
 *               Stephan Holl <sholl gmx net>, 
 *               Alex Shevlakov <sixote yahoo.com>, 
 *               Glynn Clements <glynn gclements.plus.com>, 
 *               Hamish Bowman <hamish_nospam yahoo.com>, 
 *               Markus Neteler <neteler itc.it>
 * PURPOSE:      query category values / attributes in vector maps
 * COPYRIGHT:    (C) 2002-2007 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/
/*
 *
 * attempt to auto-select vector maps displayed in monitor (like d.zoom)
 *
*/

#define MAIN
#include <stdlib.h>
#include <string.h>
#include <grass/glocale.h>
#include <grass/gis.h>
#include <grass/display.h>
#include <grass/Vect.h>
#include <grass/raster.h>
#include "what.h"
#include <grass/dbmi.h>
#include <grass/glocale.h>

/* Vector map grabbing taken from d.zoom */

int main(int argc, char **argv)
{
  struct Flag *once, *terse, *txt, *topo_flag, *flash, *edit_flag;
  struct Option *opt1;
  struct GModule *module;
  char *mapset, *openvect();
  char *str;
  int i, j, level, width = 0, mwidth = 0;
  
  /* Initialize the GIS calls */
  G_gisinit (argv[0]) ;

  module = G_define_module();
  module->keywords = _("display");
    module->description = 
    _("Allows the user to interactively query a vector map layer "
    "at user-selected locations within the current geographic region.");

  /* Conditionalize R_open_driver() so "help" works, open quiet as well */
  R__open_quiet();
  if (R_open_driver() == 0)
  {
        if(D_get_dig_list (&vect, &nvects) < 0)
            vect = NULL;
        else
        {
            vect = (char **)G_realloc(vect, (nvects+1)*sizeof(char *));
            vect[nvects] = NULL;
        }

        R_close_driver();
  }

  once = G_define_flag();
  once->key = '1';
  once->description = _("Identify just one location");
  
  opt1 = G_define_option() ;
  opt1->key        = "map" ;
  opt1->type       = TYPE_STRING ;
  opt1->multiple   = YES;
  opt1->key_desc   = "name";
  if (vect)
          opt1->answers = vect;
  opt1->required   = NO ;
  opt1->gisprompt  = "old,vector,vector" ;
  opt1->description= _("Name of existing vector map");
  
  terse = G_define_flag();
  terse->key = 't';
  terse->description = _("Terse output. For parsing by programs");
 
  txt = G_define_flag();
  txt->key = 'x';
  txt->description = _("Print information as plain text to terminal window");
  
  topo_flag = G_define_flag();
  topo_flag->key = 'd';
  topo_flag->description = _("Print topological information (debugging)");
 
  flash = G_define_flag();
  flash->key = 'f';
  flash->description = _("Enable flashing (slower)");
 
  edit_flag = G_define_flag();
  edit_flag->key = 'e';
  edit_flag->description = _("Open form in edit mode");
 
  if(!vect)
      opt1->required = YES;
    	  	      
  if((argc > 1 || !vect) && G_parser(argc,argv))
	exit(EXIT_FAILURE);

  if (opt1->answers && opt1->answers[0])
      vect = opt1->answers;

  /* Look at maps given on command line */

  if(vect)
    {
      for(i=0; vect[i]; i++);
      nvects = i;

      for(i=0; i<nvects; i++)
	{
	  mapset = openvect(vect[i]);
	  if(mapset == NULL)
	     G_fatal_error(_("Unable to open vector map <%s>"), vect[i]) ;
	}

      Map = (struct Map_info *) G_malloc(nvects * sizeof(struct Map_info));
    
      width = mwidth = 0;
      for(i=0; i<nvects; i++)
        {
          str = strchr(vect[i], '@');
          if(str) j = str - vect[i];
          else    j = strlen(vect[i]);
          if(j > width)
            width = j;
    
          mapset = openvect(vect[i]);
          j = strlen(mapset);
          if(j > mwidth)
            mwidth = j;
    
          level = Vect_open_old (&Map[i], vect[i], mapset);
          if (level < 0)
              G_fatal_error(_("Vector map <%s> not found"), vect[i]);

          if (level < 2)
              G_fatal_error(_("%s: You must build topology on vector map"), vect[i]);

	  G_message ( _("Building spatial index...") );
	  Vect_build_spatial_index ( &Map[i], stderr );
        }
    }

  if (R_open_driver() != 0)
	G_fatal_error(_("No graphics device selected"));
  D_setup(0);

  what(once->answer, txt->answer, terse->answer, flash->answer,
       width, mwidth, topo_flag->answer, edit_flag->answer); 

  for(i=0; i<nvects; i++)
      Vect_close (&Map[i]);

  R_close_driver();
  R_pad_freelist(vect, nvects);

  G_message(_("Done."));
  exit(EXIT_SUCCESS);
}






