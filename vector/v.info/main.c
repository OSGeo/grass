/***************************************************************
 *
 * MODULE:       v.info
 * 
 * AUTHOR(S):    CERL, updated to 5.7 by Markus Neteler
 *               
 * PURPOSE:      print vector map info
 *               
 * COPYRIGHT:    (C) 2002 by the GRASS Development Team
 *
 *               This program is free software under the 
 *               GNU General Public License (>=v2). 
 *               Read the file COPYING that comes with GRASS
 *               for details.
 *
 **************************************************************/
#include <string.h>
#include <stdlib.h>
#include <grass/gis.h>
#include <grass/Vect.h>
#include <grass/dbmi.h>
#include <grass/glocale.h>

#define printline(x) fprintf (stdout, " | %-74.74s |\n", x)
#define divider(x) \
	fprintf (stdout, " %c", x); \
	for (i = 0; i < 76; i++ ) \
		fprintf ( stdout, "-" ); \
	fprintf (stdout, "%c\n", x)

/* the vector header is here:
   include/vect/dig_structs.h
   
   the vector API is here:
   lib/vector/Vlib/level_two.c
*/

void format_double(double,char *);


int main (int argc, char *argv[])
{
  struct GModule *module;
  struct Option *in_opt, *fieldopt;
  struct Flag *histf, *columns, *gflag, *tflag, *mflag;
  struct Map_info Map;
  struct dig_head v_head;
  BOUND_BOX box;
  char *mapset, line[200], buf[1001];
  int i;
  int with_z;
  struct field_info *fi;
  dbDriver *driver=NULL;
  dbHandle handle;
  dbString table_name;
  dbTable *table;
  int field, num_dblinks, ncols, col;
  char tmp1[100], tmp2[100];

  G_gisinit (argv[0]);

  module = G_define_module();
  module->keywords = _("vector");
    module->description = _("Outputs basic information about a user-specified vector map layer.");

  /* get G_OPT_ from include/gis.h */
  in_opt = G_define_standard_option(G_OPT_V_MAP);
 
  fieldopt = G_define_standard_option(G_OPT_V_FIELD);

  histf = G_define_flag ();
  histf->key             = 'h';
  histf->description     = _("Print vector history instead of info");

  columns = G_define_flag();
  columns->key           = 'c';
  columns->description   = _("Print types/names of table columns for specified layer instead of info");

  gflag = G_define_flag();
  gflag->key            = 'g';
  gflag->description    = _("Print map region only");
  
  mflag = G_define_flag();
  mflag->key            = 'm';
  mflag->description    = _("Print map title only");

  tflag = G_define_flag();
  tflag->key            = 't';
  tflag->description    = _("Print topology information only");

  if (G_parser(argc,argv))
    exit(EXIT_FAILURE);

  /* open input vector */
  if ((mapset = G_find_vector2 (in_opt->answer, "")) == NULL) {
     G_fatal_error (_("Vector map <%s> not found"), in_opt->answer);
  }
    
  Vect_set_open_level (2);
  Vect_open_old_head (&Map, in_opt->answer, mapset);
  with_z = Vect_is_3d (&Map);
  v_head = Map.head;

  if ( histf->answer ) {
      Vect_hist_rewind ( &Map );
      while ( Vect_hist_read ( buf, 1000, &Map ) != NULL ) {
	 fprintf ( stdout, "%s\n", buf );
      }
  }
  else if ( mflag->answer ) {
	fprintf ( stdout, "%s\n", Vect_get_map_name(&Map));
  }
  else if (gflag->answer || tflag->answer ){
       if (gflag->answer) {
            Vect_get_map_box (&Map, &box );
            G_format_northing (box.N, tmp1, Vect_get_proj(&Map));
            G_format_northing (box.S, tmp2, Vect_get_proj(&Map));
            fprintf(stdout, "north=%s\n", tmp1);
            fprintf(stdout, "south=%s\n", tmp2);
                    
            G_format_easting (box.E, tmp1, Vect_get_proj(&Map));
            G_format_easting (box.W, tmp2, Vect_get_proj(&Map));
            fprintf(stdout, "east=%s\n", tmp1);
            fprintf(stdout, "west=%s\n", tmp2);
            fprintf(stdout, "top=%f\n", box.T);
            fprintf(stdout, "bottom=%f\n", box.B);
       }
       if (tflag->answer) {
            long int nprimitives = 0;
            nprimitives += (long)Vect_get_num_primitives(&Map, GV_POINT);
            nprimitives += (long)Vect_get_num_primitives(&Map, GV_LINE);
            nprimitives += (long)Vect_get_num_primitives(&Map, GV_BOUNDARY);
            nprimitives += (long)Vect_get_num_primitives(&Map, GV_FACE);
            nprimitives += (long)Vect_get_num_primitives(&Map, GV_CENTROID);
            nprimitives += (long)Vect_get_num_primitives(&Map, GV_KERNEL);


            fprintf (stdout, "nodes=%ld\n", (long)Vect_get_num_nodes(&Map));
            fflush(stdout);
            fprintf (stdout, "points=%ld\n", (long)Vect_get_num_primitives(&Map, GV_POINT));
            fflush(stdout);

            fprintf (stdout, "lines=%ld\n", (long)Vect_get_num_primitives(&Map, GV_LINE));
            fflush(stdout);

            fprintf (stdout, "boundaries=%ld\n", (long)Vect_get_num_primitives(&Map, GV_BOUNDARY));
            fflush(stdout);

            fprintf (stdout, "centroids=%ld\n",  (long)Vect_get_num_primitives(&Map, GV_CENTROID));
            fflush(stdout);

            fprintf (stdout, "areas=%ld\n", (long)Vect_get_num_areas(&Map));
            fflush(stdout);

            fprintf (stdout, "islands=%ld\n", (long)Vect_get_num_islands(&Map));
            fflush(stdout);

            fprintf (stdout, "faces=%ld\n", (long)Vect_get_num_primitives(&Map, GV_FACE));
            fflush(stdout);

            fprintf (stdout, "kernels=%ld\n", (long)Vect_get_num_primitives(&Map, GV_KERNEL));
            fflush(stdout);

            fprintf (stdout, "primitives=%ld\n", nprimitives);
            fflush(stdout);

            fprintf (stdout, "map3d=%d\n", Vect_is_3d(&Map));
            fflush(stdout);
       }
     } 
  else {
     if ( columns->answer ) {
      num_dblinks = Vect_get_num_dblinks(&Map);
      if (num_dblinks <= 0) {
         G_fatal_error(_("Database connection for map <%s> is not defined in DB file"), in_opt->answer);
      }
      else /* num_dblinks > 0 */
      {
       field = atoi ( fieldopt->answer );
       G_message(_("Displaying column types/names for database connection of layer %d:"), field);
       if ( (fi = Vect_get_field ( &Map, field)) == NULL)
          G_fatal_error("Database connection not defined");
       driver = db_start_driver(fi->driver);
       if (driver == NULL)
           G_fatal_error("Cannot open driver %s", fi->driver) ;
       db_init_handle (&handle);
       db_set_handle (&handle, fi->database, NULL);
       if (db_open_database(driver, &handle) != DB_OK)
           G_fatal_error("Cannot open database <%s>", fi->database);
       db_init_string(&table_name);
       db_set_string(&table_name, fi->table);
       if(db_describe_table (driver, &table_name, &table) != DB_OK)
           G_fatal_error("Cannot open table <%s>", fi->table);

       ncols = db_get_table_number_of_columns(table);
       for (col = 0; col < ncols; col++)
           fprintf (stdout,"%s|%s\n", db_sqltype_name(db_get_column_sqltype(db_get_table_column(table, col))), db_get_column_name(db_get_table_column(table, col)));
 
       db_close_database(driver);
       db_shutdown_driver(driver);
      }
     } else {  
      divider ('+');
      sprintf (line, _("Layer:           %s"), in_opt->answer);
      printline (line);
      sprintf (line, _("Mapset:          %s"), mapset);
      printline (line);
      sprintf (line, _("Location:        %s"), G_location());
      printline (line);
      sprintf (line, _("Database:        %s"), G_gisdbase());
      printline (line);
      sprintf (line, _("Title:           %s"), Vect_get_map_name(&Map));
      printline (line);
      sprintf (line, _("Map scale:       1:%d"), Vect_get_scale(&Map));
      printline (line);
      sprintf (line, _("Map format:      %s"),  Vect_maptype_info(&Map));
      printline (line);
      sprintf (line, _("Name of creator: %s"), Vect_get_person(&Map));
      printline (line);
      sprintf (line, _("Organization:    %s"), Vect_get_organization(&Map));
      printline (line);
      sprintf (line, _("Source date:     %s"), Vect_get_map_date(&Map));
      printline (line);

      divider ('|');

      sprintf (line, _("  Type of Map:  %s (level: %i)        "), _("vector"), Vect_level (&Map));

      printline (line);

      if ( Vect_level (&Map) > 1)
      {
	printline ("");
	sprintf (line, _("  Number of points:       %-9ld       Number of areas:      %-9ld"), 
			  (long)Vect_get_num_primitives(&Map, GV_POINT), (long)Vect_get_num_areas(&Map));
	printline (line);
	sprintf (line, _("  Number of lines:        %-9ld       Number of islands:    %-9ld"),
			  (long)Vect_get_num_primitives(&Map, GV_LINE), (long)Vect_get_num_islands(&Map));
	printline (line);
	sprintf (line, _("  Number of boundaries:   %-9ld       Number of faces:      %-9ld"),
			  (long)Vect_get_num_primitives(&Map, GV_BOUNDARY), (long)Vect_get_num_primitives(&Map, GV_FACE));
	printline (line);
	sprintf (line, _("  Number of centroids:    %-9ld       Number of kernels:    %-9ld"),
			  (long)Vect_get_num_primitives(&Map, GV_CENTROID), (long)Vect_get_num_primitives(&Map, GV_KERNEL));
	printline (line);
	printline ("");
	sprintf (line, _("  Map is 3D:              %d"), Vect_is_3d(&Map));
	printline (line);
	sprintf (line, _("  Number of dblinks:      %-9ld"), (long)Vect_get_num_dblinks(&Map));
	printline (line);
      }
      else
      { /* should not be reached */
	sprintf (line, _("                No topology present"));
	printline (line);
      }

      printline ("");
      /* this differs from r.info in that proj info IS taken from the map here, not the location settings */
      /* Vect_get_proj_name() and _zone() are typically unset?! */
      if(G_projection() == PROJECTION_UTM)
	sprintf (line, _("        Projection: %s (zone %d)"),
		 Vect_get_proj_name(&Map), Vect_get_zone(&Map));
      else
	sprintf (line, _("        Projection: %s"), Vect_get_proj_name(&Map));
      printline (line);

      Vect_get_map_box (&Map, &box );

      G_format_northing (box.N, tmp1, G_projection());
      G_format_northing (box.S, tmp2, G_projection());
      sprintf (line, "              N: %17s    S: %17s", tmp1, tmp2);
      printline (line);

      G_format_easting (box.E, tmp1, G_projection());
      G_format_easting (box.W, tmp2, G_projection());
      sprintf (line, "              E: %17s    W: %17s", tmp1, tmp2);
      printline (line);

      if( Vect_is_3d(&Map) ) {
	  format_double(box.B, tmp1);
	  format_double(box.T, tmp2);
	  sprintf (line, "              B: %17s    T: %17s", tmp1, tmp2);
	  printline (line);
      }
      printline ("");

      format_double(Vect_get_thresh(&Map), tmp1);
      sprintf (line, _("  Digitization threshold: %s"), tmp1);
      printline (line);
      sprintf (line, _("  Comments:"));
      printline (line);
      sprintf (line, "    %s", v_head.line_3);
      printline (line);
      divider ('+');
      fprintf (stdout, "\n");
    }
  }

  Vect_close (&Map);

  return (EXIT_SUCCESS);
}


/* cloned from lib/gis/wind_format.c */
void format_double ( double value, char *buf)
{
    sprintf (buf, "%.8f", value);
    G_trim_decimal (buf);
}
