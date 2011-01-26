/*!
 * \file gis/parser_standard_options.c
 *
 * \brief GIS Library - Argument parsing functions (standard options)
 *
 * (C) 2001-2010 by the GRASS Development Team
 *
 * This program is free software under the GNU General Public License
 * (>=v2). Read the file COPYING that comes with GRASS for details.
 *
 * \author Original author CERL
 * \author Soeren Gebbert added Dec. 2009 WPS process_description document
 */

#include <grass/gis.h>
#include <grass/glocale.h>

#include "parser_local_proto.h"

/*!
 * \brief Create standardised Option structure.
 *
 * This function will create a standardised Option structure defined
 * by parameter opt. A list of valid parameters can be found in gis.h.
 * It allocates memory for the Option structure and returns a pointer
 * to this memory.
 *
 * If an invalid parameter was specified a empty Option structure will
 * be returned (not NULL).
 *
 *  - database:
 *   - G_OPT_DB_WHERE
 *   - G_OPT_DB_COLUMN
 *   - G_OPT_DB_COLUMNS
 *   - G_OPT_DB_TABLE
 *   - G_OPT_DB_DRIVER
 *   - G_OPT_DB_DATABASE
 *   - G_OPT_DB_SCHEMA
 *
 *  - imagery:
 *   - G_OPT_I_GROUP
 *   - G_OPT_I_SUBGROUP
 *
 *  - raster:
 *   - G_OPT_R_INPUT
 *   - G_OPT_R_INPUTS
 *   - G_OPT_R_OUTPUT
 *   - G_OPT_R_MAP
 *   - G_OPT_R_MAPS
 *   - G_OPT_R_BASE
 *   - G_OPT_R_COVER
 *   - G_OPT_R_ELEV
 *   - G_OPT_R_ELEVS
 *
 *  - raster3d:
 *   - G_OPT_R3_INPUT
 *   - G_OPT_R3_INPUTS
 *   - G_OPT_R3_OUTPUT
 *   - G_OPT_R3_MAP
 *   - G_OPT_R3_MAPS
 *
 *  - vector:
 *   - G_OPT_V_INPUT
 *   - G_OPT_V_INPUTS
 *   - G_OPT_V_OUTPUT
 *   - G_OPT_V_MAP
 *   - G_OPT_V_MAPS
 *   - G_OPT_V_TYPE
 *   - G_OPT_V_FIELD
 *   - G_OPT_V_FIELD_ALL
 *   - G_OPT_V_CAT
 *   - G_OPT_V_CATS
 *   - G_OPT_V_ID
 *   - G_OPT_V_IDS
 * 
 *  - files
 *   - G_OPT_F_INPUT
 *   - G_OPT_F_OUTPUT
 *   - G_OPT_F_SEP
 *
 *  - colors
 *   - G_OPT_C_FG
 *   - G_OPT_C_BG
 *
 *  - misc
 *
 *   - G_OPT_M_UNITS
 *   - G_OPT_M_DATATYPE
 *   - G_OPT_M_MAPSET
 *
 * \param opt type of Option struct to create
 *
 * \return pointer to an Option struct
 */

struct Option *G_define_standard_option(int opt)
{
    struct Option *Opt;

    Opt = G_define_option();

    switch (opt) {
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
	Opt->description = _("Table name");
	Opt->gisprompt = "old,dbtable,dbtable";
	break;
    case G_OPT_DB_DRIVER:
	Opt->key = "driver";
	Opt->type = TYPE_STRING;
	Opt->key_desc = "name";
	Opt->required = NO;
	Opt->multiple = NO;
	Opt->description = _("Driver name");
	Opt->gisprompt = "old,dbdriver,dbdriver";
	break;
    case G_OPT_DB_DATABASE:
	Opt->key = "database";
	Opt->type = TYPE_STRING;
	Opt->key_desc = "name";
	Opt->required = NO;
	Opt->multiple = NO;
	Opt->description = _("Database name");
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

	/*vector maps */
    case G_OPT_V_INPUT:
	Opt->key = "input";
	Opt->type = TYPE_STRING;
	Opt->key_desc = "name";
	Opt->required = YES;
	Opt->gisprompt = "old,vector,vector";
	Opt->label = _("Name of input vector map");
	Opt->description = _("Data source for direct OGR access");
	break;
    case G_OPT_V_INPUTS:
	Opt->key = "input";
	Opt->type = TYPE_STRING;
	Opt->key_desc = "name";
	Opt->required = YES;
	Opt->multiple = YES;
	Opt->gisprompt = "old,vector,vector";
	Opt->label = _("Name of input vector map(s)");
	Opt->description = _("Data source(s) for direct OGR access");
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
	Opt->description = _("Data source for direct OGR access");
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
	Opt->required = YES;
	Opt->answer = "1";
	Opt->label = _("Layer number or name");
	Opt->description =
	    _("A single vector map can be connected to multiple database "
	      "tables. This number determines which table to use. "
	      "Layer name for direct OGR access.");
	Opt->gisprompt = "old,layer,layer";
	break;
    case G_OPT_V_FIELD_ALL:
	Opt->key = "layer";
	Opt->type = TYPE_STRING;
	Opt->required = YES;
	Opt->answer = "-1";
	Opt->label = _("Layer number or name ('-1' for all layers)");
	Opt->description =
	    _("A single vector map can be connected to multiple database "
	      "tables. This number determines which table to use. "
	      "Layer name for direct OGR access.");
	Opt->gisprompt = "old,layer,layer_all";
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
	Opt->gisprompt = "old,file,input";
	Opt->description = _("Name to input file");
	break;
    case G_OPT_F_OUTPUT:
	Opt->key = "output";
	Opt->type = TYPE_STRING;
	Opt->key_desc = "name";
	Opt->required = YES;
	Opt->gisprompt = "new,file,output";
	Opt->description = _("Name for output file");
	break;
    case G_OPT_F_SEP:
	Opt->key = "fs";
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
	Opt->gisprompt = "old,color,color_none";
	Opt->label = _("Background color");
	Opt->description =
	    _("Either a standard GRASS color, R:G:B triplet, or \"none\"");
	break;

	/* misc */
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
    }

    return (Opt);
}
