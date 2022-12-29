
/***************************************************************************
 *
 * MODULE:    r.fill.stats
 * FILE:      cell_funcs.c
 * AUTHOR(S): Benjamin Ducke
 * PURPOSE:   Provide cell type dependent functions for cell data operations.
 *            At program initialization, a function pointer is set to point to
 *            the right one of the three alternatives.
 *            This function pointer can later be used to work with cell data of
 *            different types without having to go through any switching logics.
 *
 * COPYRIGHT: (C) 2011 by the GRASS Development Team
 *
 *            This program is free software under the GPL (>=v2)
 *            Read the file COPYING that comes with GRASS for details.
 *
 ****************************************************************************
 */

#include <math.h>
#include <string.h>
#include <stdlib.h>

#include <grass/gis.h>
#include <grass/glocale.h>

#include "cell_funcs.h"


RASTER_MAP_TYPE IN_TYPE;
RASTER_MAP_TYPE OUT_TYPE;

unsigned char CELL_IN_SIZE;
unsigned char CELL_IN_PTR_SIZE;
unsigned char CELL_OUT_SIZE;
unsigned char CELL_OUT_PTR_SIZE;
unsigned char CELL_ERR_SIZE;

void (*WRITE_CELL_VAL) (void *, void *);
void (*WRITE_DOUBLE_VAL) (void *, double);
int (*IS_NULL) (void *);
void (*SET_NULL) (void *, unsigned long);

/*
 * Write cell values.
 */
void write_cell_value_c(void *cell_output, void *cell_input)
{
    Rast_set_c_value(cell_output, Rast_get_c_value(cell_input, IN_TYPE),
                     OUT_TYPE);
}

void write_cell_value_f(void *cell_output, void *cell_input)
{
    Rast_set_f_value(cell_output, Rast_get_f_value(cell_input, IN_TYPE),
                     OUT_TYPE);
}

void write_cell_value_d(void *cell_output, void *cell_input)
{
    Rast_set_d_value(cell_output, Rast_get_d_value(cell_input, IN_TYPE),
                     OUT_TYPE);
}


/*
 * Write a double value into a cell (truncates for CELL type output).
 */

void write_double_value_c(void *cell, double val)
{
    Rast_set_c_value(cell, (CELL) val, OUT_TYPE);
}

void write_double_value_f(void *cell, double val)
{
    Rast_set_f_value(cell, (FCELL) val, OUT_TYPE);
}

void write_double_value_d(void *cell, double val)
{
    Rast_set_d_value(cell, (DCELL) val, OUT_TYPE);
}



/*
 * Check for "no data".
 */
int is_null_value_c(void *cell)
{
    return (Rast_is_c_null_value((CELL *) cell));
}

int is_null_value_f(void *cell)
{
    return (Rast_is_f_null_value((FCELL *) cell));
}

int is_null_value_d(void *cell)
{
    return (Rast_is_d_null_value((DCELL *) cell));
}


/*
 * Set consecutive cells to "no data".
 */
void set_null_c(void *cells, unsigned long count)
{
    Rast_set_c_null_value((CELL *) cells, count);
}

void set_null_f(void *cells, unsigned long count)
{
    Rast_set_f_null_value((FCELL *) cells, count);
}

void set_null_d(void *cells, unsigned long count)
{
    Rast_set_d_null_value((DCELL *) cells, count);
}


/*
 * Call this init function once it is clear which input
 * and output type will be used (CELL, DCELL or FCELL).
 * I.e. right after IN_TYPE and OUT_TYPE have been set
 * to valid values;
 */
void init_cell_funcs()
{

    CELL_ERR_SIZE = sizeof(FCELL);

    if (IN_TYPE == CELL_TYPE) {
        CELL_IN_SIZE = sizeof(CELL);
        CELL_IN_PTR_SIZE = sizeof(CELL *);
        IS_NULL = &is_null_value_c;
    }
    if (IN_TYPE == FCELL_TYPE) {
        CELL_IN_SIZE = sizeof(FCELL);
        CELL_IN_PTR_SIZE = sizeof(FCELL *);
        IS_NULL = &is_null_value_f;
    }
    if (IN_TYPE == DCELL_TYPE) {
        CELL_IN_SIZE = sizeof(DCELL);
        CELL_IN_PTR_SIZE = sizeof(DCELL *);
        IS_NULL = &is_null_value_d;
    }
    if (OUT_TYPE == CELL_TYPE) {
        CELL_OUT_SIZE = sizeof(CELL);
        CELL_OUT_PTR_SIZE = sizeof(CELL *);
        WRITE_CELL_VAL = &write_cell_value_c;
        WRITE_DOUBLE_VAL = &write_double_value_c;
        SET_NULL = &set_null_c;
    }
    if (OUT_TYPE == FCELL_TYPE) {
        CELL_OUT_SIZE = sizeof(FCELL);
        CELL_OUT_PTR_SIZE = sizeof(FCELL *);
        WRITE_CELL_VAL = &write_cell_value_f;
        WRITE_DOUBLE_VAL = &write_double_value_f;
        SET_NULL = &set_null_f;
    }
    if (OUT_TYPE == DCELL_TYPE) {
        CELL_OUT_SIZE = sizeof(DCELL);
        CELL_OUT_PTR_SIZE = sizeof(DCELL *);
        WRITE_CELL_VAL = &write_cell_value_d;
        WRITE_DOUBLE_VAL = &write_double_value_d;
        SET_NULL = &set_null_d;
    }
}
