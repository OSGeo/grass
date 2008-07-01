/******************************************************************************
 * MODULE:       v.in.sites -- Convert site_list to a vector point layer.
 *               (was s.to.vect)
 * AUTHOR(S):    Original author (1991) R.L. Glenn 
 *                  - USDA, Soil Conservation Service
 *               Changes (1995) -- PWC (??) from NRCS
 *               Modified (2000-1) Eric G. Miller <egm2@jps.net>
 *               Update to GRASS 5.7               
 *               
 * PURPOSE:      A general module to convert site_lists to vector point layers.
 * 	    
 * COPYRIGHT:    (C) 2000 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <grass/gis.h>
#include <grass/dbmi.h>
#include <grass/site.h>
#include <grass/Vect.h>
#include <grass/glocale.h>

int main (int argc, char *argv[])
{
    int    i, cat, *clen, len;
    char   *sname, *mapset;
    char   buf[1024];
    int    count, withz;
    int    dims, dbls, strs ;
    double z;
    FILE   *site;
    struct Option *sitein, *outvect;
    struct GModule *module;
    struct Map_info Map;
    struct line_pnts *Points ;
    struct line_cats *Cats;
    dbString sql, strval;
    dbDriver *driver;
    dbHandle handle;
    struct field_info *fi;

    Site *s;
    RASTER_MAP_TYPE map_type; 

    G_gisinit(argv[0]);

    module = G_define_module();
    module->keywords = _("vector, import");
    module->description = "Converts a GRASS site_lists file into a vector map.";

    sitein = G_define_option();
    sitein->key          = "input";
    sitein->description  = "Name of site input file";
    sitein->type         = TYPE_STRING;
    sitein->required     = YES;
    sitein->multiple     = NO;
    sitein->gisprompt    = "old,site_lists,site list";

    outvect = G_define_option();
    outvect->key          = "output";
    outvect->description  = "Name of vector output file";
    outvect->type         = TYPE_STRING;
    outvect->required     = YES;
    outvect->multiple     = NO;
    outvect->gisprompt    = "new,vector,vector";

    if (G_parser(argc, argv)) exit(EXIT_FAILURE);

    Points = Vect_new_line_struct();
    Cats = Vect_new_cats_struct ();
    db_init_string (&sql);
    db_init_string (&strval);

    sname = sitein->answer;

    mapset = G_find_file2 ("site_lists", sname, "");
    if (mapset == NULL) G_fatal_error("Site file file [%s] not available\n",sname);

    if ((site = G_oldsites_open_old ( sname, mapset)) == NULL)
        G_fatal_error ("Not able to open site file <%s@%s>\n", sname, mapset);

    if (G_oldsite_describe(site, &dims, &map_type, &strs, &dbls) != 0) 
        G_fatal_error("Unable to guess site_list format!\n");

    if ((s = G_site_new_struct(map_type, dims, strs, dbls)) == NULL)
        G_fatal_error ("Failed to allocate site structure");

    fprintf (stdout,"Input format: dimension: %d   strings: %d   FP:%d\n", dims, strs, dbls); 
    
    if (map_type == FCELL_TYPE || map_type == DCELL_TYPE) {
        fprintf (stdout,"Floating point category values, using sequential integer for category\n");
    } else if (map_type != CELL_TYPE) {
        fprintf (stdout,  "No category values, using sequential integer for category\n");
    }

    clen = (int *) G_calloc ( strs, sizeof ( int ) );
    while ( G_oldsite_get(site,s) >= 0) {
	for ( i = 0; i < strs; i++ ) {
	    len = strlen ( s->str_att[i] );
	    if ( len > clen[i] ) clen[i] = len;
	}
    }
    for ( i = 0; i < strs; i++ ) clen[i] += 10;

    if ( dims == 3 ) withz = 1; else withz = 0;
	
    Vect_open_new (&Map, outvect->answer, withz);
    Vect_hist_command ( &Map );

    fi = Vect_default_field_info ( &Map, 1, NULL, GV_1TABLE );
    Vect_map_add_dblink ( &Map, 1, NULL, fi->table, "cat", fi->database, fi->driver);

    /* Create new table */
    sprintf ( buf, "create table %s ( ", fi->table );
    db_append_string ( &sql, buf);
    db_append_string ( &sql, "cat integer" );
    
    if ( dims > 3 ) {
        for ( i = 2; i < dims; i++ ) {
	    sprintf (buf, ", dim%d double precision", i - 1 );
	    db_append_string ( &sql, buf);
	}
    }

    if ( map_type == FCELL_TYPE || map_type == DCELL_TYPE ) {
        db_append_string ( &sql, ", fcat double precision" );
    }
    
    for ( i = 0; i < strs; i++ ) {
	sprintf (buf, ", str%d varchar ( %d )", i + 1, clen[i] );
	db_append_string ( &sql, buf);
    }
    
    for ( i = 0; i < dbls; i++ ) {
	sprintf (buf, ", flt%d double precision", i + 1 );
	db_append_string ( &sql, buf);
    }
    db_append_string ( &sql, ")" );

    G_debug ( 1, db_get_string ( &sql ) );
    
    driver = db_start_driver( fi->driver );
    if (driver == NULL) G_fatal_error ( "Cannot open driver %s", fi->driver );
    db_init_handle (&handle);
    db_set_handle (&handle, Vect_subst_var(fi->database,&Map), NULL);
    if (db_open_database(driver, &handle) != DB_OK) {
	db_shutdown_driver(driver);
	G_fatal_error ( "Cannot open database %s", fi->database );
    }

    if (db_execute_immediate (driver, &sql) != DB_OK ) {
	db_close_database(driver);
	db_shutdown_driver(driver);
	G_fatal_error ( "Cannot create table: %s", db_get_string ( &sql )  );
    }

    if ( db_create_index2(driver, fi->table, "cat" ) != DB_OK )
	G_warning ( "Cannot create index" );

    if (db_grant_on_table (driver, fi->table, DB_PRIV_SELECT, DB_GROUP|DB_PUBLIC ) != DB_OK )
	G_fatal_error ( "Cannot grant privileges on table %s", fi->table );

    /* Convert */
    fprintf (stdout, "Transfering sites to vect file\n");

    count = 0;
    rewind ( site );
    while ( G_oldsite_get(site,s) >= 0) {
	/* Geometry */
	Vect_reset_line ( Points );
	Vect_reset_cats ( Cats );

	if ( withz ) z = s->dim[0]; else z = 0; 
        Vect_append_point ( Points, s->east, s->north, z );
	
	if ( map_type == CELL_TYPE ) cat = s->ccat; else cat = count + 1;
	
        Vect_cat_set ( Cats, 1, cat );

        Vect_write_line (&Map, GV_POINT, Points, Cats);

	/* Attributes */
        db_zero_string ( &sql );
	sprintf (buf, "insert into %s values ( %d ", fi->table, cat );
	db_append_string ( &sql, buf);

	if ( map_type == FCELL_TYPE ) {
	    sprintf (buf, ", %f", s->fcat );
	    db_append_string ( &sql, buf );
	}
	if ( map_type == DCELL_TYPE ) {
	    sprintf (buf, ", %f", s->dcat );
	    db_append_string ( &sql, buf );
	}
	
	for ( i = 0; i < strs; i++ ) {
	    db_set_string ( &strval, s->str_att[i] );
	    db_double_quote_string ( &strval );
	    sprintf (buf, ", '%s'", db_get_string ( &strval) );
	    db_append_string ( &sql, buf);
	}
	
	for ( i = 0; i < dbls; i++ ) {
	    sprintf (buf, ", %f", s->dbl_att[i] );
	    db_append_string ( &sql, buf);
	}
	
        db_append_string ( &sql, ")" );
	
        G_debug ( 3, db_get_string ( &sql ) );

	if (db_execute_immediate (driver, &sql) != DB_OK ) {
	    db_close_database(driver);
	    db_shutdown_driver(driver);
	    G_fatal_error ( "Cannot inser new row: %s", db_get_string ( &sql )  );
	}
        count++;
    }
    fprintf (stdout,"%d sites read\n", count); 
    fclose(site);
    db_close_database(driver);
    db_shutdown_driver(driver);
    Vect_build (&Map, stderr);
    Vect_close (&Map);
    G_site_free_struct(s);

    fprintf (stdout,"Vector file complete\n"); 
    
    return 0;
}
