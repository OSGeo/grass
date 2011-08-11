#include "proto.h"

struct Option *define_standard_option(const char *name)
{
    int key;
    struct Option *opt;

    key = G_OPT_UNDEFINED;
    if (G_strcasecmp(name, "G_OPT_DB_WHERE") == 0)
	key = G_OPT_DB_WHERE;
    else if (G_strcasecmp(name, "G_OPT_DB_TABLE") == 0)
	key = G_OPT_DB_TABLE;
    else if (G_strcasecmp(name, "G_OPT_DB_DRIVER") == 0)
	key = G_OPT_DB_DRIVER;
    else if (G_strcasecmp(name, "G_OPT_DB_DATABASE") == 0)
	key = G_OPT_DB_DATABASE;
    else if (G_strcasecmp(name, "G_OPT_DB_SCHEMA") == 0)
	key = G_OPT_DB_SCHEMA;
    else if (G_strcasecmp(name, "G_OPT_DB_COLUMN") == 0)
	key = G_OPT_DB_COLUMN;
    else if (G_strcasecmp(name, "G_OPT_DB_COLUMNS") == 0)
	key = G_OPT_DB_COLUMNS;
    else if (G_strcasecmp(name, "G_OPT_DB_KEYCOLUMN") == 0)
	key = G_OPT_DB_KEYCOLUMN;
    else if (G_strcasecmp(name, "G_OPT_I_GROUP") == 0)
	key = G_OPT_I_GROUP;
    else if (G_strcasecmp(name, "G_OPT_I_SUBGROUP") == 0)
	key = G_OPT_I_SUBGROUP;
    else if (G_strcasecmp(name, "G_OPT_R_INPUT") == 0)
	key = G_OPT_R_INPUT;
    else if (G_strcasecmp(name, "G_OPT_R_INPUTS") == 0)
	key = G_OPT_R_INPUTS;
    else if (G_strcasecmp(name, "G_OPT_R_OUTPUT") == 0)
	key = G_OPT_R_OUTPUT;
    else if (G_strcasecmp(name, "G_OPT_R_MAP") == 0)
	key = G_OPT_R_MAP;
    else if (G_strcasecmp(name, "G_OPT_R_MAPS") == 0)
	key = G_OPT_R_MAPS;
    else if (G_strcasecmp(name, "G_OPT_R_BASE") == 0)
	key = G_OPT_R_BASE;
    else if (G_strcasecmp(name, "G_OPT_R_COVER") == 0)
	key = G_OPT_R_COVER;
    else if (G_strcasecmp(name, "G_OPT_R_ELEV") == 0)
	key = G_OPT_R_ELEV;
    else if (G_strcasecmp(name, "G_OPT_R_ELEVS") == 0)
	key = G_OPT_R_ELEVS;
    else if (G_strcasecmp(name, "G_OPT_R3_INPUT") == 0)
	key = G_OPT_R3_INPUT;
    else if (G_strcasecmp(name, "G_OPT_R3_INPUTS") == 0)
	key = G_OPT_R3_INPUTS;
    else if (G_strcasecmp(name, "G_OPT_R3_OUTPUT") == 0)
	key = G_OPT_R3_OUTPUT;
    else if (G_strcasecmp(name, "G_OPT_R3_MAP") == 0)
	key = G_OPT_R3_MAP;
    else if (G_strcasecmp(name, "G_OPT_R3_MAPS") == 0)
	key = G_OPT_R3_MAPS;
    else if (G_strcasecmp(name, "G_OPT_R3_TYPE") == 0)
	key = G_OPT_R3_TYPE;
    else if (G_strcasecmp(name, "G_OPT_R3_PRECISION") == 0)
	key = G_OPT_R3_PRECISION;
    else if (G_strcasecmp(name, "G_OPT_R3_COMPRESSION") == 0)
	key = G_OPT_R3_COMPRESSION;
    else if (G_strcasecmp(name, "G_OPT_R3_TILE_DIMENSION") == 0)
	key = G_OPT_R3_TILE_DIMENSION;
    else if (G_strcasecmp(name, "G_OPT_V_INPUT") == 0)
	key = G_OPT_V_INPUT;
    else if (G_strcasecmp(name, "G_OPT_V_INPUTS") == 0)
	key = G_OPT_V_INPUTS;
    else if (G_strcasecmp(name, "G_OPT_V_OUTPUT") == 0)
	key = G_OPT_V_OUTPUT;
    else if (G_strcasecmp(name, "G_OPT_V_MAP") == 0)
	key = G_OPT_V_MAP;
    else if (G_strcasecmp(name, "G_OPT_V_MAPS") == 0)
	key = G_OPT_V_MAPS;
    else if (G_strcasecmp(name, "G_OPT_V_TYPE") == 0)
	key = G_OPT_V_TYPE;
    else if (G_strcasecmp(name, "G_OPT_V3_TYPE") == 0)
	key = G_OPT_V3_TYPE;
    else if (G_strcasecmp(name, "G_OPT_V_FIELD") == 0)
	key = G_OPT_V_FIELD;
    else if (G_strcasecmp(name, "G_OPT_V_FIELD_ALL") == 0)
	key = G_OPT_V_FIELD_ALL;
    else if (G_strcasecmp(name, "G_OPT_V_CAT") == 0)
	key = G_OPT_V_CAT;
    else if (G_strcasecmp(name, "G_OPT_V_CATS") == 0)
	key = G_OPT_V_CATS;
    else if (G_strcasecmp(name, "G_OPT_V_ID") == 0)
	key = G_OPT_V_ID;
    else if (G_strcasecmp(name, "G_OPT_V_IDS") == 0)
	key = G_OPT_V_IDS;
    else if (G_strcasecmp(name, "G_OPT_F_INPUT") == 0)
	key = G_OPT_F_INPUT;
    else if (G_strcasecmp(name, "G_OPT_F_OUTPUT") == 0)
	key = G_OPT_F_OUTPUT;
    else if (G_strcasecmp(name, "G_OPT_F_SEP") == 0)
	key = G_OPT_F_SEP;
    else if (G_strcasecmp(name, "G_OPT_C_FG") == 0)
	key = G_OPT_C_FG;
    else if (G_strcasecmp(name, "G_OPT_C_BG") == 0)
	key = G_OPT_C_BG;
    else if (G_strcasecmp(name, "G_OPT_M_DIR") == 0)
        key = G_OPT_M_DIR;
    else if (G_strcasecmp(name, "G_OPT_M_UNITS") == 0)
	key = G_OPT_M_UNITS;
    else if (G_strcasecmp(name, "G_OPT_M_DATATYPE") == 0)
	key = G_OPT_M_DATATYPE;
    else if (G_strcasecmp(name, "G_OPT_M_MAPSET") == 0)
	key = G_OPT_M_MAPSET;
    else if (G_strcasecmp(name, "G_OPT_M_EN") == 0)
	key = G_OPT_M_EN;
    else if (G_strcasecmp(name, "G_OPT_V_MAP") == 0)
	key = G_OPT_V_MAP;

    if (key == G_OPT_UNDEFINED)
	opt = G_define_option();
    else
	opt = G_define_standard_option(key);
    
    return opt;
}
