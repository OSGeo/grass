
/**
 * \file IPC.h
 *
 * \brief Types, definitions and primitives for interprocess comunication 
 *  between r.li.daemon and r.li.worker
 *
 * \author Lucio Davide Spano
 * 
 * This program is free software under the GPL (>=v2)
 * Read the COPYING file that comes with GRASS for details.
 *
 * \version 1.0
 *
 * \include stdio.h
 * 
 */
#include <unistd.h>

#define AREA 1
#define MASKEDAREA 2
#define DONE 3
#define ERROR 4
#define TERM 5

/**
 * \brief fields of message AREA
 * \member <i>aid</i> area identifier <br>
 * \member <i>x</i> x coordinate of upper left corner <br>
 * \member <i>y</i> y coordinate of upper left corner <br>
 * \member <i>rl</i> area length in rows <br>
 * \member <i>cl</i> area length in columns <br>
 */
typedef struct fields_area
{
    int aid;
    int x;
    int y;
    int rl;
    int cl;
} fields_area;

/**
 * \brief fields of message MASKEDAREA
 * \member <i>aid</i> area identifier <br>
 * \member <i>x</i> x coordinate of upper left corner <br>
 * \member <i>y</i> y coordinate of upper left corner <br>
 * \member <i>rl</i> area length in rows <br>
 * \member <i>cl</i> area length in columns <br>
 * \member <i>mask</i> name of area mask <br>
 */
typedef struct fields_maskedarea
{
    int aid;
    int x;
    int y;
    int rl;
    int cl;
    char mask[GNAME_MAX];
} fields_maskedarea;

/**
 * \brief fields of message DONE
 * \member <i>aid</i> area identifier <br>
 * \member <i>pid</i> pid of worker <br>
 * \member <i>result</i> result of analysis <br>
 */
typedef struct fields_done
{
    int aid;
    int pid;
    double res;
} fields_done;

/**
 * \brief fields of message ERROR
 * \member <i>aid</i> area identifier <br>
 * \member <i>pid</i> pid of worker <br>
 */
typedef struct fields_error
{
    int aid;
    int pid;
} fields_error;

/**
 * \brief fields of message TERM
 * \member <i>pid</i> pid of daemon <br>
 */
typedef struct fields_term
{
    int pid;
} fields_term;

/**
 * \brief field of the generic IPC message
 * \member <i>f_a</i> fields of AREA message <br>
 * \member <i>f_ma</i> fields of MASKEDAREA message <br>
 * \member <i>f_d</i> fields of DONE message <br>
 * \member <i>f_e</i> fields of ERROR message <br>
 * \member <i>f_t</i> fields of TERM message <br>
 */
typedef union fields
{
    fields_area f_a;
    fields_maskedarea f_ma;
    fields_done f_d;
    fields_error f_e;
    fields_term f_t;
} fields;

/** 
 * \brief generic IPC message
 * \member <i>type</i> type of message
 * \member <i>fields</i> fields of message
 */
typedef struct msg
{
    int type;
    fields f;
} msg;

