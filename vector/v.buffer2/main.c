/****************************************************************
 *
 * MODULE:       v.buffer
 * 
 * AUTHOR(S):    Radim Blazek, Rosen Matev
 *               
 * PURPOSE:      Vector buffer
 *               
 * COPYRIGHT:    (C) 2001 by the GRASS Development Team
 *
 *               This program is free software under the 
 *               GNU General Public License (>=v2). 
 *               Read the file COPYING that comes with GRASS
 *               for details.
 *
 **************************************************************/
#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/Vect.h>
#include <grass/dbmi.h>
#include <grass/glocale.h>
#include "local_proto.h"

#define PI M_PI
#ifndef MIN
    #define MIN(X,Y) ((X<Y)?X:Y)
#endif
#ifndef MAX
    #define MAX(X,Y) ((X>Y)?X:Y)
#endif    


void stop(struct Map_info *In, struct Map_info *Out) {
    Vect_close (In);

    G_message ( _("Rebuilding topology...") );
    Vect_build_partial ( Out, GV_BUILD_NONE);
    Vect_build (Out);
    Vect_close (Out);
}

/* returns 1 if unit_tolerance is adjusted, 0 otherwise */
int adjust_tolerance(double *tolerance) {
    double t = 0.999 * ( 1 - cos ( 2 * PI / 8 / 2 ) );
    G_debug(2, "Maximum tolerance = %f", t);
    if (*tolerance > t) {
        *tolerance = t;
        return 1;
    }
    return 0;
}

int db_CatValArray_get_value_di(dbCatValArray *cvarr, int cat, double *value) {
    int t;
    int ctype = cvarr->ctype;
    int ret;
    
    if (ctype == DB_C_TYPE_INT) {
        ret = db_CatValArray_get_value_int(cvarr, cat, &t);
        if (ret != DB_OK)
            return ret;
        *value = (double)t;
        return DB_OK;
    }

    if (ctype == DB_C_TYPE_DOUBLE) {
        ret = db_CatValArray_get_value_double(cvarr, cat, value);
        return ret;
    }
    
    return DB_FAILED;
}

struct buf_contours {
    int inner_count;
    struct line_pnts *oPoints;
    struct line_pnts **iPoints;
};

int point_in_buffer(struct buf_contours *arr_bc, int buffers_count, struct Map_info *Out, double x, double y) {
    int i, j, ret, flag;
    
    for (i = 0; i < buffers_count; i++) {
        ret = Vect_point_in_poly(x, y, arr_bc[i].oPoints);
        if (ret == 0)
            continue;
            
        flag = 1;
        for (j = 0; j < arr_bc[i].inner_count; j++) {
            ret = Vect_point_in_poly(x, y, arr_bc[i].iPoints[j]);
            if (ret != 0) { /* inside inner contour */
                flag = 0;
                break;
            }
        }
        
        if (flag) {
            /* (x,y) is inside outer contour and outside inner contours of arr_bc[i] */
            return 1;
        }
    }
    
    return 0;
}

int main (int argc, char *argv[])
{
    struct Map_info In, Out;
    struct line_pnts *Points;
    struct line_cats *Cats, *BCats;
    const char *mapset;
    struct GModule *module;
    struct Option *in_opt, *out_opt, *type_opt, *dista_opt, *distb_opt, *angle_opt;
    struct Flag *straight_flag, *nocaps_flag;
    struct Option *tol_opt, *bufcol_opt, *scale_opt, *field_opt;
    double da, db, dalpha, tolerance, unit_tolerance;
    int type;
    int i, j, ret, nareas, area, nlines, line;
    char *Areas, *Lines;
    int field;
    struct buf_contours *arr_bc;
    int buffers_count;

 /* Attributes if sizecol is used */
    int nrec, ctype;
    struct field_info *Fi;
    dbDriver *Driver;
    dbCatValArray cvarr;
    double size_val, scale;


    module = G_define_module();
    module->keywords = _("vector");
    module->description = _("Creates a buffer around features of given type (areas must contain centroid).");
    
    in_opt = G_define_standard_option(G_OPT_V_INPUT);
    out_opt = G_define_standard_option(G_OPT_V_OUTPUT);
    
    type_opt = G_define_standard_option(G_OPT_V_TYPE) ;
    type_opt->options = "point,line,boundary,centroid,area";
    type_opt->answer = "point,line,area";
    
    field_opt = G_define_standard_option(G_OPT_V_FIELD);
    
    dista_opt = G_define_option();
    dista_opt->key = "distance";
    dista_opt->type = TYPE_DOUBLE;
    dista_opt->required = NO;
    dista_opt->description = _("Buffer distance along major axis in map units");
    
    distb_opt = G_define_option();
    distb_opt->key = "minordistance";
    distb_opt->type = TYPE_DOUBLE;
    distb_opt->required = NO;
    distb_opt->description = _("Buffer distance along minor axis in map units");
    distb_opt->guisection  = _("Advanced");
    
    angle_opt = G_define_option();
    angle_opt->key = "angle";
    angle_opt->type =  TYPE_DOUBLE;
    angle_opt->required = NO;
    angle_opt->answer = "0";
    angle_opt->description = _("Angle of major axis in degrees");
    angle_opt->guisection  = _("Advanced");
    
    bufcol_opt = G_define_option();
    bufcol_opt->key = "bufcol";
    bufcol_opt->type = TYPE_STRING;
    bufcol_opt->required = NO;
    bufcol_opt->description = _("Attribute column to use for buffer distances");
    bufcol_opt->guisection  = _("Advanced");
    
    scale_opt = G_define_option();
    scale_opt->key = "scale";
    scale_opt->type = TYPE_DOUBLE;
    scale_opt->required = NO;
    scale_opt->answer = "1.0";
    scale_opt->description = _("Scaling factor for attribute column values");
    scale_opt->guisection  = _("Advanced");

    tol_opt = G_define_option();
    tol_opt->key = "tolerance";
    tol_opt->type = TYPE_DOUBLE;
    tol_opt->required = NO;
    tol_opt->answer = "0.01";
    tol_opt->guisection = _("Advanced");    
    tol_opt->description = _("Maximum distance between theoretical arc and polygon segments as multiple of buffer");

    straight_flag = G_define_flag();
    straight_flag->key = 's';
    straight_flag->description = _("Make outside corners straight");

    nocaps_flag = G_define_flag();
    nocaps_flag->key = 'c';
    nocaps_flag->description = _("Don't make caps at the ends of polylines");

    G_gisinit(argv[0]);
    if (G_parser (argc, argv))
        exit(EXIT_FAILURE);
    
    type = Vect_option_to_types ( type_opt );
    field = atoi( field_opt->answer );
    
    if ((dista_opt->answer && bufcol_opt->answer) || (!(dista_opt->answer || bufcol_opt->answer)))
        G_fatal_error("Select a buffer distance/minordistance/angle or column, but not both.");
    
    if (bufcol_opt->answer)
        G_warning(_("The bufcol option may contain bugs during the cleaning "
            "step. If you encounter problems, use the debug "
            "option or clean manually with v.clean tool=break; "
            "v.category step=0; v.extract -d type=area"));
    
    tolerance = atof(tol_opt->answer);
    if (adjust_tolerance(&tolerance))
        G_warning(_("The tolerance was reset to %g."), tolerance);
    
    scale = atof( scale_opt->answer );
    if (scale <= 0.0)
        G_fatal_error("Illegal scale value");


    if (dista_opt->answer) {
        da = atof(dista_opt->answer);
        
        if (distb_opt->answer)
            db = atof(distb_opt->answer);
        else
            db = da;
            
        if (angle_opt->answer)
            dalpha = atof(angle_opt->answer);
        else
            dalpha = 0;
        
        unit_tolerance = tolerance * MIN(da, db);
        G_message(_("The tolerance in map units = %g"), unit_tolerance);
    }

    Vect_check_input_output_name(in_opt->answer, out_opt->answer, GV_FATAL_EXIT);
    
    Points = Vect_new_line_struct();
    Cats = Vect_new_cats_struct();
    BCats = Vect_new_cats_struct();

    /* open input vector */
    if ((mapset = G_find_vector2(in_opt->answer, "")) == NULL)
        G_fatal_error(_("Vector map <%s> not found"), in_opt->answer);

    Vect_set_open_level(2);

    if (1 > Vect_open_old(&In, in_opt->answer, mapset))
        G_fatal_error(_("Unable to open vector map <%s>"), in_opt->answer);

    if (0 > Vect_open_new(&Out, out_opt->answer, WITHOUT_Z)) {
        Vect_close(&In);
        G_fatal_error(_("Unable to create vector map <%s>"), out_opt->answer);
    }
    
    /* check and load attribute column data */
    if (bufcol_opt->answer) {
        db_CatValArray_init ( &cvarr );
        
        Fi = Vect_get_field(&In, field);
        if ( Fi == NULL )
            G_fatal_error(_("Unable to get layer info for vector map"));
        
        Driver = db_start_driver_open_database(Fi->driver, Fi->database);
        if (Driver == NULL)
            G_fatal_error(_("Unable to open database <%s> by driver <%s>"), Fi->database, Fi->driver);
        
        /* Note do not check if the column exists in the table because it may be expression */

        /* TODO: only select values we need instead of all in column */
        nrec = db_select_CatValArray(Driver, Fi->table, Fi->key, bufcol_opt->answer, NULL, &cvarr);
        if (nrec < 0)
            G_fatal_error(_("Unable to select data from table"));
        G_debug(2, "%d records selected from table", nrec);

        ctype = cvarr.ctype;
        if (ctype != DB_C_TYPE_INT && ctype != DB_C_TYPE_DOUBLE)
            G_fatal_error(_("Column type not supported"));
        
        db_close_database_shutdown_driver(Driver);
        
        /* Output cats/values list */
        for (i = 0; i < cvarr.n_values; i++) {
            if (ctype == DB_C_TYPE_INT) {
                G_debug(4, "cat = %d val = %d", cvarr.value[i].cat, cvarr.value[i].val.i);
            } else if (ctype == DB_C_TYPE_DOUBLE) {
                G_debug(4, "cat = %d val = %f", cvarr.value[i].cat, cvarr.value[i].val.d);
            }
        }
    }
        
    Vect_copy_head_data(&In, &Out);
    Vect_hist_copy(&In, &Out);
    Vect_hist_command(&Out);


    /* Create buffers' boundaries */
    nlines = Vect_get_num_lines(&In);
    nareas = Vect_get_num_areas (&In);
    /* TODO: don't allocate so much space */
    buffers_count = 0;
    arr_bc = G_malloc((nlines+nareas)*sizeof(struct buf_contours));
    
    /* Lines (and Points) */
    if ((type & GV_POINTS) || (type & GV_LINES)) {
        int ltype;
        
        G_message(_("Lines buffers... "));
        for (line = 1; line <= nlines; line++) {
            int cat;
            
            G_debug(2, "line = %d", line);
            G_percent(line, nlines, 2);

            ltype = Vect_read_line(&In, Points, Cats, line);
            if (!(ltype & type))
                continue;

            if (!Vect_cat_get(Cats, field, &cat))
                continue;

            if (bufcol_opt->answer) {
                ret = db_CatValArray_get_value_di(&cvarr, cat, &size_val);
                if (ret != DB_OK) {
                    G_warning(_("No record for category %d in table <%s>"), cat, Fi->table);
                    continue;
                }
                
                if (size_val < 0.0) {
                    G_warning(_("Attribute is of invalid size (%.3f) for category %d"), size_val, cat);
                    continue;
                }
                
                if (size_val == 0.0)
                    continue;
                
                da = size_val * scale;
                db = da;
                dalpha = 0;
                unit_tolerance = tolerance * MIN(da, db);
                
                G_debug(2, "    dynamic buffer size = %.2f", da);                
                G_debug(2, _("The tolerance in map units: %g"), unit_tolerance);
            }
            
            if (ltype & GV_POINTS) {
                Vect_point_buffer2(Points->x[0], Points->y[0], da, db, dalpha, !(straight_flag->answer), unit_tolerance, &(arr_bc[buffers_count].oPoints));
                arr_bc[buffers_count].iPoints = NULL;
                arr_bc[buffers_count].inner_count = 0;
                buffers_count++;
                    
            }
            else {
                Vect_line_buffer2(Points, da, db, dalpha, !(straight_flag->answer), !(nocaps_flag->answer), unit_tolerance, &(arr_bc[buffers_count].oPoints), &(arr_bc[buffers_count].iPoints), &(arr_bc[buffers_count].inner_count));
                buffers_count++;
            }
        }
    }

    /* Areas */
    if (type & GV_AREA) {
        int centroid;
        
        G_message ( _("Areas buffers... "));
        for (area = 1; area <= nareas; area++) {
            int cat;
            G_percent(area, nareas, 2);
            centroid = Vect_get_area_centroid(&In, area);
            if (centroid == 0)
                continue;
            
            Vect_read_line(&In, NULL, Cats, centroid);
            if (!Vect_cat_get(Cats, field, &cat))
                continue;
            
            if (bufcol_opt->answer) {
                ret = db_CatValArray_get_value_di(&cvarr, cat, &size_val);
                if (ret != DB_OK) {
                    G_warning(_("No record for category %d in table <%s>"), cat, Fi->table);
                    continue;
                }
                
                if (size_val < 0.0) {
                    G_warning(_("Attribute is of invalid size (%.3f) for category %d"), size_val, cat);
                    continue;
                }
                
                if (size_val == 0.0)
                    continue;
                
                da = size_val * scale;
                db = da;
                dalpha = 0;
                unit_tolerance = tolerance * MIN(da, db);
                
                G_debug(2, "    dynamic buffer size = %.2f", da);                
                G_debug(2, _("The tolerance in map units: %g"), unit_tolerance);
            }
            
            Vect_area_buffer2(&In, area, da, db, dalpha, !(straight_flag->answer), !(nocaps_flag->answer), unit_tolerance, &(arr_bc[buffers_count].oPoints), &(arr_bc[buffers_count].iPoints), &(arr_bc[buffers_count].inner_count));
            buffers_count++;
        }
    }

    /* write all buffer contours */
    for (i = 0; i < buffers_count; i++) {
        Vect_write_line(&Out, GV_BOUNDARY, arr_bc[i].oPoints, BCats);
        for (j = 0; j < arr_bc[i].inner_count; j++)
            Vect_write_line(&Out, GV_BOUNDARY, arr_bc[i].iPoints[j], BCats);
    }

    /* Create areas */
    
    /* Break lines */
    G_message (_("Building parts of topology...") );
    Vect_build_partial(&Out, GV_BUILD_BASE);

    G_message(_("Snapping boundaries...") );
    Vect_snap_lines ( &Out, GV_BOUNDARY, 1e-7, NULL);

    G_message(_( "Breaking boundaries...") );
    Vect_break_lines ( &Out, GV_BOUNDARY, NULL );

    G_message(_( "Removing duplicates...") );
    Vect_remove_duplicates ( &Out, GV_BOUNDARY, NULL );

    /* Dangles and bridges don't seem to be necessary if snapping is small enough. */
    /*
    G_message (  "Removing dangles..." );
    Vect_remove_dangles ( &Out, GV_BOUNDARY, -1, NULL, stderr );

    G_message (  "Removing bridges..." );
    Vect_remove_bridges ( &Out, NULL, stderr );
    */

    G_message(_( "Attaching islands..."));
    Vect_build_partial(&Out, GV_BUILD_ATTACH_ISLES);
    
    /* Calculate new centroids for all areas */
    nareas = Vect_get_num_areas ( &Out );
    Areas = (char *) G_calloc ( nareas+1, sizeof(char) );
    for (area = 1; area <= nareas; area++) {
        double x, y;
        
        G_debug ( 3, "area = %d", area );

        if (!Vect_area_alive(&Out, area))
            continue;
                    
        ret = Vect_get_point_in_area(&Out, area, &x, &y);
        if ( ret < 0 ) {
            G_warning (_("Cannot calculate area centroid"));
            continue;
        }
        
        ret = point_in_buffer(arr_bc, buffers_count, &Out, x, y);
        
        if (ret) {
            G_debug (3, "  -> in buffer");
            Areas[area] = 1;
        }
    }
    
    /* Make a list of boundaries to be deleted (both sides inside) */
    nlines = Vect_get_num_lines(&Out);
    G_debug ( 3, "nlines = %d", nlines );
    Lines = (char *) G_calloc ( nlines+1, sizeof(char) );

    for ( line = 1; line <= nlines; line++ ) {
        int j, side[2], areas[2];
        
        G_debug ( 3, "line = %d", line );
        
        if ( !Vect_line_alive ( &Out, line ) )
            continue;
        
        Vect_get_line_areas ( &Out, line, &side[0], &side[1] );
        
        for ( j = 0; j < 2; j++ ) { 
            if ( side[j] == 0 ) { /* area/isle not build */
                areas[j] = 0;
            } else if ( side[j] > 0 ) { /* area */
                areas[j] = side[j];
            } else { /* < 0 -> island */
                areas[j] = Vect_get_isle_area ( &Out, abs ( side[j] ) );
            }
        }
        
        G_debug ( 3, " areas = %d , %d -> Areas = %d, %d", areas[0], areas[1], Areas[areas[0]], Areas[areas[1]] );
        if ( Areas[areas[0]] && Areas[areas[1]] )
            Lines[line] = 1;
    }
    G_free(Areas);
    
    /* Delete boundaries */
    for ( line = 1; line <= nlines; line++ ) {
        if ( Lines[line] ) {
            G_debug ( 3, " delete line %d", line );
            Vect_delete_line ( &Out, line );
        }
    }

    G_free( Lines );

    /* Create new centroids */
    Vect_reset_cats ( Cats );
    Vect_cat_set (Cats, 1, 1 );
    nareas = Vect_get_num_areas(&Out);

    for (area = 1; area <= nareas; area++) {
        double x, y;
        
        G_debug ( 3, "area = %d", area );

        if (!Vect_area_alive(&Out, area))
            continue;
                    
        ret = Vect_get_point_in_area(&Out, area, &x, &y);
        if ( ret < 0 ) {
            G_warning (_("Cannot calculate area centroid"));
            continue;
        }
        
        ret = point_in_buffer(arr_bc, buffers_count, &Out, x, y);
        
        if (ret) {
            Vect_reset_line(Points);
            Vect_append_point(Points, x, y, 0.);
            Vect_write_line(&Out, GV_CENTROID, Points, Cats);
        }
    }

    /* free arr_bc[] */
    /* will only slow down the module
    for (i = 0; i < buffers_count; i++) {
        Vect_destroy_line_struct(arr_bc[i].oPoints);
        for (j = 0; j < arr_bc[i].inner_count; j++)
            Vect_destroy_line_struct(arr_bc[i].iPoints[j]);
        G_free(arr_bc[i].iPoints);
    } */
    
    G_message(_("Attaching centroids...") );
    Vect_build_partial ( &Out, GV_BUILD_CENTROIDS);
    
    stop ( &In, &Out );
    exit(EXIT_SUCCESS);
}

