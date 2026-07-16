/*!
  \file lib/external/parson/gjson.c

  \brief GRASS JSON Library

  \since 8.5
*/

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
#include "parson.h"

/// \since version 8.5
typedef struct json_object_t G_json_object_t;
/// \since version 8.5
typedef struct json_array_t G_json_array_t;
/// \since version 8.5
typedef struct json_value_t G_json_value_t;

/* *************************************************************** */
/* ***** WRAPPER FOR PARSON FUNCTIONS USED IN GRASS ************** */
/* *************************************************************** */

/// \since version 8.5
G_JSON_Value *G_json_value_init_object(void)
{
    return (G_JSON_Value *)json_value_init_object();
}

/// \since version 8.5
G_JSON_Value *G_json_value_init_array(void)
{
    return (G_JSON_Value *)json_value_init_array();
}

/// \since version 8.5
G_JSON_Value_Type G_json_value_get_type(const G_JSON_Value *value)
{
    return json_value_get_type((const JSON_Value *)value);
}

/// \since version 8.5
G_JSON_Object *G_json_value_get_object(const G_JSON_Value *value)
{
    return (G_JSON_Object *)json_value_get_object((const JSON_Value *)value);
}

/// \since version 8.5
G_JSON_Object *G_json_object(const G_JSON_Value *value)
{
    return (G_JSON_Object *)json_object((const JSON_Value *)value);
}

/// \since version 8.5
G_JSON_Object *G_json_object_get_object(const G_JSON_Object *object,
                                        const char *name)
{
    return (G_JSON_Object *)json_object_get_object((const JSON_Object *)object,
                                                   name);
}

/// \since version 8.5
G_JSON_Array *G_json_object_get_array(const G_JSON_Object *object,
                                      const char *name)
{
    return (G_JSON_Array *)json_object_get_array((const JSON_Object *)object,
                                                 name);
}

/// \since version 8.5
G_JSON_Value *G_json_object_get_value(const G_JSON_Object *object,
                                      const char *name)
{
    return (G_JSON_Value *)json_object_get_value((const JSON_Object *)object,
                                                 name);
}

/// \since version 8.5
const char *G_json_object_get_string(const G_JSON_Object *object,
                                     const char *name)
{
    return json_object_get_string((const JSON_Object *)object, name);
}

/// \since version 8.5
double G_json_object_get_number(const G_JSON_Object *object, const char *name)
{
    return json_object_get_number((const JSON_Object *)object, name);
}

/// \since version 8.5
int G_json_object_get_boolean(const G_JSON_Object *object, const char *name)
{
    return json_object_get_boolean((const JSON_Object *)object, name);
}

/// \since version 8.5
G_JSON_Value *G_json_object_get_wrapping_value(const G_JSON_Object *object)
{
    return (G_JSON_Value *)json_object_get_wrapping_value(
        (const JSON_Object *)object);
}

/// \since version 8.5
G_JSON_Status G_json_object_set_value(G_JSON_Object *object, const char *name,
                                      G_JSON_Value *value)
{
    return json_object_set_value((JSON_Object *)object, name,
                                 (JSON_Value *)value);
}

/// \since version 8.5
G_JSON_Status G_json_object_set_string(G_JSON_Object *object, const char *name,
                                       const char *string)
{
    return json_object_set_string((JSON_Object *)object, name, string);
}

/// \since version 8.5
G_JSON_Status G_json_object_set_number(G_JSON_Object *object, const char *name,
                                       double number)
{
    return json_object_set_number((JSON_Object *)object, name, number);
}

/// \since version 8.5
G_JSON_Status G_json_object_set_boolean(G_JSON_Object *object, const char *name,
                                        int boolean)
{
    return json_object_set_boolean((JSON_Object *)object, name, boolean);
}

/// \since version 8.5
G_JSON_Status G_json_object_set_null(G_JSON_Object *object, const char *name)
{
    return json_object_set_null((JSON_Object *)object, name);
}

/// \since version 8.5
G_JSON_Status G_json_object_dotset_string(G_JSON_Object *object,
                                          const char *name, const char *string)
{
    return json_object_dotset_string((JSON_Object *)object, name, string);
}

/// \since version 8.5
const char *G_json_object_dotget_string(G_JSON_Object *object, const char *name)
{
    return json_object_dotget_string((JSON_Object *)object, name);
}

/// \since version 8.5
G_JSON_Status G_json_object_dotset_number(G_JSON_Object *object,
                                          const char *name, double number)
{
    return json_object_dotset_number((JSON_Object *)object, name, number);
}

/// \since version 8.5
double G_json_object_dotget_number(G_JSON_Object *object, const char *name)
{
    return json_object_dotget_number((JSON_Object *)object, name);
}

/// \since version 8.5
G_JSON_Status G_json_object_dotset_null(G_JSON_Object *object, const char *name)
{
    return json_object_dotset_null((JSON_Object *)object, name);
}

/// \since version 8.5
G_JSON_Array *G_json_array(const G_JSON_Value *value)
{
    return (G_JSON_Array *)json_array((const JSON_Value *)value);
}

/// \since version 8.5
G_JSON_Value *G_json_array_get_value(const G_JSON_Array *array, size_t index)
{
    return (G_JSON_Value *)json_array_get_value((const JSON_Array *)array,
                                                index);
}

/// \since version 8.5
const char *G_json_array_get_string(const G_JSON_Array *array, size_t index)
{
    return json_array_get_string((const JSON_Array *)array, index);
}

/// \since version 8.5
double G_json_array_get_number(const G_JSON_Array *array, size_t index)
{
    return json_array_get_number((const JSON_Array *)array, index);
}

/// \since version 8.5
int G_json_array_get_boolean(const G_JSON_Array *array, size_t index)
{
    return json_array_get_boolean((const JSON_Array *)array, index);
}

/// \since version 8.5
G_JSON_Status G_json_array_append_value(G_JSON_Array *array,
                                        G_JSON_Value *value)
{
    return json_array_append_value((JSON_Array *)array, (JSON_Value *)value);
}

/// \since version 8.5
G_JSON_Status G_json_array_append_string(G_JSON_Array *array,
                                         const char *string)
{
    return json_array_append_string((JSON_Array *)array, string);
}

/// \since version 8.5
G_JSON_Status G_json_array_append_number(G_JSON_Array *array, double number)
{
    return json_array_append_number((JSON_Array *)array, number);
}

/// \since version 8.5
G_JSON_Status G_json_array_append_boolean(G_JSON_Array *array, int boolean)
{
    return json_array_append_boolean((JSON_Array *)array, boolean);
}

/// \since version 8.5
G_JSON_Status G_json_array_append_null(G_JSON_Array *array)
{
    return json_array_append_null((JSON_Array *)array);
}

/// \since version 8.5
void G_json_set_float_serialization_format(const char *format)
{
    json_set_float_serialization_format(format);
}

/// \since version 8.5
char *G_json_serialize_to_string_pretty(const G_JSON_Value *value)
{
    return json_serialize_to_string_pretty((const JSON_Value *)value);
}

/// \since version 8.5
char *G_json_serialize_to_string(const G_JSON_Value *value)
{
    return json_serialize_to_string((const JSON_Value *)value);
}

/// \since version 8.5
void G_json_free_serialized_string(char *string)
{
    json_free_serialized_string(string);
}

/// \since version 8.5
void G_json_value_free(G_JSON_Value *value)
{
    json_value_free((JSON_Value *)value);
}
