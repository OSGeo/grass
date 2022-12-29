/****************************************************************************
 *
 * MODULE:       test.dbmi_base.lib
 *
 * AUTHOR(S):    Original author
 *               Soeren Gebbert soerengebbert <at> googlemail <dot> com
 * 		 2010 Braunschweig, Germany
 *
 * PURPOSE:      Unit and integration tests for the dbmi_base library
 *
 * COPYRIGHT:    (C) 2007 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *   	    	License (>=v2). Read the file COPYING that comes with GRASS
 *   	    	for details.
 *
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include <grass/dbmi.h>
#include "test_dbmi_base_lib.h"

/* prototypes */
static int test_copy_column(void);

/* ************************************************************************* */
/* Performe the column unit tests  ****************************************** */
/* ************************************************************************* */
int unit_test_column(void)
{
	int sum = 0;

	G_message(_("\n++ Running column unit tests ++"));

	sum += test_copy_column();

	if (sum > 0)
		G_warning(_("\n-- column unit tests failure --"));
	else
		G_message(_("\n-- column unit tests finished successfully --"));

	return sum;
}

/* *************************************************************** */
/* Test copy dbColumn functionality ****************************** */
/* *************************************************************** */
int test_copy_column(void)
{
	int sum = 0;
        dbColumn *column, *c2;

	G_message(_("\n++ Run test copy column ++"));

        /*Test the set and copy function*/
        column = (dbColumn*)db_calloc(sizeof(dbColumn), 2);
        db_init_column(&column[0]);
        db_init_column(&column[1]);

        /*Write some initial values*/
        db_set_value_double(&column[0].defaultValue, 0.5);
        db_set_value_double(&column[0].value, 10.5);
        
        db_set_column_description(&column[0], "Test column");
        db_set_column_host_type(&column[0], 1);
        db_set_column_length(&column[0], 8);
        db_set_column_name(&column[0], "test");
        db_set_column_null_allowed(&column[0]);
        db_set_column_precision(&column[0], 20);
        db_set_column_scale(&column[0], 1);
        db_set_column_select_priv_granted(&column[0]);
        db_set_column_sqltype(&column[0], DB_SQL_TYPE_DOUBLE_PRECISION);
        db_set_column_select_priv_granted(&column[0]);
        db_set_column_update_priv_granted(&column[0]);
        db_set_column_use_default_value(&column[0]);

        /*Copy the column*/
        db_copy_column(&column[1], &column[0]);
        c2 = db_copy_column(NULL, &column[1]);

        fprintf(stdout, "##### First column:\n");
        db_print_column_definition(stdout, &column[0]);
        fprintf(stdout, "##### Second column:\n");
        db_print_column_definition(stdout, &column[1]);
        fprintf(stdout, "##### Third column:\n");
        db_print_column_definition(stdout, c2);

        /*Compare the columns*/
        if(strcmp(column[0].columnName.string, c2->columnName.string) != 0) {
            G_warning("Error copying column name");
            sum++;
        }
        if(strcmp(column[0].description.string, c2->description.string) != 0) {
            G_warning("Error copying column description");
            sum++;
        }
        if(column[0].dataLen != c2->dataLen) {
            G_warning("Error copying dataLen");
            sum++;
        }
        if(column[0].defaultValue.d != c2->defaultValue.d) {
            G_warning("Error copying default value");
            sum++;
        }
        if(column[0].hasDefaultValue != c2->hasDefaultValue) {
            G_warning("Error copying hasDefaultValue");
            sum++;
        }
        if(column[0].hostDataType != column[1].hostDataType) {
            G_warning("Error copying hostDataType");
            sum++;
        }
        if(column[0].nullAllowed != c2->nullAllowed) {
            G_warning("Error copying nullAllowed");
            sum++;
        }
        if(column[0].precision != c2->precision) {
            G_warning("Error copying precision");
            sum++;
        }
        if(column[0].scale != c2->scale) {
            G_warning("Error copying scale");
            sum++;
        }
        if(column[0].select != c2->select) {
            G_warning("Error copying select");
            sum++;
        }
        if(column[0].sqlDataType != c2->sqlDataType) {
            G_warning("Error copying sqlDataType");
            sum++;
        }
        if(column[0].update != c2->update) {
            G_warning("Error copying update");
            sum++;
        }
        if(column[0].useDefaultValue != c2->useDefaultValue) {
            G_warning("Error copying useDefaultValue");
            sum++;
        }
        if(column[0].value.d != c2->value.d) {
            G_warning("Error copying value");
            sum++;
        }

	G_message(_("\n++ Test copy column finished ++"));

	return sum;
}

