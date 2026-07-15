/*****************************************************************************
 *
 * MODULE:       GRASS Parson Output Library
 *
 * AUTHOR:       Nishant Bansal (nishant.bansal.282003@gmail.com)
 *
 * PURPOSE:      Unit tests for parson wrapper
 *
 * COPYRIGHT:    (C) 2024 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#include <string.h>

#include <grass/gis.h>
#include <grass/glocale.h>
#include <grass/gjson.h>

#include "test_gjson_lib.h"

static int test_parson_wrapper(void);

/* ************************************************************************* */
/* Perform the JSON function unit tests *************************** */
/* ************************************************************************* */
int unit_test_parson_wrapper(void)
{
    int sum = 0;

    G_message(_("\n++ Running gjson wrapper unit tests ++"));

    sum += test_parson_wrapper();

    if (sum > 0)
        G_warning(_("\n-- gjson wrapper unit tests failure --"));
    else
        G_message(_("\n-- gjson wrapper unit tests finished successfully --"));

    return sum;
}

/* *************************************************************** */
/* Test all implemented parson wrapper **************** */
/* *************************************************************** */
int test_parson_wrapper(void)
{
    int sum = 0;
    G_JSON_Value *value = NULL;
    G_JSON_Object *object = NULL;
    G_JSON_Array *array = NULL;
    char *serialized_string;

    G_message("\t * testing JSON object initialization\n");
    value = G_json_value_init_object();
    if (value == NULL) {
        G_warning("Error in G_json_value_init_object");
        sum++;
    }
    else {
        G_json_value_free(value);
    }

    G_message("\t * testing JSON array initialization\n");
    value = G_json_value_init_array();
    if (value == NULL) {
        G_warning("Error in G_json_value_init_array");
        sum++;
    }
    else {
        G_json_value_free(value);
    }

    G_message("\t * testing JSON object set and get string\n");
    value = G_json_value_init_object();
    object = G_json_value_get_object(value);
    if (object == NULL) {
        G_warning("Error in G_json_value_get_object");
        sum++;
    }
    if (G_json_object_set_string(object, TEST_OBJECT_KEY, TEST_OBJECT_VALUE) !=
        G_JSONSuccess) {
        G_warning("Error in G_json_object_set_string");
        sum++;
    }
    const char *retrieved = G_json_object_get_string(object, TEST_OBJECT_KEY);
    if (strcmp(retrieved, TEST_OBJECT_VALUE) != 0) {
        G_warning("Error in G_json_object_get_string %s != %s",
                  TEST_OBJECT_VALUE, retrieved);
        sum++;
    }
    G_json_value_free(value);

    G_message("\t * testing JSON object dotset and dotget string\n");
    value = G_json_value_init_object();
    object = G_json_value_get_object(value);
    if (object == NULL) {
        G_warning("Error in G_json_value_get_object");
        sum++;
    }
    if (G_json_object_dotset_string(object, TEST_OBJECT_DOT_KEY,
                                    TEST_OBJECT_VALUE) != G_JSONSuccess) {
        G_warning("Error in G_json_object_dotset_string");
        sum++;
    }
    retrieved = G_json_object_dotget_string(object, TEST_OBJECT_DOT_KEY);
    if (strcmp(retrieved, TEST_OBJECT_VALUE) != 0) {
        G_warning("Error in G_json_object_dotget_string %s != %s",
                  TEST_OBJECT_VALUE, retrieved);
        sum++;
    }
    G_json_value_free(value);

    G_message("\t * testing JSON object dotset and dotget number\n");
    value = G_json_value_init_object();
    object = G_json_value_get_object(value);
    if (object == NULL) {
        G_warning("Error in G_json_value_get_object");
        sum++;
    }
    if (G_json_object_dotset_number(object, TEST_OBJECT_DOT_KEY, TEST_NUMBER) !=
        G_JSONSuccess) {
        G_warning("Error in G_json_object_dotset_number");
        sum++;
    }
    double number = G_json_object_dotget_number(object, TEST_OBJECT_DOT_KEY);
    if (number != TEST_NUMBER) {
        G_warning("Error in G_json_object_dotget_number %f != %f", TEST_NUMBER,
                  number);
        sum++;
    }
    G_json_value_free(value);

    G_message("\t * testing JSON object get wrapping value");
    value = G_json_value_init_object();
    object = G_json_value_get_object(value);
    if (G_json_object_get_wrapping_value(object) != value) {
        G_warning("Error in G_json_object_get_wrapping_value");
        sum++;
    }
    G_json_value_free(value);

    G_message("\t * testing JSON object set null\n");
    value = G_json_value_init_object();
    object = G_json_value_get_object(value);
    if (G_json_object_set_null(object, TEST_OBJECT_KEY) != G_JSONSuccess) {
        G_warning("Error in G_json_object_set_null");
        sum++;
    }
    if (G_json_value_get_type(
            G_json_object_get_value(object, TEST_OBJECT_KEY)) != G_JSONNull) {
        G_warning("Error: G_json_object_set_null failed, the value type is not "
                  "null.");
        sum++;
    }
    G_json_value_free(value);

    G_message("\t * testing JSON object set and get array\n");
    value = G_json_value_init_object();
    object = G_json_value_get_object(value);
    if (G_json_object_set_value(object, TEST_OBJECT_KEY,
                                G_json_value_init_array()) != G_JSONSuccess) {
        G_warning("Error in G_json_object_set_value for array");
        sum++;
    }
    array = G_json_object_get_array(object, TEST_OBJECT_KEY);
    if (!array) {
        G_warning("Error in G_json_object_get_array");
        sum++;
    }
    G_json_value_free(value);

    G_message("\t * testing JSON object get object\n");
    value = G_json_value_init_object();
    object = G_json_value_get_object(value);
    if (G_json_object_set_value(object, TEST_OBJECT_KEY,
                                G_json_value_init_object()) != G_JSONSuccess) {
        G_warning("Error in G_json_object_set_value for nested object");
        sum++;
    }
    G_JSON_Object *nested_object =
        G_json_object_get_object(object, TEST_OBJECT_KEY);
    if (!nested_object) {
        G_warning("Error in G_json_object_get_object");
        sum++;
    }
    G_json_value_free(value);

    G_message("\t * testing JSON array append value\n");
    value = G_json_value_init_array();
    array = G_json_array(value);
    if (G_json_array_append_value(array, G_json_value_init_object()) !=
        G_JSONSuccess) {
        G_warning("Error in G_json_array_append_value");
        sum++;
    }
    G_json_value_free(value);

    G_message("\t * testing JSON array append number\n");
    value = G_json_value_init_array();
    array = G_json_array(value);
    if (G_json_array_append_number(array, TEST_NUMBER) != G_JSONSuccess) {
        G_warning("Error in G_json_array_append_number");
        sum++;
    }
    if (G_json_array_get_number(array, 0) != TEST_NUMBER) {
        G_warning("Error in G_json_array_append_number %f != %f", TEST_NUMBER,
                  G_json_array_get_number(array, 0));
        sum++;
    }
    G_json_value_free(value);

    G_message("\t * testing JSON array append boolean\n");
    value = G_json_value_init_array();
    array = G_json_array(value);
    if (G_json_array_append_boolean(array, TEST_BOOLEAN) != G_JSONSuccess) {
        G_warning("Error in G_json_array_append_boolean");
        sum++;
    }
    if (G_json_array_get_boolean(array, 0) != TEST_BOOLEAN) {
        G_warning("Error in G_json_array_append_boolean %i != %i", TEST_BOOLEAN,
                  G_json_array_get_boolean(array, 0));
        sum++;
    }
    G_json_value_free(value);

    G_message("\t * testing JSON array append null\n");
    value = G_json_value_init_array();
    array = G_json_array(value);
    if (G_json_array_append_null(array) != G_JSONSuccess) {
        G_warning("Error in G_json_array_append_null");
        sum++;
    }
    if (G_json_value_get_type(G_json_array_get_value(array, 0)) != G_JSONNull) {
        G_warning("Error in G_json_array_append_null, the value type is not "
                  "null.");
        sum++;
    }
    G_json_value_free(value);

    G_message("\t * testing JSON array append string\n");
    value = G_json_value_init_array();
    array = G_json_array(value);
    if (G_json_array_append_string(array, TEST_ARRAY_STRING) != G_JSONSuccess) {
        G_warning("Error in G_json_array_append_string");
        sum++;
    }
    if (strcmp(G_json_array_get_string(array, 0), TEST_ARRAY_STRING) != 0) {
        G_warning("Error in G_json_array_append_string %s != %s",
                  TEST_ARRAY_STRING, G_json_array_get_string(array, 0));
        sum++;
    }
    G_json_value_free(value);

    G_message("\t * testing JSON object set and get number\n");
    value = G_json_value_init_object();
    object = G_json_value_get_object(value);
    if (G_json_object_set_number(object, TEST_OBJECT_KEY, TEST_NUMBER) !=
        G_JSONSuccess) {
        G_warning("Error in G_json_object_set_number");
        sum++;
    }
    number = G_json_object_get_number(object, TEST_OBJECT_KEY);
    if (number != TEST_NUMBER) {
        G_warning("Error in G_json_object_get_number %f != %f", TEST_NUMBER,
                  number);
        sum++;
    }
    G_json_value_free(value);

    G_message("\t * testing JSON object set and get boolean\n");
    value = G_json_value_init_object();
    object = G_json_value_get_object(value);
    if (G_json_object_set_boolean(object, TEST_OBJECT_KEY, TEST_BOOLEAN) !=
        G_JSONSuccess) {
        G_warning("Error in G_json_object_set_boolean");
        sum++;
    }
    int boolean_value = G_json_object_get_boolean(object, TEST_OBJECT_KEY);
    if (boolean_value != TEST_BOOLEAN) {
        G_warning("Error in G_json_object_get_boolean %i != %i", TEST_BOOLEAN,
                  boolean_value);
        sum++;
    }
    G_json_value_free(value);

    G_message("\t * testing JSON serialization\n");
    value = G_json_value_init_object();
    object = G_json_value_get_object(value);
    G_json_object_set_string(object, TEST_OBJECT_KEY, TEST_OBJECT_VALUE);
    serialized_string = G_json_serialize_to_string_pretty(value);
    if (!serialized_string ||
        strstr(serialized_string, TEST_OBJECT_VALUE) == NULL) {
        G_warning("Error in G_json_serialize_to_string_pretty");
        sum++;
    }
    G_json_free_serialized_string(serialized_string);
    G_json_value_free(value);

    return sum;
}
