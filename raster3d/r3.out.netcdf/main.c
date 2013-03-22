
/****************************************************************************
 *
 * MODULE:       r3.out.netCDF 
 *   	    	
 * AUTHOR(S):    Soeren Gebbert
 *
 * PURPOSE:      Export a 3D raster map as netCDF file  
 *
 * COPYRIGHT:    (C) 2012 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *   	    	License (>=v2). Read the file COPYING that comes with GRASS
 *   	    	for details.
 * 
 * TODO: Add time zone support to time variable
 * TODO: Implement better support for CF coordinate reference system defined here:
 *       http://cf-pcmdi.llnl.gov/documents/cf-conventions/1.6/cf-conventions.html#coordinate-system
 *       https://cf-pcmdi.llnl.gov/trac/wiki/Cf2CrsWkt
 *       http://trac.osgeo.org/gdal/wiki/NetCDF_ProjectionTestingStatus
 *
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netcdf.h>
#include <grass/datetime.h>
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
#define X_STANDARD_NAME "projection_x_coordinate"
#define X_LONG_NAME "x coordinate of projection"
#define Y_NAME "y"
#define Y_LONG_NAME "y coordinate of projection"
#define Y_STANDARD_NAME "projection_y_coordinate"
#define Z_NAME "z"
#define Z_LONG_NAME "z coordinate of projection"
#define Z_STANDARD_NAME "projection_z_coordinate"
#define UNITS "units"
#define DEGREES_EAST "degrees_east"
#define DEGREES_NORTH "degrees_north"
#define HISTORY_TEXT "GRASS GIS 7 netCDF export of r3.out.netcdf"
#define CF_SUPPORT "CF-1.5"

#define ERR(e) {fatalError(nc_strerror(e));}

/* structs */
typedef struct
{
    struct Option *input, *output, *null;
    struct Flag *mask, *proj;
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
	    G_fatal_error(_
			  ("Unable to close 3D raster map while catching error: %s"),
			  errorMsg);
    }
    G_fatal_error("%s", errorMsg);
}

/*---------------------------------------------------------------------------*/

/* Convenient way to set up the arguments we are expecting
 */
static void setParams()
{
    param.input = G_define_standard_option(G_OPT_R3_INPUT);

    param.output = G_define_standard_option(G_OPT_F_OUTPUT);
    param.output->key = "output";
    param.output->description = _("Name for netCDF output file");

    param.null = G_define_option();
    param.null->key = "null";
    param.null->type = TYPE_DOUBLE;
    param.null->required = NO;
    param.null->multiple = NO;
    param.null->description =
	_
	("The value to be used for null values, default is the netCDF standard");

    param.proj = G_define_flag();
    param.proj->key = 'p';
    param.proj->description =
	_("Export projection information as wkt and proj4 parameter");

    param.mask = G_define_flag();
    param.mask->key = 'm';
    param.mask->description =
	_("Use 3D raster mask (if exists) with input map");
}

static void write_netcdf_header(int ncid, RASTER3D_Region * region,
				int *varid, char write_proj, char *null)
{
    int retval, typeIntern, time;
    size_t i;
    int is_time = 0;
    int is_absolute_time = 0;
    float c;
    int lat_dimid = 0, lon_dimid = 0, time_dimid = 0;
    int lat_varid = 0, lon_varid = 0, time_varid = 0;
    int dimids[NDIMS];
    struct Cell_head window;
    double min, max;

    /* global attributes */
    if ((retval =
	 nc_put_att_text(ncid, NC_GLOBAL, "Conventions", strlen(CF_SUPPORT),
			 CF_SUPPORT)))
	ERR(retval);
    if ((retval =
	 nc_put_att_text(ncid, NC_GLOBAL, "history", strlen(HISTORY_TEXT),
			 HISTORY_TEXT)))
	ERR(retval);

    G_get_window(&window);

    /* Projection parameter */
    if (window.proj != PROJECTION_XY && write_proj) {
	struct Key_Value *pkv, *ukv;
	struct pj_info pjinfo;
	char *proj4, *proj4mod;
	const char *unfact;
	int crs_dimid = 0, crs_varid;

	if ((retval =
	     nc_def_var(ncid, "crs", NC_CHAR, 0, &crs_dimid, &crs_varid)))
	    ERR(retval);

	pkv = G_get_projinfo();
	ukv = G_get_projunits();

	pj_get_kv(&pjinfo, pkv, ukv);
	proj4 = pj_get_def(pjinfo.pj, 0);
	pj_free(pjinfo.pj);

#ifdef HAVE_OGR
	/* We support the CF suggestion crs_wkt and the gdal spatil_ref attribute */
	if ((retval =
	     nc_put_att_text(ncid, crs_varid, "crs_wkt",
			     strlen(GPJ_grass_to_wkt(pkv, ukv, 0, 0)),
			     GPJ_grass_to_wkt(pkv, ukv, 0, 0))))
	    ERR(retval);
	if ((retval =
	     nc_put_att_text(ncid, crs_varid, "spatial_ref",
			     strlen(GPJ_grass_to_wkt(pkv, ukv, 0, 0)),
			     GPJ_grass_to_wkt(pkv, ukv, 0, 0))))
	    ERR(retval);
#endif
	/* Code from g.proj:
	 * GRASS-style PROJ.4 strings don't include a unit factor as this is
	 * handled separately in GRASS - must include it here though */
	unfact = G_find_key_value("meters", ukv);
	if (unfact != NULL && (strcmp(pjinfo.proj, "ll") != 0))
	    G_asprintf(&proj4mod, "%s +to_meter=%s", proj4, unfact);
	else
	    proj4mod = G_store(proj4);

	pj_dalloc(proj4);

	if ((retval =
	     nc_put_att_text(ncid, crs_varid, "crs_proj4", strlen(proj4mod),
			     proj4mod)))
	    ERR(retval);

	if (pkv)
	    G_free_key_value(pkv);
	if (ukv)
	    G_free_key_value(ukv);
    }

    typeIntern = Rast3d_tile_type_map(map);

    if (window.proj == PROJECTION_LL) {
	/* X-Axis */
	if ((retval = nc_def_dim(ncid, LON_NAME, region->cols, &lon_dimid)))
	    ERR(retval);

	if ((retval =
	     nc_def_var(ncid, LON_NAME, NC_FLOAT, 1, &lon_dimid, &lon_varid)))
	    ERR(retval);

	if ((retval =
	     nc_put_att_text(ncid, lon_varid, UNITS, strlen(DEGREES_EAST),
			     DEGREES_EAST)))
	    ERR(retval);
	if ((retval = nc_put_att_text(ncid, lon_varid, LONG_NAME,
				      strlen(LON_LONG_NAME), LON_LONG_NAME)))
	    ERR(retval);
	if ((retval =
	     nc_put_att_text(ncid, lon_varid, STANDARD_NAME, strlen(LON_NAME),
			     LON_NAME)))
	    ERR(retval);

	/* Y-Axis */
	if ((retval = nc_def_dim(ncid, LAT_NAME, region->rows, &lat_dimid)))
	    ERR(retval);

	if ((retval =
	     nc_def_var(ncid, LAT_NAME, NC_FLOAT, 1, &lat_dimid, &lat_varid)))
	    ERR(retval);

	if ((retval =
	     nc_put_att_text(ncid, lat_varid, UNITS, strlen(DEGREES_NORTH),
			     DEGREES_NORTH)))
	    ERR(retval);
	if ((retval = nc_put_att_text(ncid, lat_varid, LONG_NAME,
				      strlen(LAT_LONG_NAME), LAT_LONG_NAME)))
	    ERR(retval);
	if ((retval =
	     nc_put_att_text(ncid, lat_varid, STANDARD_NAME, strlen(LAT_NAME),
			     LAT_NAME)))
	    ERR(retval);
    }
    else {
	/* X-Axis */
	if ((retval = nc_def_dim(ncid, X_NAME, region->cols, &lon_dimid)))
	    ERR(retval);

	if ((retval =
	     nc_def_var(ncid, X_NAME, NC_FLOAT, 1, &lon_dimid, &lon_varid)))
	    ERR(retval);

	if ((retval = nc_put_att_text(ncid, lon_varid, UNITS, strlen("meter"),
				      "meter")))
	    ERR(retval);
	if ((retval =
	     nc_put_att_text(ncid, lon_varid, LONG_NAME, strlen(X_LONG_NAME),
			     X_LONG_NAME)))
	    ERR(retval);
	if ((retval = nc_put_att_text(ncid, lon_varid, STANDARD_NAME,
				      strlen(X_STANDARD_NAME),
				      X_STANDARD_NAME)))
	    ERR(retval);
	/* Y-Axis */
	if ((retval = nc_def_dim(ncid, Y_NAME, region->rows, &lat_dimid)))
	    ERR(retval);

	if ((retval =
	     nc_def_var(ncid, Y_NAME, NC_FLOAT, 1, &lat_dimid, &lat_varid)))
	    ERR(retval);

	if ((retval = nc_put_att_text(ncid, lat_varid, UNITS, strlen("meter"),
				      "meter")))
	    ERR(retval);
	if ((retval =
	     nc_put_att_text(ncid, lat_varid, LONG_NAME, strlen(Y_LONG_NAME),
			     Y_LONG_NAME)))
	    ERR(retval);
	if ((retval = nc_put_att_text(ncid, lat_varid, STANDARD_NAME,
				      strlen(Y_STANDARD_NAME),
				      Y_STANDARD_NAME)))
	    ERR(retval);
    }

    /* We set the vertical axis and its unit. Units can be spatial
     * or temporal. Temporal can be absolute or relative. 
     */

    is_time = 0;
    is_absolute_time = 0;

    if (Rast3d_get_vertical_unit(map) &&
	G_strncasecmp(Rast3d_get_vertical_unit(map), "units", 5) != 0) {
	/* Time handling */
	if (G_is_units_type_temporal(Rast3d_get_vertical_unit2(map))) {
	    is_time = 1;
	    char long_name[1024];
	    char time_unit[1024];

	    G_snprintf(long_name, 1024, "Time in %s",
		       Rast3d_get_vertical_unit(map));

	    if ((retval =
		 nc_def_dim(ncid, TIME_NAME, region->depths, &time_dimid)))
		ERR(retval);
	    if ((retval =
		 nc_def_var(ncid, TIME_NAME, NC_INT, 1, &time_dimid,
			    &time_varid)))
		ERR(retval);

	    /* Temporal unit */
	    if (G_has_raster3d_timestamp(map->fileName, map->mapset)) {
		struct TimeStamp ts;

		G_read_raster3d_timestamp(map->fileName, map->mapset, &ts);

		/* Days since datum in ISO norm */
		if (datetime_is_absolute(&ts.dt[0])) {
		    is_absolute_time = 1;
		    G_snprintf(time_unit, 1024,
			       "%s since %d-%02d-%02d %02d:%02d:%02.0f",
			       Rast3d_get_vertical_unit(map), ts.dt[0].year,
			       ts.dt[0].month, ts.dt[0].day, ts.dt[0].hour,
			       ts.dt[0].minute, ts.dt[0].second);
		}
		else {
		    G_snprintf(time_unit, 1024, "%s",
			       Rast3d_get_vertical_unit(map));
		}
	    }
	    else {
		G_snprintf(time_unit, 1024, "%s since %s",
			   Rast3d_get_vertical_unit(map),
			   "1900-01-01 00:00:00");
	    }

	    if ((retval =
		 nc_put_att_text(ncid, time_varid, UNITS, strlen(time_unit),
				 time_unit)))
		ERR(retval);
	    if ((retval =
		 nc_put_att_text(ncid, time_varid, LONG_NAME,
				 strlen(long_name), long_name)))
		ERR(retval)
		    if (is_absolute_time) {
		    if ((retval =
			 nc_put_att_text(ncid, time_varid, "calendar",
					 strlen("gregorian"), "gregorian")))
			ERR(retval);
		}
		else {
		    if ((retval =
			 nc_put_att_text(ncid, time_varid, "calendar",
					 strlen("none"), "none")))
			ERR(retval);
		}
	}
	else {
	    if ((retval =
		 nc_def_dim(ncid, Z_NAME, region->depths, &time_dimid)))
		ERR(retval);
	    ;
	    if ((retval =
		 nc_def_var(ncid, Z_NAME, NC_FLOAT, 1, &time_dimid,
			    &time_varid)))
		ERR(retval);
	    if ((retval =
		 nc_put_att_text(ncid, time_varid, UNITS,
				 strlen(Rast3d_get_vertical_unit(map)),
				 Rast3d_get_vertical_unit(map))))
		ERR(retval);
	    if ((retval =
		 nc_put_att_text(ncid, time_varid, LONG_NAME,
				 strlen(Z_LONG_NAME), Z_LONG_NAME)))
		ERR(retval);
	    if ((retval =
		 nc_put_att_text(ncid, time_varid, STANDARD_NAME,
				 strlen(Z_STANDARD_NAME), Z_STANDARD_NAME)))
		ERR(retval);
	}
    }
    else {
	/* Default is z unit meter */
	if ((retval = nc_def_dim(ncid, Z_NAME, region->depths, &time_dimid)))
	    ERR(retval);
	if ((retval =
	     nc_def_var(ncid, Z_NAME, NC_FLOAT, 1, &time_dimid, &time_varid)))
	    ERR(retval);
	if ((retval =
	     nc_put_att_text(ncid, time_varid, UNITS, strlen("meter"),
			     "meter")))
	    ERR(retval);
	if ((retval =
	     nc_put_att_text(ncid, time_varid, LONG_NAME, strlen(Z_LONG_NAME),
			     Z_LONG_NAME)))
	    ERR(retval);
	if ((retval =
	     nc_put_att_text(ncid, time_varid, STANDARD_NAME,
			     strlen(Z_STANDARD_NAME), Z_STANDARD_NAME)))
	    ERR(retval);
    }

    /* z - axis orientation */
    if ((retval =
	 nc_put_att_text(ncid, time_varid, "positive", strlen("up"), "up")))
	ERR(retval);

    /* Axis identifier attributes */
    if ((retval = nc_put_att_text(ncid, lon_varid, "axis", 1, "X")))
	ERR(retval);
    if ((retval = nc_put_att_text(ncid, lat_varid, "axis", 1, "Y")))
	ERR(retval);
    if (is_time) {
	if ((retval = nc_put_att_text(ncid, time_varid, "axis", 1, "T")))
	    ERR(retval);
    }
    else {
	if ((retval = nc_put_att_text(ncid, time_varid, "axis", 1, "Z")))
	    ERR(retval);
    }

    dimids[0] = time_dimid;
    dimids[1] = lat_dimid;
    dimids[2] = lon_dimid;


    Rast3d_range_load(map);
    Rast3d_range_min_max(map, &min, &max);

    if (typeIntern == FCELL_TYPE) {
	if ((retval =
	     nc_def_var(ncid, param.input->answer, NC_FLOAT, NDIMS, dimids,
			varid)))
	    ERR(retval);
	/* Set the range values */
	float fmin = min;

	float fmax = max;

	if ((retval =
	     nc_put_att_float(ncid, *varid, "valid_min", NC_FLOAT, 1, &fmin)))
	    ERR(retval);
	if ((retval =
	     nc_put_att_float(ncid, *varid, "valid_max", NC_FLOAT, 1, &fmax)))
	    ERR(retval);

	if (null) {
	    float null_val = (float)atof(null);

	    if ((retval =
		 nc_put_att_float(ncid, *varid, "missing_value", NC_FLOAT, 1,
				  &null_val)))
		ERR(retval);
	    if ((retval =
		 nc_put_att_float(ncid, *varid, "_FillValue", NC_FLOAT, 1,
				  &null_val)))
		ERR(retval);
	}
    }
    else {
	if ((retval =
	     nc_def_var(ncid, param.input->answer, NC_DOUBLE, NDIMS, dimids,
			varid)))
	    ERR(retval);
	/* Set the range values */
	if ((retval =
	     nc_put_att_double(ncid, *varid, "valid_min", NC_DOUBLE, 1,
			       &min)))
	    ERR(retval);
	if ((retval =
	     nc_put_att_double(ncid, *varid, "valid_max", NC_DOUBLE, 1,
			       &max)))
	    ERR(retval);

	if (null) {
	    double null_val = (double)atof(null);

	    if ((retval =
		 nc_put_att_double(ncid, *varid, "missing_value", NC_DOUBLE,
				   1, &null_val)))
		ERR(retval);
	    if ((retval =
		 nc_put_att_double(ncid, *varid, "_FillValue", NC_DOUBLE, 1,
				   &null_val)))
		ERR(retval);
	}
    }

    if (window.proj != PROJECTION_XY && write_proj) {
	if ((retval =
	     nc_put_att_text(ncid, *varid, "grid_mapping", strlen("crs"),
			     "crs")))
	    ERR(retval);
    }

    /* End define mode. */
    if ((retval = nc_enddef(ncid)))
	ERR(retval);

    /* 
     * Build coordinates, we need to use the cell center in case of spatial dimensions 
     * */

    for (i = 0; i < region->cols; i++) {
	c = region->west + i * region->ew_res + 0.5 * region->ew_res;
	nc_put_var1_float(ncid, lon_varid, &i, &c);
    }

    for (i = 0; i < region->rows; i++) {
	/* c = region->south + i * region->ns_res + 0.5 * region->ns_res; */
	c = region->north - i * region->ns_res - 0.5 * region->ns_res;
	nc_put_var1_float(ncid, lat_varid, &i, &c);
    }

    for (i = 0; i < region->depths; i++) {
	if (is_time) {
	    c = i * region->tb_res;
	    time = (int)c;
	    nc_put_var1_int(ncid, time_varid, &i, &time);
	}
	else {
	    c = region->bottom + i * region->tb_res + 0.5 * region->tb_res;
	    nc_put_var1_float(ncid, time_varid, &i, &c);
	}
    }
}

/*---------------------------------------------------------------------------*/

static void write_netcdf_data(int ncid, RASTER3D_Region * region, int varid)
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
	for (y = 0; y < rows; y++) {
	    for (x = 0; x < cols; x++) {
		coords[0] = z;
		coords[1] = y;
		coords[2] = x;
		if (typeIntern == FCELL_TYPE) {

		    Rast3d_get_value(map, x, y, z, &fvalue, FCELL_TYPE);

		    if (!Rast3d_is_null_value_num(&fvalue, FCELL_TYPE)) {
			nc_put_var1_float(ncid, varid, coords, &fvalue);
		    }
		}
		else {

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
    G_add_keyword(_("export"));
    G_add_keyword(_("netCDF"));
    module->description = _("Export a 3D raster map as netCDF file.");

    /* Get parameters from user */
    setParams();

    /* Have GRASS get inputs */
    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    if (NULL == G_find_raster3d(param.input->answer, ""))
	Rast3d_fatal_error(_("3D raster map <%s> not found"),
			   param.input->answer);

    /* Initiate the default settings */
    Rast3d_init_defaults();

    /* Figure out the current region settings */
    Rast3d_get_window(&region);

    /* Open the map and use XY cache mode */
    map =
	Rast3d_open_cell_old(param.input->answer,
			     G_find_raster3d(param.input->answer, ""),
			     &region, RASTER3D_TILE_SAME_AS_FILE,
			     RASTER3D_USE_CACHE_DEFAULT);

    if (map == NULL)
	G_fatal_error(_("Error opening 3d raster map <%s>"),
		      param.input->answer);

    /* Create netCDF file */
    if ((retval = nc_create(param.output->answer, NC_CLOBBER, &ncid)))
	ERR(retval);

    write_netcdf_header(ncid, &region, &varid, param.proj->answer,
			param.null->answer);

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

    /*We set the Mask off, if it was off before */
    if (param.mask->answer) {
	if (Rast3d_mask_file_exists())
	    if (Rast3d_mask_is_on(map) && changemask)
		Rast3d_mask_off(map);
    }

    /* Close files and exit */
    if (!Rast3d_close(map))
	fatalError(_("Unable to close 3D raster map"));

    /* Close the netCDF file */
    if ((retval = nc_close(ncid)))
	ERR(retval);

    return 0;
}
