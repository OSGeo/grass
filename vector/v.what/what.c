#include <string.h>
#include <unistd.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/raster.h>
#include <grass/display.h>
#include <grass/colors.h>
#include <grass/Vect.h>
#include <grass/form.h>
#include <grass/dbmi.h>
#include <grass/glocale.h>
#include "what.h"
static int nlines = 50;

#define WDTH 5

int what(double east, double north, double maxdist, int width,
	 int mwidth, int topo, int showextra)
{
    int type;
    char east_buf[40], north_buf[40];
    double sq_meters;
    double z = 0, l = 0;
    int notty = 0;
    int getz = 0;
    struct field_info *Fi;
    plus_t line, area = 0, centroid;
    int i;
    struct line_pnts *Points;
    struct line_cats *Cats;
    char buf[1000], *str;
    dbString html; 
    char *form;
    
    Points = Vect_new_line_struct();
    Cats = Vect_new_cats_struct();
    db_init_string (&html);
    
    for (i = 0; i < nvects; i++) 
    {
	
	Vect_reset_cats ( Cats );
	/* Try to find point first and only if no one was found try lines,
	 *  otherwise point on line could not be selected and similarly for areas */
	line = Vect_find_line(&Map[i], east, north, 0.0, GV_POINT | GV_CENTROID, maxdist, 0, 0);
	if (line == 0) 
        {
            line = Vect_find_line(&Map[i], east, north, 0.0,
				  GV_LINE | GV_BOUNDARY | GV_FACE, maxdist, 0, 0);
        }
	
	if ( line == 0 ) 
	{
	    area = Vect_find_area(&Map[i], east, north);
	    getz = Vect_tin_get_z(&Map[i], east, north, &z, NULL, NULL);
	} 
	
	G_debug (2, "line = %d \narea = %d", line, area);
	
	if ( !i  ) 
        {
            G_format_easting(east, east_buf, G_projection());
            G_format_northing(north, north_buf, G_projection());
            if (line + area > 0 || G_verbose() >= G_verbose_std()) {
		fprintf(stdout, "\nEast: %s\nNorth: %s\n", east_buf, north_buf);
		if (notty)
		    fprintf(stderr, "\nEast: %s\nNorth: %s\n", east_buf, north_buf);
	    }
            nlines++;
	}
	
	strcpy(buf, vect[i]);
	if ((str = strchr(buf, '@'))) *str = 0;
	
	if (line + area > 0 || G_verbose() >= G_verbose_std()) {
	    /* fprintf(stdout, "Map: %*s \nMapset: %-*s\n", width, Map[i].name, mwidth, Map[i].mapset); */
	    fprintf(stdout, "\nMap: %s \nMapset: %s\n", Map[i].name, Map[i].mapset);
	    if (notty)
		/* fprintf(stderr, "Map: %*s \nMapset: %-*s\n", width, Map[i].name, mwidth, Map[i].mapset); */
		fprintf(stderr, "\nMap: %s \nMapset: %s\n", Map[i].name, Map[i].mapset);
	}
	nlines++;
	
	if (line + area == 0) 
	{
	    if (line + area > 0 || G_verbose() >= G_verbose_std()) {
		fprintf(stdout, _("Nothing Found.\n"));
		if (notty)
		    fprintf(stderr, _("Nothing Found.\n"));
	    }
	    nlines++;
	    continue;
	} 
	
	if ( line >  0) 
	{
            sprintf(buf, "Object type: ");
            type = Vect_read_line(&Map[i], Points, Cats, line);
            switch ( type ) 
            {
	    case GV_POINT:
                sprintf ( buf, "Point\n" );
                break;
	    case GV_LINE:
                sprintf ( buf, "Line\n" );
                break;
	    case GV_BOUNDARY:
                sprintf ( buf, "Boundary\n" );
                break;
	    case GV_FACE:
                sprintf ( buf, "Face\n" );
                break;
	    case GV_CENTROID:
                sprintf ( buf, "Centroid\n" );
                break;
	    default:
                sprintf ( buf, "Unknown\n" );
            }
            if ( type & GV_LINES ) 
            {
                if ( G_projection() == 3) l = Vect_line_geodesic_length ( Points );
                else l = Vect_line_length ( Points );
            }
	    
	    
            if ( topo ) 
            {
                int n, node[2], nnodes, nnlines, nli, nodeline, left, right;
                float angle;
                
                Vect_get_line_areas ( &(Map[i]), line, &left, &right );
                fprintf(stdout, "Looking for features within: %f \n", maxdist); 
                fprintf(stdout, _("Line: %d  \nType: %s  \nLeft: %d  \nRight: %d  \n"), line, buf, left, right);
                if ( type & GV_LINES ) 
                {
		    nnodes = 2;
		    fprintf(stdout, _("Length: %f\n"), l);
                } 
                else 
                { /* points */
		    nnodes = 1;
		    fprintf(stdout, "\n");
                }
                
                Vect_get_line_nodes ( &(Map[i]), line, &node[0], &node[1]);
		
                for ( n = 0; n < nnodes; n++ ) 
                { 
		    double nx, ny, nz;
		    nnlines = Vect_get_node_n_lines (  &(Map[i]), node[n]);
		    
		    Vect_get_node_coor (  &(Map[i]), node[n], &nx, &ny, &nz ); 
                    fprintf(stdout,
			    _("Node[%d]: %d  \nNumber of lines: %d  \nCoordinates: %.6f, %.6f, %.6f\n"),
			    n, node[n], nnlines, nx, ny, nz );
		    
		    for ( nli = 0; nli < nnlines; nli++ ) 
		    {
			nodeline =  Vect_get_node_line ( &(Map[i]), node[n], nli );
			angle =  Vect_get_node_line_angle ( &(Map[i]), node[n], nli );
                        fprintf(stdout, _("Line: %5d  \nAngle: %.8f\n"), nodeline, angle);
		    }
                }
                
            } 
            else 
            {
                fprintf(stdout, _("Type: %s"), buf);
		fprintf(stdout, _("Line: %d\n"), line);
                if ( type & GV_LINES )
		    fprintf(stdout, _("Length: %f\n"), l);
            } 
	    
            /* Height */
            if ( Vect_is_3d(&(Map[i])) ) 
            {
                int j;
                double min, max;
		
                if ( type & GV_POINTS ) 
                {
                    fprintf(stdout, _("Point height: %f\n"), Points->z[0]);
                } 
                else if ( type & GV_LINES ) 
                {
                    min = max = Points->z[0];
                    for ( j = 1; j < Points->n_points; j++) 
                    {
                        if ( Points->z[j] < min ) min = Points->z[j];
                        if ( Points->z[j] > max ) max = Points->z[j];
                    }
                    if ( min == max ) 
                    {
                        fprintf(stdout, _("Line height: %f\n"), min);
                    } 
                    else 
                    { 
                        fprintf(stdout, _("Line height min: %f \nLine height max: %f\n"),
				min, max);
                    }
                }
            } /* if height */
	} /* if line > 0 */
	
	if (area > 0) 
	{
            if (Map[i].head.with_z && getz) 
            {
                fprintf(stdout, _("Object type: Area \nArea height: %f\n"), z);
            } 
            else 
            {
                fprintf(stdout, _("Object type: Area\n"));
            }
            sq_meters = Vect_get_area_area(&Map[i], area);
            if ( topo ) 
            {
                int nisles, isleidx, isle, isle_area;
                
                nisles = Vect_get_area_num_isles ( &Map[i], area );
                fprintf(stdout, _("Area: %d  \nNumber of isles: %d\n"), area, nisles );
		
                for ( isleidx = 0; isleidx < nisles; isleidx++ ) 
                {
                    isle = Vect_get_area_isle ( &Map[i], area, isleidx );
		    fprintf(stdout, _("Isle[%d]: %d\n"), isleidx, isle );
                }
		
                isle = Vect_find_island ( &Map[i], east, north );
                
                if ( isle ) 
                {
		    isle_area = Vect_get_isle_area ( &Map[i], isle );
                    fprintf(stdout, _("Island: %d In area: %d\n"), isle, isle_area );
                }
	    } 
            else 
            {
                fprintf(stdout, _("Sq Meters: %.3f \nHectares: %.3f\n"),
			sq_meters, (sq_meters / 10000.));
		fprintf(stdout, _("Acres: %.3f \nSq Miles: %.4f\n"),
			((sq_meters * 10.763649) / 43560.),
			((sq_meters * 10.763649) / 43560.) / 640.);
                if (notty) 
                {
		    fprintf(stderr,
			    _("Sq Meters: %.3f \nHectares: %.3f\n"),
			    sq_meters, (sq_meters / 10000.));
		    fprintf(stderr,
			    _("Acres: %.3f \nSq Miles: %.4f\n"),
			    ((sq_meters * 10.763649) / 43560.),
			    ((sq_meters * 10.763649) / 43560.) / 640.);
                }
                nlines += 3;
            } 
            centroid = Vect_get_area_centroid(&Map[i], area);
            if (centroid > 0) 
            {
                Vect_read_line(&Map[i], Points, Cats, centroid);
            }
	} /* if area > 0 */
	
        if ( Cats->n_cats > 0 ) 
        {
            int j;
            for (j = 0; j < Cats->n_cats; j++)
            {
                G_debug(2, "field = %d \ncategory = %d\n", Cats->field[j], Cats->cat[j]);
                fprintf( stdout, _("Layer: %d\nCategory: %d \n"), Cats->field[j], Cats->cat[j] );
                Fi = Vect_get_field(&(Map[i]), Cats->field[j]);
                if (Fi != NULL && showextra) 
                {
                    int format = F_TXT, edit_mode = F_VIEW;
                    fprintf( stdout, _("\nDriver: %s\nDatabase: %s\nTable: %s\nKey column: %s\n"),
			     Fi->driver, Fi->database, Fi->table, Fi->key);
                    F_generate ( Fi->driver, Fi->database, Fi->table, Fi->key, Cats->cat[j], 
				 NULL, NULL, edit_mode, format, &form);
                    fprintf( stdout, "%s", form );
                    G_free(form);
                    G_free(Fi);
                }
            }
        }
    } /* for nvects */
    
    fflush(stdout);
    
    return 0;
}
