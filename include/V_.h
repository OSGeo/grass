#ifndef GRASS_VLIB_V_H
#define GRASS_VLIB_V_H
#ifndef _STDIO_H
#include "stdio.h"
#endif
#include <grass/Vect.h>


#define MODE_READ  0
#define MODE_WRITE 1
#define MODE_RW    2

#define VECT_OPEN_CODE   0x5522AA22
#define VECT_CLOSED_CODE 0x22AA2255

#define LEVEL_1  1
#define LEVEL_2  2
#define LEVEL_3  3

#define VECT_OPEN(Map)   (Map->open == VECT_OPEN_CODE)


/* no longer used */
struct V_info
{
    int Level;			/* 0 means unused */
    int RW;			/* 0 read, 1 write, 2, update */

    char *name;
    char *mapset;

    FILE *fp;
    FILE *att_fp;

    char *tmp_name_dig;		/* temp not yet implemented */
    char *tmp_name_att;
    FILE *tmp_fp;
    FILE *tmp_att_fp;

    struct dig_head Head;

    struct Map_info Map;
};

#endif
