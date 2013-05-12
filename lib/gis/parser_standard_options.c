/*!
  \file lib/gis/parser_standard_options.c
  
  \brief GIS Library - Argument parsing functions (standard options)
  
  (C) 2001-2013 by the GRASS Development Team
  
  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.
  
  \author Original author CERL
  \author Soeren Gebbert added Dec. 2009 WPS process_description document
  \author Luca Delucchi added Aug 2011 G_OPT_M_DIR
*/

#include <grass/gis.h>
#include <grass/glocale.h>

#include "parser_local_proto.h"

/*!
  \brief Create standardised Option structure.
  
  This function will create a standardised Option structure defined by
  parameter <i>opt</i>. A list of valid parameters bellow. It
  allocates memory for the Option structure and returns a pointer to
  this memory.
  
  If an invalid parameter was specified a empty Option structure will
  be returned (not NULL).
  
  - database:
   - G_OPT_DB_SQL
   - G_OPT_DB_WHERE
   - G_OPT_DB_TABLE
   - G_OPT_DB_DRIVER
   - G_OPT_DB_DATABASE
   - G_OPT_DB_SCHEMA
   - G_OPT_DB_COLUMN
   - G_OPT_DB_COLUMNS
   - G_OPT_DB_KEYCOLUMN

  - imagery:
   - G_OPT_I_GROUP
   - G_OPT_I_SUBGROUP

  - raster:
   - G_OPT_R_INPUT
   - G_OPT_R_INPUTS
   - G_OPT_R_OUTPUT
   - G_OPT_R_MAP
   - G_OPT_R_MAPS
   - G_OPT_R_BASE
   - G_OPT_R_COVER
   - G_OPT_R_ELEV
   - G_OPT_R_ELEVS
   - G_OPT_R_INTERP_TYPE

  - raster3d:
   - G_OPT_R3_INPUT
   - G_OPT_R3_INPUTS
   - G_OPT_R3_OUTPUT
   - G_OPT_R3_MAP
   - G_OPT_R3_MAPS
   
  - vector:
   - G_OPT_V_INPUT
   - G_OPT_V_INPUTS
   - G_OPT_V_OUTPUT
   - G_OPT_V_MAP
   - G_OPT_V_MAPS
   - G_OPT_V_TYPE
   - G_OPT_V_FIELD
   - G_OPT_V_FIELD_ALL
   - G_OPT_V_CAT
   - G_OPT_V_CATS
   - G_OPT_V_ID
   - G_OPT_V_IDS
   
  - files
   - G_OPT_F_INPUT
   - G_OPT_F_OUTPUT
   - G_OPT_F_SEP
   
  - colors
   - G_OPT_C_FG
   - G_OPT_C_BG

  - misc
   - G_OPT_M_UNITS
   - G_OPT_M_DATATYPE
   - G_OPT_M_MAPSET
   - G_OPT_M_COORDS
   - G_OPT_M_COLR
   - G_OPT_M_DIR

  - temporal GIS framework
   - G_OPT_STDS_INPUT
   - G_OPT_STDS_INPUTS
   - G_OPT_STDS_OUTPUT
   - G_OPT_STRDS_INPUT
   - G_OPT_STRDS_INPUTS
   - G_OPT_STRDS_OUTPUT
   - G_OPT_STR3DS_INPUT
   - G_OPT_STR3DS_INPUTS
   - G_OPT_STR3DS_OUTPUT
   - G_OPT_STVDS_INPUT
   - G_OPT_STVDS_INPUTS
   - G_OPT_STVDS_OUTPUT
   - G_OPT_MAP_INPUT
   - G_OPT_MAP_INPUTS
   - G_OPT_STDS_TYPE
   - G_OPT_MAP_TYPE
   - G_OPT_T_TYPE
   - G_OPT_T_WHERE
 
   \param opt type of Option struct to create
   
   \return pointer to an Option struct
*/
struct Option *G_define_standard_option(int opt)
{
    struct Option *Opt;

    Opt = G_define_option();

    switch (opt) {
    case G_OPT_DB_SQL:
        Opt->key = "sql";
        Opt->type = TYPE_STRING;
        Opt->key_desc = "sql_query";
        Opt->required = NO;
        Opt->label = _("SQL select statement");
        Opt->description =
            _("For example: 'select * from rybniky where kapri = 'hodne'");
        break;
    case G_OPT_DB_WHERE:
	Opt->key = "where";
	Opt->type = TYPE_STRING;
	Opt->key_desc = "sql_query";
	Opt->required = NO;
	Opt->label = _("WHERE conditions of SQL statement without 'where' keyword");
	Opt->description = _("Example: income < 1000 and inhab >= 10000");
	break;
    case G_OPT_DB_TABLE:
	Opt->key = "table";
	Opt->type = TYPE_STRING;
	Opt->key_desc = "name";
	Opt->required = NO;
	Opt->multiple = NO;
	Opt->description = _("Name of attribute table");
	Opt->gisprompt = "old,dbtable,dbtable";
	break;
    case G_OPT_DB_DRIVER:
	Opt->key = "driver";
	Opt->type = TYPE_STRING;
	Opt->key_desc = "name";
	Opt->required = NO;
	Opt->multiple = NO;
	Opt->description = _("Name of database driver");
	Opt->gisprompt = "old,dbdriver,dbdriver";
	break;
    case G_OPT_DB_DATABASE:
	Opt->key = "database";
	Opt->type = TYPE_STRING;
	Opt->key_desc = "name";
	Opt->required = NO;
	Opt->multiple = NO;
	Opt->description = _("Name of database");
	Opt->gisprompt = "old,dbname,dbname";
	break;
    case G_OPT_DB_SCHEMA:
	Opt->key = "schema";
	Opt->type = TYPE_STRING;
	Opt->key_desc = "name";
	Opt->required = NO;
	Opt->multiple = NO;
	Opt->label = _("Database schema");
	Opt->description = _("Do not use this option if schemas "
			     "are not supported by driver/database server");
	break;
    case G_OPT_DB_COLUMN:
	Opt->key = "column";
	Opt->type = TYPE_STRING;
	Opt->key_desc = "name";
	Opt->required = NO;
	Opt->multiple = NO;
	Opt->description = _("Name of attribute column");
	Opt->gisprompt = "old,dbcolumn,dbcolumn";
	break;
    case G_OPT_DB_COLUMNS:
	Opt->key = "columns";
	Opt->type = TYPE_STRING;
	Opt->key_desc = "name";
	Opt->required = NO;
	Opt->multiple = YES;
	Opt->description = _("Name of attribute column(s)");
	Opt->gisprompt = "old,dbcolumn,dbcolumn";
	break;
    case G_OPT_DB_KEYCOLUMN:
	Opt->key = "key";
	Opt->type = TYPE_STRING;
	Opt->key_desc = "name";
	Opt->required = NO;
	Opt->multiple = NO;
	Opt->label = _("Name of key column");
	Opt->description = _("Must refer to an integer column");
	/* Opt->gisprompt = "old,dbcolumn,dbcolumn"; */
	Opt->answer = GV_KEY_COLUMN;
	break;

	/* imagery group */
    case G_OPT_I_GROUP:
	Opt->key = "group";
	Opt->type = TYPE_STRING;
	Opt->key_desc = "name";
	Opt->required = YES;
	Opt->gisprompt = "old,group,group";
	Opt->description = _("Name of input imagery group");
	break;
    case G_OPT_I_SUBGROUP:
	Opt->key = "subgroup";
	Opt->type = TYPE_STRING;
	Opt->key_desc = "name";
	Opt->required = YES;
	Opt->gisprompt = "old,subgroup,subgroup";
	Opt->description = _("Name of input imagery subgroup");
	break;

	/* raster maps */
    case G_OPT_R_INPUT:
	Opt->key = "input";
	Opt->type = TYPE_STRING;
	Opt->key_desc = "name";
	Opt->required = YES;
	Opt->gisprompt = "old,cell,raster";
	Opt->description = _("Name of input raster map");
	break;
    case G_OPT_R_INPUTS:
	Opt->key = "input";
	Opt->type = TYPE_STRING;
	Opt->key_desc = "name";
	Opt->required = YES;
	Opt->multiple = YES;
	Opt->gisprompt = "old,cell,raster";
	Opt->description = _("Name of input raster map(s)");
	break;
    case G_OPT_R_OUTPUT:
	Opt->key = "output";
	Opt->type = TYPE_STRING;
	Opt->key_desc = "name";
	Opt->required = YES;
	Opt->gisprompt = "new,cell,raster";
	Opt->description = _("Name for output raster map");
	break;
    case G_OPT_R_MAP:
	Opt->key = "map";
	Opt->type = TYPE_STRING;
	Opt->key_desc = "name";
	Opt->required = YES;
	Opt->gisprompt = "old,cell,raster";
	Opt->description = _("Name of raster map");
	break;
    case G_OPT_R_MAPS:
	Opt->key = "map";
	Opt->type = TYPE_STRING;
	Opt->key_desc = "name";
	Opt->required = YES;
	Opt->multiple = YES;
	Opt->gisprompt = "old,cell,raster";
	Opt->description = _("Name of raster map(s)");
	break;
    case G_OPT_R_BASE:
	Opt->key = "base";
	Opt->type = TYPE_STRING;
	Opt->key_desc = "name";
	Opt->required = YES;
	Opt->gisprompt = "old,cell,raster";
	Opt->description = _("Name of base raster map");
	break;
    case G_OPT_R_COVER:
	Opt->key = "cover";
	Opt->type = TYPE_STRING;
	Opt->key_desc = "name";
	Opt->required = YES;
	Opt->gisprompt = "old,cell,raster";
	Opt->description = _("Name of cover raster map");
	break;
    case G_OPT_R_ELEV:
	Opt->key = "elevation";
	Opt->type = TYPE_STRING;
	Opt->key_desc = "name";
	Opt->required = YES;
	Opt->gisprompt = "old,cell,raster";
	Opt->description = _("Name of input elevation raster map");
	break;
    case G_OPT_R_ELEVS:
	Opt->key = "elevation";
	Opt->type = TYPE_STRING;
	Opt->key_desc = "name";
	Opt->required = YES;
	Opt->multiple = YES;
	Opt->gisprompt = "old,cell,raster";
	Opt->description = _("Name of input elevation raster map(s)");
	break;
    case G_OPT_R_INTERP_TYPE:
        Opt->key = "method";
        Opt->type = TYPE_STRING;
        Opt->required = NO;
        Opt->description = _("Sampling interpolation method");
        Opt->options = "nearest,bilinear,bicubic";
        G_asprintf((char **) &(Opt->descriptions),
                   "nearest;%s;bilinear;%s;bicubic;%s",
                   _("Nearest-neighbor interpolation"),
                   _("Bilinear interpolation"),
                   _("Bicubic interpolation"));
        break;

	/*g3d maps */
    case G_OPT_R3_INPUT:
	Opt->key = "input";
	Opt->type = TYPE_STRING;
	Opt->key_desc = "name";
	Opt->required = YES;
	Opt->gisprompt = "old,grid3,3d-raster";
	Opt->description = _("Name of input 3D raster map");
	break;
    case G_OPT_R3_INPUTS:
	Opt->key = "input";
	Opt->type = TYPE_STRING;
	Opt->key_desc = "name";
	Opt->required = YES;
	Opt->multiple = YES;
	Opt->gisprompt = "old,grid3,3d-raster";
	Opt->description = _("Name of input 3D raster map(s)");
	break;
    case G_OPT_R3_OUTPUT:
	Opt->key = "output";
	Opt->type = TYPE_STRING;
	Opt->key_desc = "name";
	Opt->required = YES;
	Opt->gisprompt = "new,grid3,3d-raster";
	Opt->description = _("Name for output 3D raster map");
	break;
    case G_OPT_R3_MAP:
	Opt->key = "map";
	Opt->type = TYPE_STRING;
	Opt->key_desc = "name";
	Opt->required = YES;
	Opt->gisprompt = "old,grid3,3d-raster";
	Opt->description = _("Name of 3D raster map");
	break;
    case G_OPT_R3_MAPS:
	Opt->key = "map";
	Opt->type = TYPE_STRING;
	Opt->key_desc = "name";
	Opt->required = YES;
	Opt->multiple = YES;
	Opt->gisprompt = "old,grid3,3d-raster";
	Opt->description = _("Name of 3D raster map(s)");
	break;
    case G_OPT_R3_TYPE:
        Opt->key = "type";
        Opt->type = TYPE_STRING;
        Opt->required = NO;
        Opt->multiple = NO;
        Opt->answer = "default";
        Opt->options = "default,double,float";
        Opt->description = _("Data type used in the output raster3d map");
	break;
    case G_OPT_R3_PRECISION:
        Opt->key = "precision";
        Opt->type = TYPE_STRING;
        Opt->required = NO;
        Opt->multiple = NO;
        Opt->answer = "default";
        Opt->description =
            _("Number of digits used as mantissa in the internal map storage, 0 -23 for float, 0 - 52 for double, max or default");
	break;
    case G_OPT_R3_COMPRESSION:
        Opt->key = "compression";
        Opt->type = TYPE_STRING;
        Opt->required = NO;
        Opt->multiple = NO;
        Opt->answer = "default";
        Opt->options = "default,zip,none";
        Opt->description =
            _("The compression method used in the output raster3d map");
	break;
    case G_OPT_R3_TILE_DIMENSION:
        Opt->key = "tiledimension";
        Opt->type = TYPE_STRING;
        Opt->required = NO;
        Opt->multiple = NO;
        Opt->key_desc = "XxYxZ";
        Opt->answer = "default";
        Opt->description =
            _("The dimensions of the tiles used in the output raster3d map (XxYxZ or default: 16x16x8)");
	break;

	/*vector maps */
    case G_OPT_V_INPUT:
	Opt->key = "input";
	Opt->type = TYPE_STRING;
	Opt->key_desc = "name";
	Opt->required = YES;
	Opt->gisprompt = "old,vector,vector";
	Opt->label = _("Name of input vector map");
	Opt->description = _("Or data source for direct OGR access");
	break;
    case G_OPT_V_INPUTS:
	Opt->key = "input";
	Opt->type = TYPE_STRING;
	Opt->key_desc = "name";
	Opt->required = YES;
	Opt->multiple = YES;
	Opt->gisprompt = "old,vector,vector";
	Opt->label = _("Name of input vector map(s)");
	Opt->description = _("Or data source(s) for direct OGR access");
	break;
    case G_OPT_V_OUTPUT:
	Opt->key = "output";
	Opt->type = TYPE_STRING;
	Opt->key_desc = "name";
	Opt->required = YES;
	Opt->gisprompt = "new,vector,vector";
	Opt->description = _("Name for output vector map");
	break;
    case G_OPT_V_MAP:
	Opt->key = "map";
	Opt->type = TYPE_STRING;
	Opt->key_desc = "name";
	Opt->required = YES;
	Opt->gisprompt = "old,vector,vector";
	Opt->label = _("Name of vector map");
	Opt->description = _("Or data source for direct OGR access");
	break;
    case G_OPT_V_MAPS:
	Opt->key = "map";
	Opt->type = TYPE_STRING;
	Opt->key_desc = "name";
	Opt->required = YES;
	Opt->multiple = YES;
	Opt->gisprompt = "old,vector,vector";
	Opt->description = _("Name of vector map(s)");
	break;
    case G_OPT_V_TYPE:
	Opt->key = "type";
	Opt->type = TYPE_STRING;
	Opt->required = NO;
	Opt->multiple = YES;
	Opt->answer = "point,line,boundary,centroid,area";
	Opt->options = "point,line,boundary,centroid,area";
	Opt->description = _("Feature type");
	break;
    case G_OPT_V3_TYPE:
	Opt->key = "type";
	Opt->type = TYPE_STRING;
	Opt->required = NO;
	Opt->multiple = YES;
	Opt->answer = "point,line,boundary,centroid,area,face,kernel";
	Opt->options = "point,line,boundary,centroid,area,face,kernel";
	Opt->description = _("Feature type");
	break;
    case G_OPT_V_FIELD:
	Opt->key = "layer";
	Opt->type = TYPE_STRING;
	Opt->required = NO;
	Opt->answer = "1";
	Opt->label = _("Layer number or name");
	Opt->description =
	    _("Vector features can have category values in different layers."
	      " This number determines which layer to use. "
	      "When used with direct OGR access this is the layer name.");
	Opt->gisprompt = "old,layer,layer";
	break;
    case G_OPT_V_FIELD_ALL:
	Opt->key = "layer";
	Opt->type = TYPE_STRING;
	Opt->required = NO;
	Opt->answer = "-1";
	Opt->label = _("Layer number or name ('-1' for all layers)");
	Opt->description =
	    _("A single vector map can be connected to multiple database "
	      "tables. This number determines which table to use. "
	      "When used with direct OGR access this is the layer name.");
	Opt->gisprompt = "old,layer_all,layer";
	break;
    case G_OPT_V_CAT:
	Opt->key = "cat";
	Opt->type = TYPE_INTEGER;
	Opt->required = NO;
	Opt->description = _("Category value");
	break;
    case G_OPT_V_CATS:
	Opt->key = "cats";
	Opt->type = TYPE_STRING;
	Opt->key_desc = "range";
	Opt->required = NO;
	Opt->label = _("Category values");
	Opt->description = _("Example: 1,3,7-9,13");
	break;
    case G_OPT_V_ID:
	Opt->key = "id";
	Opt->type = TYPE_INTEGER;
	Opt->required = NO;
	Opt->description = _("Feature id");
	break;
    case G_OPT_V_IDS:
	Opt->key = "ids";
	Opt->type = TYPE_STRING;
	Opt->key_desc = "range";
	Opt->required = NO;
	Opt->label = _("Feature ids");
	Opt->description = _("Example: 1,3,7-9,13");
	break;

	/* files */
    case G_OPT_F_INPUT:
	Opt->key = "input";
	Opt->type = TYPE_STRING;
	Opt->key_desc = "name";
	Opt->required = YES;
	Opt->gisprompt = "old,file,file";
	Opt->description = _("Name of input file");
	break;
    case G_OPT_F_OUTPUT:
	Opt->key = "output";
	Opt->type = TYPE_STRING;
	Opt->key_desc = "name";
	Opt->required = YES;
	Opt->gisprompt = "new,file,file";
	Opt->description = _("Name for output file");
	break;
    case G_OPT_F_SEP:
	Opt->key = "separator";
	Opt->type = TYPE_STRING;
	Opt->key_desc = "character";
	Opt->required = NO;
	Opt->answer = "|";
	Opt->label = _("Field separator");
	Opt->description = _("Special characters: newline, space, comma, tab");
	break;

	/* colors */
    case G_OPT_C_FG:
	Opt->key = "color";
	Opt->type = TYPE_STRING;
	Opt->key_desc = "name";
	Opt->required = NO;
	Opt->answer = DEFAULT_FG_COLOR;
	Opt->gisprompt = "old,color,color";
	Opt->label = _("Color");
	Opt->description = _("Either a standard color name or R:G:B triplet");
	break;
    case G_OPT_C_BG:
	Opt->key = "bgcolor";
	Opt->type = TYPE_STRING;
	Opt->key_desc = "name";
	Opt->required = NO;
	Opt->answer = DEFAULT_BG_COLOR;
	Opt->gisprompt = "old,color_none,color";
	Opt->label = _("Background color");
	Opt->description =
	    _("Either a standard GRASS color, R:G:B triplet, or \"none\"");
	break;

	/* misc */

    case G_OPT_M_DIR:
        Opt->key = "input";
        Opt->type = TYPE_STRING;
        Opt->key_desc = "name";
        Opt->required = YES;
        Opt->gisprompt = "old,dir,dir";
        Opt->description = _("Name to input directory");
        break;

    case G_OPT_M_UNITS:
	Opt->key = "units";
	Opt->type = TYPE_STRING;
	Opt->required = NO;
	Opt->multiple = NO;
	Opt->options =
	    "miles,feet,meters,kilometers,acres,hectares";
	Opt->description = _("Units");
	break;
	
    case G_OPT_M_DATATYPE:
	Opt->key = "type";
	Opt->key_desc = "datatype";
	Opt->type = TYPE_STRING;
	Opt->required = YES;
	Opt->multiple = YES;
	Opt->description = _("Data type(s)");
	break;

    case G_OPT_M_MAPSET:
	Opt->key = "mapset";
	Opt->type = TYPE_STRING;
	Opt->required = NO;
	Opt->multiple = NO;
	Opt->key_desc = "name";
	Opt->gisprompt = "old,mapset,mapset";
	Opt->label = _("Name of mapset (default: current search path)");
	Opt->description = _("'.' for current mapset");
	break;

    case G_OPT_M_COORDS:
	Opt->key = "coordinates";
	Opt->type = TYPE_DOUBLE;
	Opt->required = NO;
	Opt->multiple = NO;
	Opt->key_desc = "east,north";
	Opt->gisprompt = "old,coords,coords";
	Opt->description = _("Coordinates");
	break;

    case G_OPT_M_COLR:
	Opt->key = "color";
	Opt->key_desc = "style";
	Opt->type = TYPE_STRING;
	Opt->required = NO;
	Opt->options = G_color_rules_options();
	Opt->description = _("Name of color table");
	Opt->descriptions = G_color_rules_descriptions();
	break;

    /* Spatio-temporal modules of the temporal GIS framework */
    case G_OPT_STDS_INPUT:
	Opt->key = "input";
	Opt->type = TYPE_STRING;
	Opt->key_desc = "name";
	Opt->required = YES;
	Opt->gisprompt = "old,stds,stds";
	Opt->description = _("Name of the input space time dataset");
	break;
    case G_OPT_STDS_INPUTS:
	Opt->key = "inputs";
	Opt->type = TYPE_STRING;
	Opt->key_desc = "name";
	Opt->required = YES;
	Opt->multiple = YES;
	Opt->gisprompt = "old,stds,stds";
	Opt->description = _("Name of the input space time datasets");
	break;
    case G_OPT_STDS_OUTPUT:
	Opt->key = "output";
	Opt->type = TYPE_STRING;
	Opt->key_desc = "name";
	Opt->required = YES;
	Opt->gisprompt = "new,stds,stds";
	Opt->description = _("Name of the output space time dataset");
	break;
    case G_OPT_STRDS_INPUT:
	Opt->key = "input";
	Opt->type = TYPE_STRING;
	Opt->key_desc = "name";
	Opt->required = YES;
	Opt->gisprompt = "old,strds,strds";
	Opt->description = _("Name of the input space time raster dataset");
	break;
    case G_OPT_STRDS_INPUTS:
	Opt->key = "inputs";
	Opt->type = TYPE_STRING;
	Opt->key_desc = "name";
	Opt->required = YES;
	Opt->multiple = YES;
	Opt->gisprompt = "old,strds,strds";
	Opt->description = _("Name of the input space time raster datasets");
	break;
    case G_OPT_STRDS_OUTPUT:
	Opt->key = "output";
	Opt->type = TYPE_STRING;
	Opt->key_desc = "name";
	Opt->required = YES;
	Opt->gisprompt = "new,strds,strds";
	Opt->description = _("Name of the output space time raster dataset");
	break;
    case G_OPT_STVDS_INPUT:
	Opt->key = "input";
	Opt->type = TYPE_STRING;
	Opt->key_desc = "name";
	Opt->required = YES;
	Opt->gisprompt = "old,stvds,stvds";
	Opt->description = _("Name of the input space time vector dataset");
	break;
    case G_OPT_STVDS_INPUTS:
	Opt->key = "inputs";
	Opt->type = TYPE_STRING;
	Opt->key_desc = "name";
	Opt->required = YES;
	Opt->multiple = YES;
	Opt->gisprompt = "old,stvds,stvds";
	Opt->description = _("Name of the input space time vector datasets");
	break;
    case G_OPT_STVDS_OUTPUT:
	Opt->key = "output";
	Opt->type = TYPE_STRING;
	Opt->key_desc = "name";
	Opt->required = YES;
	Opt->gisprompt = "new,stvds,stvds";
	Opt->description = _("Name of the output space time vector dataset");
	break;
    case G_OPT_STR3DS_INPUT:
	Opt->key = "input";
	Opt->type = TYPE_STRING;
	Opt->key_desc = "name";
	Opt->required = YES;
	Opt->gisprompt = "old,str3ds,str3ds";
	Opt->description = _("Name of the input space time raster3d dataset");
	break;
    case G_OPT_STR3DS_INPUTS:
	Opt->key = "inputs";
	Opt->type = TYPE_STRING;
	Opt->key_desc = "name";
	Opt->required = YES;
	Opt->multiple = YES;
	Opt->gisprompt = "old,str3ds,str3ds";
	Opt->description = _("Name of the input space time raster3d datasets");
	break;
    case G_OPT_STR3DS_OUTPUT:
	Opt->key = "output";
	Opt->type = TYPE_STRING;
	Opt->key_desc = "name";
	Opt->required = YES;
	Opt->gisprompt = "new,str3ds,str3ds";
	Opt->description = _("Name of the output space time raster3d dataset");
	break;
    case G_OPT_STDS_TYPE:
	Opt->key = "type";
	Opt->type = TYPE_STRING;
	Opt->key_desc = "name";
	Opt->required = NO;
	Opt->answer = "strds";
	Opt->options = "strds,stvds,str3ds";
	Opt->description = _("Type of the input space time dataset");
	break;
    case G_OPT_MAP_INPUT:
	Opt->key = "map";
	Opt->type = TYPE_STRING;
	Opt->key_desc = "name";
	Opt->required = YES;
	Opt->gisprompt = "old,map,map";
	Opt->description = _("Name of the input map");
	break;
    case G_OPT_MAP_INPUTS:
	Opt->key = "maps";
	Opt->type = TYPE_STRING;
	Opt->key_desc = "name";
	Opt->required = YES;
	Opt->multiple = YES;
	Opt->gisprompt = "old,map,map";
	Opt->description = _("Name of the input maps");
	break;
    case G_OPT_MAP_TYPE:
	Opt->key = "type";
	Opt->type = TYPE_STRING;
	Opt->key_desc = "name";
	Opt->required = NO;
	Opt->answer = "rast";
	Opt->options = "rast,vect,rast3d";
	Opt->description = _("Type of the input map");
	break;
    case G_OPT_T_TYPE:
	Opt->key = "temporaltype";
	Opt->type = TYPE_STRING;
	Opt->key_desc = "name";
	Opt->required = NO;
	Opt->answer = "absolute";
	Opt->options = "absolute,relative";
	Opt->description = _("The temporal type of the space time dataset");
	break;
    case G_OPT_T_WHERE:
	Opt->key = "where";
	Opt->type = TYPE_STRING;
	Opt->key_desc = "sql_query";
	Opt->required = NO;
	Opt->label = _("WHERE conditions of SQL statement without 'where' keyword used in the temporal GIS framework");
	Opt->description = _("Example: start_time > '2001-01-01 12:30:00'");
	break;
    case G_OPT_T_SAMPLE:
	Opt->key = "sampling";
	Opt->type = TYPE_STRING;
	Opt->key_desc = "name";
	Opt->required = NO;
	Opt->multiple = YES;
	Opt->answer = "start";
	Opt->options = "start,during,overlap,contain,equal,follows,precedes";
	Opt->description = _("The method to be used for sampling the input dataset");
	break;
    }
    
    return Opt;
}

/*!
  \brief Create standardised Flag structure.
  
  This function will create a standardised Flag structure defined by
  parameter <i>flag</i>. A list of valid parameters bellow. It
  allocates memory for the Flag structure and returns a pointer to
  this memory.
  
  If an invalid parameter was specified a empty Flag structure will be
  returned (not NULL).

  - G_FLG_V_TABLE  (do not create attribute table)
  - G_FLG_V_TOPO   (do not build topology)

  \param flag type of Flag struct to create
  
  \return pointer to an Flag struct
*/
struct Flag *G_define_standard_flag(int flag)
{
    struct Flag *Flg;

    Flg = G_define_flag();

    switch (flag) {
    case G_FLG_V_TABLE:
	Flg->key = 't';
	Flg->description = _("Do not create attribute table");
	break;
    case G_FLG_V_TOPO:
	Flg->key = 'b';
	Flg->description = _("Do not build topology");
	break;
    }
    
    return Flg;
}
