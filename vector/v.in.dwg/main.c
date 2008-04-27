/* **************************************************************
 * 
 *  MODULE:       v.in.dwg
 *  
 *  AUTHOR(S):    Radim Blazek
 *                
 *  PURPOSE:      Import of DWG/DXF files
 *                
 *  COPYRIGHT:    (C) 2001 by the GRASS Development Team
 * 
 *                This program is free software under the 
 *                GNU General Public License (>=v2). 
 *                Read the file COPYING that comes with GRASS
 *                for details.
 * 
 * In addition, as a special exception, Radim Blazek gives permission
 * to link the code of this program with the OpenDWG libraries (or with
 * modified versions of the OpenDWG libraries that use the same license
 * as OpenDWG libraries), and distribute linked combinations including the two.
 * You must obey the GNU General Public License in all respects for all
 * of the code used other than. If you modify this file, you may extend
 * this exception to your version of the file, but you are not obligated
 * to do so. If you do not wish to do so, delete this exception statement
 * from your version.
 * 
 * **************************************************************/
#define AD_PROTOTYPES
#define AD_VM_PC
#define OD_GENERIC_READ

#define MAIN

#include <stdio.h>
#include <stdlib.h> 
#include <string.h> 
#include <math.h>
#include <fcntl.h>
#include <unistd.h> 
#include <grass/gis.h>
#include <grass/dbmi.h>
#include <grass/Vect.h>
#include <grass/glocale.h>
#include "ad2.h"
#include "io/odio.h"
#include "global.h"

int 
main (int argc, char *argv[])
{
    struct GModule *module;
    struct Option *out_opt, *in_opt;
    struct Flag *z_flag, *circle_flag, *l_flag, *int_flag;
    char   buf[2000];

    /* DWG */
    char   path[2000];
    short initerror, entset, retval;
    AD_OBJHANDLE pspace, mspace;
    PAD_ENT_HDR adenhd;
    PAD_ENT aden;
    AD_VMADDR entlist;
    
    G_gisinit(argv[0]);
    
    module = G_define_module();
    module->keywords = _("vector, import");
    module->description = "Convert DWG/DXF to GRASS vector map";

    in_opt = G_define_option();
    in_opt->key = "input";
    in_opt->type =  TYPE_STRING;
    in_opt->required = YES;
    in_opt->description = "DWG or DXF file";
    in_opt->gisprompt = "old_file,file,input";

    out_opt = G_define_standard_option(G_OPT_V_OUTPUT);
    out_opt->required = YES;

    layers_opt= G_define_option();
    layers_opt->key = "layers";
    layers_opt->type =  TYPE_STRING;
    layers_opt->required = NO;
    layers_opt->multiple = YES;
    layers_opt->description = "List of layers to import";

    invert_flag = G_define_flag();
    invert_flag->key         = 'i';
    invert_flag->description = "Invert selection by layers (don't import layers in list)";
    
    z_flag = G_define_flag();
    z_flag->key         = 'z';
    z_flag->description = "Create 3D vector";

    circle_flag = G_define_flag();
    circle_flag->key               = 'c';
    circle_flag->description       = "Write circles as points (centre)";

    l_flag = G_define_flag();
    l_flag->key               = 'l';
    l_flag->description       = "List available layers and exit";

    int_flag = G_define_flag();
    int_flag->key               = 'n';
    int_flag->description       = "Use numeric type for attribute \"layer\"";

    if (G_parser (argc, argv))
	exit(EXIT_FAILURE); 

    db_init_string (&sql);
    db_init_string (&str);
    adenhd=(PAD_ENT_HDR)G_malloc (sizeof(AD_ENT_HDR));
    aden=(PAD_ENT)G_malloc (sizeof(AD_ENT));
    Layer=(PAD_LAY)G_malloc (sizeof(AD_LAY));
    Points = Vect_new_line_struct ();
    Cats = Vect_new_cats_struct ();
    Block = NULL;

    atrans = 20; /* nested, recursive levels */
    Trans = (TRANS *) G_malloc ( atrans * sizeof(TRANS) );
    
    /* Init OpenDWG */
    sprintf ( path, "%s/etc/adinit.dat", G_gisbase() ); 
    if (!adInitAd2( path, &initerror)) {
        sprintf(buf, "Unable to initialize OpenDWG Toolkit, error: %d: %s.", initerror,adErrorStr(initerror));
        if (initerror==AD_UNABLE_TO_OPEN_INIT_FILE) sprintf ( buf, "%s Cannot open %s", buf, path);
        G_fatal_error ( buf );
    }
    adSetupDwgRead();
    adSetupDxfRead();
    
    /* Open input file */
    if ((dwghandle=adLoadFile(in_opt->answer,AD_PRELOAD_ALL,1))==NULL) {
	G_fatal_error ("Cannot open input file. Error %d: %s", adError(),adErrorStr(adError()) );
    }

    if ( l_flag->answer ) { /* List layers */
	PAD_TB adtb;
	AD_DWGHDR adhd;
	int i;
	char on, frozen, vpfrozen, locked;

	adtb=(PAD_TB)G_malloc (sizeof(AD_TB));

	G_debug (2, "%d layers", (int) adNumLayers(dwghandle) );
	adReadHeaderBlock(dwghandle,&adhd);
	adStartLayerGet(dwghandle);

        fprintf(stdout, "%d layers:\n", (int) adNumLayers(dwghandle));
	for ( i = 0; i < (int) adNumLayers(dwghandle); i++) {
	    adGetLayer(dwghandle, &(adtb->lay));
	    if (!adtb->lay.purgedflag) {
	        fprintf( stdout, "%s COLOR %d, ", adtb->lay.name, adtb->lay.color);
	    }
	    adGetLayerState ( dwghandle,adtb->lay.objhandle, &on, &frozen, &vpfrozen, &locked);
	    if (on) fprintf(stdout, "ON, "); else fprintf(stdout, "OFF, ");
	    if (frozen) fprintf(stdout, "FROZEN, "); else fprintf(stdout, "THAWED, ");
	    if (vpfrozen) fprintf(stdout, "VPFROZEN, "); else fprintf(stdout, "VPTHAWED, ");
	    if (locked) fprintf(stdout, "LOCKED\n"); else fprintf(stdout, "UNLOCKED\n");
	}
	adCloseFile(dwghandle);
	adCloseAd2();
 	exit(EXIT_SUCCESS);
    }


    /* open output vector */
    Vect_open_new (&Map, out_opt->answer, z_flag->answer );

    Vect_hist_command ( &Map );

    /* Add DB link */
    Fi = Vect_default_field_info ( &Map, 1, NULL, GV_1TABLE );
    Vect_map_add_dblink ( &Map, 1, NULL, Fi->table, "cat", Fi->database, Fi->driver);

    driver = db_start_driver_open_database ( Fi->driver, Vect_subst_var(Fi->database,&Map) );
    if ( driver == NULL ) {
        G_fatal_error ( "Cannot open database %s by driver %s",
	                   Vect_subst_var(Fi->database,&Map), Fi->driver );
    }
    db_begin_transaction ( driver );
    
    /* Create table */
    if ( int_flag->answer ) { /* List layers */
      sprintf ( buf, "create table %s ( cat integer, entity_name varchar(20), color int, weight int, "
		"layer real, block varchar(100), txt varchar(100) )", Fi->table );

    }else{
      sprintf ( buf, "create table %s ( cat integer, entity_name varchar(20), color int, weight int, "
	           "layer varchar(100), block varchar(100), txt varchar(100) )", Fi->table );
    }
    db_set_string ( &sql, buf);
    G_debug ( 3, db_get_string ( &sql ) );

    if (db_execute_immediate (driver, &sql) != DB_OK ) {
	db_close_database(driver);
	db_shutdown_driver(driver);
	G_fatal_error ( "Cannot create table: %s", db_get_string ( &sql )  );
    }

    if ( db_create_index2(driver, Fi->table, "cat" ) != DB_OK )
	G_warning ( "Cannot create index" );

    if (db_grant_on_table (driver, Fi->table, DB_PRIV_SELECT, DB_GROUP|DB_PUBLIC ) != DB_OK )
	G_fatal_error ( "Cannot grant privileges on table %s", Fi->table );

    cat = 1;
    n_elements = n_skipped = 0;
    /* Write each entity. Some entities may be composed by other entities (like INSERT or BLOCK) */
    /* Set transformation for first (index 0) level */
    Trans[0].dx = Trans[0].dy = Trans[0].dz = 0;
    Trans[0].xscale = Trans[0].yscale = Trans[0].zscale = 1;
    Trans[0].rotang = 0;
    if (adGetBlockHandle(dwghandle, pspace, AD_PAPERSPACE_HANDLE)) {
	entlist=adEntityList(dwghandle,pspace);
	adStartEntityGet(entlist);
	for (entset=0; entset<2; entset++) {
	    do {
	        if (!(retval=adGetEntity(entlist,adenhd,aden))) continue;
	        wrentity(adenhd,aden, 0, entlist, circle_flag->answer);
	    } while (retval==1);
	    if (entset==0) {
	        if (adGetBlockHandle(dwghandle, mspace, AD_MODELSPACE_HANDLE)) {
	            entlist=adEntityList(dwghandle,mspace);
	            adStartEntityGet(entlist);
	        }
	    }
	}
    }

    G_message (_("%d elements processed"), n_elements);
    if ( n_skipped > 0 )
	G_message ( _("%d elements skipped (layer name was not in list)"), n_skipped );

    db_commit_transaction ( driver );
    db_close_database_shutdown_driver ( driver );
	
    adCloseFile(dwghandle); 
    adCloseAd2();
    
    Vect_build ( &Map, stdout );
    Vect_close ( &Map );

    exit(EXIT_SUCCESS);
}

