#include "proto.h"

static char *STD_OPT_STRINGS[] = {"G_OPT_UNDEFINED",
                                  "G_OPT_DB_SQL",
                                  "G_OPT_DB_WHERE",
                                  "G_OPT_DB_TABLE",
                                  "G_OPT_DB_DRIVER",
                                  "G_OPT_DB_DATABASE",
                                  "G_OPT_DB_SCHEMA",
                                  "G_OPT_DB_COLUMN",
                                  "G_OPT_DB_COLUMNS",
                                  "G_OPT_DB_KEYCOLUMN",
                                  "G_OPT_I_GROUP",
                                  "G_OPT_I_SUBGROUP",
                                  "G_OPT_MEMORYMB",
                                  "G_OPT_R_INPUT",
                                  "G_OPT_R_INPUTS",
                                  "G_OPT_R_OUTPUT",
                                  "G_OPT_R_OUTPUTS",
                                  "G_OPT_R_MAP",
                                  "G_OPT_R_MAPS",
                                  "G_OPT_R_BASE",
                                  "G_OPT_R_COVER",
                                  "G_OPT_R_ELEV",
                                  "G_OPT_R_ELEVS",
                                  "G_OPT_R_TYPE",
                                  "G_OPT_R_INTERP_TYPE",
                                  "G_OPT_R_BASENAME_INPUT",
                                  "G_OPT_R_BASENAME_OUTPUT",
                                  "G_OPT_R3_INPUT",
                                  "G_OPT_R3_INPUTS",
                                  "G_OPT_R3_OUTPUT",
                                  "G_OPT_R3_MAP",
                                  "G_OPT_R3_MAPS",
                                  "G_OPT_R3_TYPE",
                                  "G_OPT_R3_PRECISION",
                                  "G_OPT_R3_TILE_DIMENSION",
                                  "G_OPT_R3_COMPRESSION",
                                  "G_OPT_V_INPUT",
                                  "G_OPT_V_INPUTS",
                                  "G_OPT_V_OUTPUT",
                                  "G_OPT_V_MAP",
                                  "G_OPT_V_MAPS",
                                  "G_OPT_V_TYPE",
                                  "G_OPT_V3_TYPE",
                                  "G_OPT_V_FIELD",
                                  "G_OPT_V_FIELD_ALL",
                                  "G_OPT_V_CAT",
                                  "G_OPT_V_CATS",
                                  "G_OPT_V_ID",
                                  "G_OPT_V_IDS",
                                  "G_OPT_F_INPUT",
                                  "G_OPT_F_BIN_INPUT",
                                  "G_OPT_F_OUTPUT",
                                  "G_OPT_F_SEP",
                                  "G_OPT_C",
                                  "G_OPT_CN",
                                  "G_OPT_C_FORMAT",
                                  "G_OPT_M_UNITS",
                                  "G_OPT_M_DATATYPE",
                                  "G_OPT_M_MAPSET",
                                  "G_OPT_M_LOCATION",
                                  "G_OPT_M_DBASE",
                                  "G_OPT_M_COORDS",
                                  "G_OPT_M_COLR",
                                  "G_OPT_M_DIR",
                                  "G_OPT_M_REGION",
                                  "G_OPT_M_NULL_VALUE",
                                  "G_OPT_M_NPROCS",
                                  "G_OPT_M_SEED",
                                  "G_OPT_STDS_INPUT",
                                  "G_OPT_STDS_INPUTS",
                                  "G_OPT_STDS_OUTPUT",
                                  "G_OPT_STRDS_INPUT",
                                  "G_OPT_STRDS_INPUTS",
                                  "G_OPT_STRDS_OUTPUT",
                                  "G_OPT_STRDS_OUTPUTS",
                                  "G_OPT_STR3DS_INPUT",
                                  "G_OPT_STR3DS_INPUTS",
                                  "G_OPT_STR3DS_OUTPUT",
                                  "G_OPT_STVDS_INPUT",
                                  "G_OPT_STVDS_INPUTS",
                                  "G_OPT_STVDS_OUTPUT",
                                  "G_OPT_MAP_INPUT",
                                  "G_OPT_MAP_INPUTS",
                                  "G_OPT_STDS_TYPE",
                                  "G_OPT_MAP_TYPE",
                                  "G_OPT_T_TYPE",
                                  "G_OPT_T_WHERE",
                                  "G_OPT_T_SAMPLE"};

struct Option *define_standard_option(const char *name)
{
    int key;
    size_t i;
    struct Option *opt;

    key = G_OPT_UNDEFINED;
    for (i = 1; key == G_OPT_UNDEFINED &&
                i < (sizeof(STD_OPT_STRINGS) / sizeof(char *));
         i++) {
        if (G_strcasecmp(name, STD_OPT_STRINGS[i]) == 0)
            key = i;
    }

    if (key == G_OPT_UNDEFINED)
        opt = G_define_option();
    else
        opt = G_define_standard_option(key);

    return opt;
}
