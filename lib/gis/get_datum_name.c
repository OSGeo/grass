/*
 *
 ****************************************************************************
 *
 * MODULE:       GRASS 5.0 gis library, get_datum_name.c
 * AUTHOR(S):    unknown, updated by Andreas Lange, andreas.lange@rhein-main.de
 * PURPOSE:      Get datum name for new location database
 * COPYRIGHT:    (C) 2000 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *   	    	 License (>=v2). Read the file COPYING that comes with GRASS
 *   	    	 for details.
 *
 *****************************************************************************/

#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <grass/gis.h>
#include <grass/glocale.h>

/***********************************************************************
 * G_ask_datum_name(char *datumname, char *ellpsname)
 *
 * ask interactively for a valid datum name
 *
 * returns <0 on error
 * returns 1 on success
 ***********************************************************************/

/*!
 * \brief ask for a valid datum name
 *
 * This function asks the user interactively for a valid datum name from the datum
 * table. The datum name is stored in the character array pointed to by
 * <b>datum</b>. The function returns 1 on sucess, -1 if no datum was entered
 * on command line and 0 on internal error. 
 *
 *  \param datum
 *  \return int
 */

int G_ask_datum_name(char *datumname, char *ellpsname)
{ 
    char buff[1024],answer[100], ellipse[100];
    char  *dat, *Tmp_file;
    FILE  *Tmp_fd = NULL;
    int  i;


    for(;;) {
        do {
            fprintf(stderr,_("\nPlease specify datum name\n"));
            fprintf(stderr,_("Enter 'list' for the list of available datums\n"));
            fprintf(stderr,_("or 'custom' if you wish to enter custom parameters\n"));
            fprintf (stderr, _("Hit RETURN to cancel request\n"));
            fprintf(stderr,">");
        } while(!G_gets(answer));
        G_strip(answer);
       
        if(strlen(answer)==0)
            return -1;
             
        if (strcmp(answer,"list") == 0) {
            Tmp_file = G_tempfile ();
            if (NULL == (Tmp_fd = fopen (Tmp_file, "w")))
                G_warning(_("Cannot open temp file") );
            else
            {
                char *pager;

                fprintf(Tmp_fd,"Short Name\tLong Name / Description\n---\n");
                for (i=0; (dat = G_datum_name(i)); i++) {
                    fprintf(Tmp_fd,"%s\t%s\n\t\t\t(%s ellipsoid)\n---\n",
                            dat, G_datum_description(i), G_datum_ellipsoid(i));
                }
                fclose(Tmp_fd);

                pager = getenv("GRASS_PAGER");
                if (!pager || strlen(pager) == 0)
                    pager = "cat";
                sprintf(buff,"%s \"%s\" 1>&2",pager, G_convert_dirseps_to_host(Tmp_file));
                G_system(buff);

	        remove ( Tmp_file );
	    }
            G_free ( Tmp_file );
        }
        else {
            if (G_strcasecmp(answer,"custom") == 0) break; 

	    if (G_get_datum_by_name(answer) < 0) {
                fprintf(stderr,_("\ninvalid datum\n"));
            }
            else break;
        }
    }

   
    if (G_strcasecmp(answer,"custom") == 0)
    {
        /* For a custom datum we need to interactively ask for the ellipsoid */
        if(G_ask_ellipse_name(ellipse) < 0)
	    return -1;        
        sprintf(ellpsname, ellipse);
        sprintf(datumname, "custom");
    }
    else
    {
        /* else can look it up from datum.table */
        if((i = G_get_datum_by_name(answer)) < 0)
	    return -1;
        sprintf(ellpsname, G_datum_ellipsoid(i));
        sprintf(datumname, G_datum_name(i));
    }
      
    return 1;

}

