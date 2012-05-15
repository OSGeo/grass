
/****************************************************************************
 *
 * MODULE:       r3.out.netcdf 
 *   	    	
 * AUTHOR(S):    Soeren Gebbert
 *
 * PURPOSE:      Export a 3D raster map as netcdf file  
 *
 * COPYRIGHT:    (C) 2012 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *   	    	License (>=v2). Read the file COPYING that comes with GRASS
 *   	    	for details.
 * 
 * TODO: Add time zone support to time variable
 * TODO: Implement support for coordinate reference system defined here:
 *       http://cf-pcmdi.llnl.gov/documents/cf-conventions/1.6/cf-conventions.html#coordinate-system
 *       https://cf-pcmdi.llnl.gov/trac/wiki/Cf2CrsWkt
 *       http://trac.osgeo.org/gdal/wiki/NetCDF_ProjectionTestingStatus
 *
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netcdf.h>
#include <grass/gis.h>
#include <grass/raster3d.h>
#include <grass/glocale.h>
#include <grass/gprojects.h>

#define NDIMS 3
#define LONG_NAME "long_name"
#define STANDARD_NAME "standard_name"
#define LAT_NAME "latitude"
#define LAT_LONG_NAME "Latitude values"
#define LON_NAME "longitude"
#define LON_LONG_NAME "Longitude values"
#define TIME_NAME "time"
#define X_NAME "x"
#define X_LONG_NAME "x-axsis coordinates"
#define Y_NAME "y"
#define Y_LONG_NAME "y-axsis coordinates"
#define Z_NAME "z"
#define Z_LONG_NAME "z-axsis coordinates"
#define UNITS "units"
#define DEGREES_EAST "degrees_east"
#define DEGREES_NORTH "degrees_north"

#define ERR(e) {fatalError(nc_strerror(e));}

/* structs */
typedef struct {
    struct Option *input, *output;
    struct Flag *mask;
} paramType;

RASTER3D_Map *map;
paramType param;

/*---------------------------------------------------------------------------*/

/* Simple error handling routine, will eventually replace this with
 * RASTER3D_fatalError.
 */
static void fatalError(const char *errorMsg)
{
    if (map != NULL) {
        /* should unopen map here! */
        if (!Rast3d_close(map))
            fatalError(_("Unable to close 3D raster map"));
    }
    G_fatal_error("%s", errorMsg);
}

/*---------------------------------------------------------------------------*/

/* Convenient way to set up the arguments we are expecting
 */
static void setParams()
{
    param.input = G_define_standard_option(G_OPT_R3_INPUTS);
    
    param.output = G_define_standard_option(G_OPT_F_OUTPUT);
    param.output->key = "output";
    param.output->description = _("Name for netcdf output file");

    param.mask = G_define_flag();
    param.mask->key = 'm';
    param.mask->description = _("Use 3D raster mask (if exists) with input map");
}

static void write_netcdf_header(int ncid, RASTER3D_Region *region, int *varid)
{
    int retval, typeIntern, time;
    size_t i;
    int is_time = 0;
    float c;
    int lat_dimid = 0, lon_dimid = 0, time_dimid = 0;
    int lat_varid = 0, lon_varid = 0, time_varid = 0;
    struct Key_Value *pkv, *ukv;
    int dimids[NDIMS];
    struct Cell_head window;
    
    G_get_window(&window);
    
    typeIntern = Rast3d_tile_type_map(map);
    
    if (window.proj == PROJECTION_LL) {
        if ((retval = nc_def_dim(ncid, LON_NAME, region->cols, &lon_dimid))) ERR(retval);
        if ((retval = nc_def_dim(ncid, LAT_NAME, region->rows, &lat_dimid))) ERR(retval);
        
        if ((retval = nc_def_var(ncid, LON_NAME, NC_FLOAT, 1, &lon_dimid, &lon_varid))) ERR(retval); 
        if ((retval = nc_def_var(ncid, LAT_NAME, NC_FLOAT, 1, &lat_dimid, &lat_varid))) ERR(retval);      

        if ((retval = nc_put_att_text(ncid, lon_varid, UNITS, strlen(DEGREES_EAST), DEGREES_EAST))) ERR(retval);
        if ((retval = nc_put_att_text(ncid, lon_varid, LONG_NAME, strlen(LON_LONG_NAME), LON_LONG_NAME))) ERR(retval);
        if ((retval = nc_put_att_text(ncid, lon_varid, STANDARD_NAME, strlen(LON_NAME), LON_NAME))) ERR(retval);
        if ((retval = nc_put_att_text(ncid, lat_varid, UNITS, strlen(DEGREES_NORTH), DEGREES_NORTH))) ERR(retval);
        if ((retval = nc_put_att_text(ncid, lat_varid, LONG_NAME, strlen(LAT_LONG_NAME), LAT_LONG_NAME))) ERR(retval);
        if ((retval = nc_put_att_text(ncid, lat_varid, STANDARD_NAME, strlen(LAT_NAME), LAT_NAME))) ERR(retval);
    } else {
        if ((retval = nc_def_dim(ncid, X_NAME, region->cols, &lon_dimid))) ERR(retval);
        if ((retval = nc_def_dim(ncid, Y_NAME, region->rows, &lat_dimid))) ERR(retval);
        
        if ((retval = nc_def_var(ncid, X_NAME, NC_FLOAT, 1, &lon_dimid, &lon_varid))) ERR(retval); 
        if ((retval = nc_def_var(ncid, Y_NAME, NC_FLOAT, 1, &lat_dimid, &lat_varid))) ERR(retval);      

        if ((retval = nc_put_att_text(ncid, lon_varid, UNITS, strlen("meter"), "meter"))) ERR(retval);
        if ((retval = nc_put_att_text(ncid, lon_varid, LONG_NAME, strlen(X_LONG_NAME), X_LONG_NAME))) ERR(retval);
        if ((retval = nc_put_att_text(ncid, lon_varid, STANDARD_NAME, strlen(X_NAME), X_NAME))) ERR(retval);
        if ((retval = nc_put_att_text(ncid, lat_varid, UNITS, strlen("meter"), "meter"))) ERR(retval);
        if ((retval = nc_put_att_text(ncid, lat_varid, LONG_NAME, strlen(Y_LONG_NAME), Y_LONG_NAME))) ERR(retval);
        if ((retval = nc_put_att_text(ncid, lat_varid, STANDARD_NAME, strlen(Y_NAME), Y_NAME))) ERR(retval);
    }
        
    is_time = 0;
    
    if(Rast3d_get_vertical_unit(map) && G_strncasecmp(Rast3d_get_vertical_unit(map), "units", 5) != 0) {
        /* Time handling */
        if(G_is_units_type_temporal(Rast3d_get_vertical_unit2(map))) {
            is_time = 1;
            char long_name[1024];
            char time_unit[1024];
            
            G_snprintf(long_name, 1024, "Time in %s", Rast3d_get_vertical_unit(map));
            
            if ((retval = nc_def_dim(ncid, TIME_NAME, region->depths, &time_dimid))) ERR(retval);
            if ((retval = nc_def_var(ncid, TIME_NAME, NC_INT, 1, &time_dimid, &time_varid))) ERR(retval);
            
            /* Temporal unit */
            if(G_has_raster3d_timestamp(map->fileName, map->mapset)) {
                struct TimeStamp ts;
                G_read_raster3d_timestamp(map->fileName, map->mapset, &ts);
                G_snprintf(time_unit, 1024, "%s since %d-%02d-%02d %02d:%02d:%02.0f", 
                           Rast3d_get_vertical_unit(map), ts.dt[0].year, ts.dt[0].month, ts.dt[0].day,
                           ts.dt[0].hour,ts.dt[0].minute,ts.dt[0].second);
            } else {
                G_snprintf(time_unit, 1024, "%s since %s", Rast3d_get_vertical_unit(map), "1900-01-01 00:00:00");
            }
            
            if ((retval = nc_put_att_text(ncid, time_varid, UNITS, strlen(time_unit), time_unit))) ERR(retval);
            if ((retval = nc_put_att_text(ncid, time_varid, LONG_NAME, strlen(long_name), long_name))) ERR(retval)
            if ((retval = nc_put_att_text(ncid, time_varid, "calendar", strlen("gregorian"), "gregorian"))) ERR(retval);
            
        } else {
            if ((retval = nc_def_dim(ncid, Z_NAME, region->depths, &time_dimid))) ERR(retval);;
            if ((retval = nc_def_var(ncid, Z_NAME, NC_FLOAT, 1, &time_dimid, &time_varid))) ERR(retval);
            if ((retval = nc_put_att_text(ncid, time_varid, UNITS, strlen(Rast3d_get_vertical_unit(map)), Rast3d_get_vertical_unit(map)))) ERR(retval);
            if ((retval = nc_put_att_text(ncid, time_varid, LONG_NAME, strlen(Z_LONG_NAME), Z_LONG_NAME))) ERR(retval);
        }
    }else {
        if ((retval = nc_def_dim(ncid, Z_NAME, region->depths, &time_dimid))) ERR(retval);
        if ((retval = nc_def_var(ncid, Z_NAME, NC_FLOAT, 1, &time_dimid, &time_varid))) ERR(retval);
        if ((retval = nc_put_att_text(ncid, time_varid, UNITS, strlen("meter"), "meter"))) ERR(retval);
        if ((retval = nc_put_att_text(ncid, time_varid, LONG_NAME, strlen(Z_LONG_NAME), Z_LONG_NAME))) ERR(retval);
    }
    /* z - axsis orientation */
    if ((retval = nc_put_att_text(ncid, time_varid, "positive", strlen("up"), "up"))) ERR(retval);
    
    dimids[0] = time_dimid;
    dimids[1] = lat_dimid;
    dimids[2] = lon_dimid;
    
    if(typeIntern == FCELL_TYPE) {
        if ((retval = nc_def_var(ncid, param.input->answer, NC_FLOAT, NDIMS, dimids, varid))) ERR(retval); 
    } else {
        if ((retval = nc_def_var(ncid, param.input->answer, NC_DOUBLE, NDIMS, dimids, varid))) ERR(retval); 
    }
    
   /* End define mode. */
   if ((retval = nc_enddef(ncid))) ERR(retval);  
    
    /* Build coordinates, we need to use the cell center in case of spatial dimensions */
    for(i = 0; i < region->cols; i++) {
        c = region->west + i*region->ew_res + 0.5*region->ew_res;
        nc_put_var1_float(ncid, lon_varid, &i, &c);     
    }
    
    for(i = 0; i < region->rows; i++) {
        c = region->south + i*region->ns_res + 0.5*region->ns_res;
        nc_put_var1_float(ncid, lat_varid, &i, &c);     
    }
    
    for(i = 0; i < region->depths; i++) {
        if(is_time)
            c = region->bottom + i*region->tb_res;
        else
            c = region->bottom + i*region->tb_res + 0.5*region->tb_res;
        time = (int)c;
        if(is_time)
            nc_put_var1_int(ncid, time_varid, &i, &time);  
        else
            nc_put_var1_float(ncid, time_varid, &i, &c);     
    }
    
    /* Lets try to specify the projection information */
    pkv = G_get_projinfo();
    ukv = G_get_projunits();
    
    if(pkv && ukv) {
        for(i = 0; i < pkv->nitems; i++)
            fprintf(stderr, "%s : %s\n", pkv->key[i], pkv->value[i]);
        for(i = 0; i < ukv->nitems; i++)
            fprintf(stderr, "%s : %s\n", ukv->key[i], ukv->value[i]);  
    }
    
    if(window.proj != PROJECTION_XY)
        ; // Do projection
    
    if(pkv)
        G_free_key_value(pkv);
    if(ukv)
        G_free_key_value(ukv);
}

/*---------------------------------------------------------------------------*/

static void write_netcdf_data(int ncid, RASTER3D_Region *region, int varid)
{
    DCELL dvalue;
    FCELL fvalue;
    int x, y, z;
    size_t coords[3];
    int rows, cols, depths, typeIntern;

    rows = region->rows;
    cols = region->cols;
    depths = region->depths;

    typeIntern = Rast3d_tile_type_map(map);

    for (z = 0; z < depths; z++) {
        G_percent(z, depths, 1);
        for (y = rows - 1; y >= 0; y--) { /* rows count from south to north */
            for (x = 0; x < cols; x++) {
                coords[0] = z;
                coords[1] = y;
                coords[2] = x;
                 if (typeIntern == FCELL_TYPE) {
                    
                    Rast3d_get_value(map, x, y, z, &fvalue, FCELL_TYPE);
                    
                    if (!Rast3d_is_null_value_num(&fvalue, FCELL_TYPE)) {
                        nc_put_var1_float(ncid, varid, coords, &fvalue);
                    }
                } else {
                    
                    Rast3d_get_value(map, x, y, z, &dvalue, DCELL_TYPE);
                    
                    if (!Rast3d_is_null_value_num(&dvalue, DCELL_TYPE)) {
                        nc_put_var1_double(ncid, varid, coords, &dvalue);
                    }
                }
            }
        }
    }
    G_percent(1, 1, 1);
    G_percent_reset();
}

/*---------------------------------------------------------------------------*/

int main(int argc, char *argv[])
{
    RASTER3D_Region region;
    
    int ncid;
    int retval;
    int varid;
    
    int changemask = 0;
    struct GModule *module;

    /* Initialize GRASS */
    G_gisinit(argv[0]);
    module = G_define_module();
    G_add_keyword(_("raster3d"));
    G_add_keyword(_("netcdf"));
    G_add_keyword(_("export"));
    module->description =
        _("Export a 3D raster map as netcdf file.");

    /* Get parameters from user */
    setParams();

    /* Have GRASS get inputs */
    if (G_parser(argc, argv))
        exit(EXIT_FAILURE);

    if (NULL == G_find_raster3d(param.input->answer, ""))
        Rast3d_fatal_error(_("3D raster map <%s> not found"), param.input->answer);

    /* Initiate the default settings */
    Rast3d_init_defaults();

    /* Figure out the current region settings */
    Rast3d_get_window(&region);

    /* Open the map and use XY cache mode */
    map = Rast3d_open_cell_old(param.input->answer, G_find_raster3d(param.input->answer, ""), &region,
                          RASTER3D_TILE_SAME_AS_FILE, RASTER3D_USE_CACHE_DEFAULT);

    if (map == NULL)
        G_fatal_error(_("Error opening 3d raster map <%s>"), param.input->answer);

    /* Create netcdf file */
    if ((retval = nc_create(param.output->answer, NC_CLOBBER, &ncid))) ERR(retval);
    
    write_netcdf_header(ncid, &region, &varid);

    /*if requested set the Mask on */
    if (param.mask->answer) {
        if (Rast3d_mask_file_exists()) {
            changemask = 0;
            if (Rast3d_mask_is_off(map)) {
                Rast3d_mask_on(map);
                changemask = 1;
            }
        }
    }
    
    write_netcdf_data(ncid, &region, varid);

    /*We set the Mask off, if it was off bevor */
    if (param.mask->answer) {
        if (Rast3d_mask_file_exists())
            if (Rast3d_mask_is_on(map) && changemask)
                Rast3d_mask_off(map);
    }

    /* Close files and exit */
    if (!Rast3d_close(map))
        fatalError(_("Unable to close 3D raster map"));
    
    /* Close the netcdf file */
    if ((retval = nc_close(ncid))) ERR(retval);
    
    return 0;
}
