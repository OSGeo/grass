/* ***************************************************************
 * *
 * * MODULE:       v.digit
 * * 
 * * AUTHOR(S):    Radim Blazek
 * *               
 * * PURPOSE:      Edit vector
 * *              
 * * COPYRIGHT:    (C) 2001 by the GRASS Development Team
 * *
 * *               This program is free software under the 
 * *               GNU General Public License (>=v2). 
 * *               Read the file COPYING that comes with GRASS
 * *               for details.
 * *
 * **************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/colors.h>
#include <grass/raster.h>
#include <grass/display.h>
#include "global.h"
#include "proto.h"


/* init variables 
 */
void var_init(void)
{
    G_debug(2, "var_init");

    G_debug(2, "Variable = %p", Variable);

    /* Note: important is that VAR_CMODE is set last, because if it is CAT_MODE_NEXT 
     *  previously set  VAR_CAT is automaticaly reset to 'next not used' for current field */
    var_seti(VAR_CAT, 1);
    var_seti(VAR_FIELD, 1);
    var_seti(VAR_CAT_MODE, CAT_MODE_NEXT);
    var_seti(VAR_INSERT, 1);

}

/* set variable value by code 
 *  return  0 ok
 *         -1 error
 */
int var_seti(int code, int iv)
{
    int i, cat;

    G_debug(5, "var_seti(): code = %d val = %d", code, iv);

    i = 0;
    while (Variable[i].name) {
	if (code == Variable[i].code) {
	    Variable[i].i = iv;

	    /* Some variables requires extra actions */
	    if ((code == VAR_FIELD &&
		 var_geti(VAR_CAT_MODE) == CAT_MODE_NEXT)) {
		cat = cat_max_get(var_geti(VAR_FIELD)) + 1;
		var_seti(VAR_CAT, cat);
	    }
	    if (code == VAR_CAT_MODE) {
		if (var_geti(VAR_CAT_MODE) == CAT_MODE_NEXT) {
		    cat = cat_max_get(var_geti(VAR_FIELD)) + 1;
		    var_seti(VAR_CAT, cat);
		}
		i_set_cat_mode();
	    }
	    i_var_seti(code, iv);	/* if called from C GUI is reset, */
	    /* if originaly called from GUI it is reset in GUI second time */
	    return 0;
	}
	i++;
    }
    G_warning("Cannot set variable code = %d", code);
    return -1;
}

/* set variable value by code 
 *  return  0 ok
 *         -1 error
 */
int var_setd(int code, double d)
{
    int i;

    i = 0;
    while (Variable[i].name) {
	if (code == Variable[i].code) {
	    Variable[i].d = d;
	    i_var_setd(code, d);
	    return 0;
	}
	i++;
    }
    G_warning("Cannot set variable code = %d", code);
    return -1;
}

/* set variable value by code 
 *  return  0 ok
 *         -1 error
 */
int var_setc(int code, char *c)
{
    int i;

    i = 0;
    while (Variable[i].name) {
	if (code == Variable[i].code) {
	    Variable[i].c = G_store(c);
	    i_var_setc(code, c);
	    return 0;
	}
	i++;
    }
    G_warning("Cannot set variable code = %d", code);
    return -1;
}

/* get variable type 
 *  return  type
 *          -1 not found
 */
int var_get_type_by_name(char *name)
{
    int i;

    G_debug(5, "var_get_type_by_name()");

    i = 0;
    while (Variable[i].name) {
	if (strcmp(name, Variable[i].name) == 0) {
	    return (Variable[i].type);
	}
	i++;
    }
    G_warning("Cannot get type of variable %s", name);
    return -1;
}

/* get variable type 
 *  return  type
 *          -1 not found
 */
int var_get_code_by_name(char *name)
{
    int i;

    G_debug(5, "var_get_code_by_name()");

    i = 0;
    while (Variable[i].name) {
	if (strcmp(name, Variable[i].name) == 0) {
	    return (Variable[i].code);
	}
	i++;
    }
    G_warning("Cannot get code of variable %s", name);
    return -1;
}

/* get pointer to variable name
 *  return  pointer to name or NULL
 */
char *var_get_name_by_code(int code)
{
    int i;

    G_debug(5, "var_get_name_by_code()");

    i = 0;
    while (Variable[i].name) {
	if (Variable[i].code == code) {
	    return (Variable[i].name);
	}
	i++;
    }
    G_warning("Cannot get name of variable %d", code);
    return NULL;
}

/* get variable value */
int var_geti(int code)
{
    int i = 0;

    while (Variable[i].name) {
	if (code == Variable[i].code) {
	    return (Variable[i].i);
	}
	i++;
    }
    G_warning("Cannot get value of variable code = %d", code);
    return 0;
}

/* get variable value */
double var_getd(int code)
{
    int i = 0;

    while (Variable[i].name) {
	if (code == Variable[i].code) {
	    return (Variable[i].d);
	}
	i++;
    }

    G_warning("Cannot get value of variable code = %d", code);
    return 0;
}

/* get variable value */
char *var_getc(int code)
{
    int i = 0;

    while (Variable[i].name) {
	if (code == Variable[i].code) {
	    return (Variable[i].c);
	}
	i++;
    }

    G_warning("Cannot get value of variable code = %d", code);
    return NULL;
}
