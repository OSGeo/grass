/*****************************************************************************
 *
 * MODULE:       GRASS json output interface
 *
 * AUTHOR:       Nishant Bansal (nishant.bansal.282003@gmail.com)
 *
 * PURPOSE:      parson library function wrapper
 *               part of the gjson library
 *
 * COPYRIGHT:    (C) 2024 by the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the file COPYING that comes with GRASS
 *               for details.
 *
 *****************************************************************************/

#include "gjson.h"

/* *************************************************************** */
/* ***** WRAPPER FOR PARSON FUNCTIONS USED IN GRASS ************** */
/* *************************************************************** */

JSON_Value *G_json_value_init_object(void)
{
    return json_value_init_object();
}

JSON_Value *G_json_value_init_array(void)
{
    return json_value_init_array();
}

JSON_Object *G_json_value_get_object(const JSON_Value *value)
{
    return json_value_get_object(value);
}

JSON_Object *G_json_object(const JSON_Value *value)
{
    return json_object(value);
}
JSON_Object *G_json_object_get_object(const JSON_Object *object,
                                      const char *name)
{
    return json_object_get_object(object, name);
}
JSON_Array *G_json_object_get_array(const JSON_Object *object, const char *name)
{
    return json_object_get_array(object, name);
}
JSON_Value *G_json_object_get_value(const JSON_Object *object, const char *name)
{
    return json_object_get_value(object, name);
}
const char *G_json_object_get_string(const JSON_Object *object,
                                     const char *name)
{
    return json_object_get_string(object, name);
}
double G_json_object_get_number(const JSON_Object *object, const char *name)
{
    return json_object_get_number(object, name);
}
int G_json_object_get_boolean(const JSON_Object *object, const char *name)
{
    return json_object_get_boolean(object, name);
}
JSON_Value *G_json_object_get_wrapping_value(const JSON_Object *object)
{
    return json_object_get_wrapping_value(object);
}
JSON_Status G_json_object_set_value(JSON_Object *object, const char *name,
                                    JSON_Value *value)
{
    return json_object_set_value(object, name, value);
}
JSON_Status G_json_object_set_string(JSON_Object *object, const char *name,
                                     const char *string)
{
    return json_object_set_string(object, name, string);
}
JSON_Status G_json_object_set_number(JSON_Object *object, const char *name,
                                     double number)
{
    return json_object_set_number(object, name, number);
}
JSON_Status G_json_object_set_boolean(JSON_Object *object, const char *name,
                                      int boolean)
{
    return json_object_set_boolean(object, name, boolean);
}
JSON_Status G_json_object_set_null(JSON_Object *object, const char *name)
{
    return json_object_set_null(object, name);
}
JSON_Status G_json_object_dotset_string(JSON_Object *object, const char *name,
                                        const char *string)
{
    return json_object_dotset_string(object, name, string);
}
const char *G_json_object_dotget_string(JSON_Object *object, const char *name)
{
    return json_object_dotget_string(object, name);
}
JSON_Array *G_json_array(const JSON_Value *value)
{
    return json_array(value);
}
JSON_Value *G_json_array_get_value(const JSON_Array *array, size_t index)
{
    return json_array_get_value(array, index);
}
const char *G_json_array_get_string(const JSON_Array *array, size_t index)
{
    return json_array_get_string(array, index);
}
double G_json_array_get_number(const JSON_Array *array, size_t index)
{
    return json_array_get_number(array, index);
}
int G_json_array_get_boolean(const JSON_Array *array, size_t index)
{
    return json_array_get_boolean(array, index);
}

JSON_Status G_json_array_append_value(JSON_Array *array, JSON_Value *value)
{
    return json_array_append_value(array, value);
}

JSON_Status G_json_array_append_string(JSON_Array *array, const char *string)
{
    return json_array_append_string(array, string);
}

JSON_Status G_json_array_append_number(JSON_Array *array, double number)
{
    return json_array_append_number(array, number);
}

JSON_Status G_json_array_append_boolean(JSON_Array *array, int boolean)
{
    return json_array_append_boolean(array, boolean);
}

JSON_Status G_json_array_append_null(JSON_Array *array)
{
    return json_array_append_null(array);
}

char *G_json_serialize_to_string_pretty(const JSON_Value *value)
{
    return json_serialize_to_string_pretty(value);
}

void G_json_free_serialized_string(char *string)
{
    json_free_serialized_string(string);
}
void G_json_value_free(JSON_Value *value)
{
    json_value_free(value);
}
