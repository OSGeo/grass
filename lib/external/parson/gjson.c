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

#include <stdatomic.h>
#include <stdbool.h>

#include <grass/gis.h>

#include "gjson.h"
#include "parson.h"

typedef struct json_object_t G_json_object_t;
typedef struct json_array_t G_json_array_t;
typedef struct json_value_t G_json_value_t;

#define G_PARSON_MALLOC_ATTR \
    G_ATTR_MALLOC G_ATTR_ALLOC_SIZE1(1) G_ATTR_OWNSHIP_RET G_ATTR_RET_NONNULL

static void *G__parson_malloc(size_t size) G_PARSON_MALLOC_ATTR;

static void *G__parson_malloc(size_t size)
{
    return G_malloc(size);
}

#undef G_PARSON_MALLOC_ATTR

typedef enum { UNINITIALIZED, INITIALIZING, INITIALIZED } init_state_t;

static _Atomic int parson_init_state = UNINITIALIZED;

void ensure_parson_initialized(void)
{
    if (atomic_load_explicit(&parson_init_state, memory_order_acquire) ==
        INITIALIZED) {
        return;
    }

    int expected = UNINITIALIZED;

    if (atomic_compare_exchange_strong(&parson_init_state, &expected,
                                       INITIALIZING)) {

        json_set_allocation_functions(G__parson_malloc, G_free);

        atomic_store_explicit(&parson_init_state, INITIALIZED,
                              memory_order_release);
    }
    else {
        while (atomic_load_explicit(&parson_init_state, memory_order_acquire) !=
               INITIALIZED)
            ;
    }
}

/* *************************************************************** */
/* ***** WRAPPER FOR PARSON FUNCTIONS USED IN GRASS ************** */
/* *************************************************************** */

G_JSON_Value *G_json_value_init_object(void)
{
    ensure_parson_initialized();
    return (G_JSON_Value *)json_value_init_object();
}

G_JSON_Value *G_json_value_init_array(void)
{
    ensure_parson_initialized();
    return (G_JSON_Value *)json_value_init_array();
}

G_JSON_Value_Type G_json_value_get_type(const G_JSON_Value *value)
{
    ensure_parson_initialized();
    return json_value_get_type((const JSON_Value *)value);
}

G_JSON_Object *G_json_value_get_object(const G_JSON_Value *value)
{
    ensure_parson_initialized();
    return (G_JSON_Object *)json_value_get_object((const JSON_Value *)value);
}

G_JSON_Object *G_json_object(const G_JSON_Value *value)
{
    ensure_parson_initialized();
    return (G_JSON_Object *)json_object((const JSON_Value *)value);
}

G_JSON_Object *G_json_object_get_object(const G_JSON_Object *object,
                                        const char *name)
{
    ensure_parson_initialized();
    return (G_JSON_Object *)json_object_get_object((const JSON_Object *)object,
                                                   name);
}

G_JSON_Array *G_json_object_get_array(const G_JSON_Object *object,
                                      const char *name)
{
    ensure_parson_initialized();
    return (G_JSON_Array *)json_object_get_array((const JSON_Object *)object,
                                                 name);
}

G_JSON_Value *G_json_object_get_value(const G_JSON_Object *object,
                                      const char *name)
{
    ensure_parson_initialized();
    return (G_JSON_Value *)json_object_get_value((const JSON_Object *)object,
                                                 name);
}

const char *G_json_object_get_string(const G_JSON_Object *object,
                                     const char *name)
{
    ensure_parson_initialized();
    return json_object_get_string((const JSON_Object *)object, name);
}

double G_json_object_get_number(const G_JSON_Object *object, const char *name)
{
    ensure_parson_initialized();
    return json_object_get_number((const JSON_Object *)object, name);
}

int G_json_object_get_boolean(const G_JSON_Object *object, const char *name)
{
    ensure_parson_initialized();
    return json_object_get_boolean((const JSON_Object *)object, name);
}

G_JSON_Value *G_json_object_get_wrapping_value(const G_JSON_Object *object)
{
    ensure_parson_initialized();
    return (G_JSON_Value *)json_object_get_wrapping_value(
        (const JSON_Object *)object);
}
G_JSON_Status G_json_object_set_value(G_JSON_Object *object, const char *name,
                                      G_JSON_Value *value)
{
    ensure_parson_initialized();
    return json_object_set_value((JSON_Object *)object, name,
                                 (JSON_Value *)value);
}
G_JSON_Status G_json_object_set_string(G_JSON_Object *object, const char *name,
                                       const char *string)
{
    ensure_parson_initialized();
    return json_object_set_string((JSON_Object *)object, name, string);
}
G_JSON_Status G_json_object_set_number(G_JSON_Object *object, const char *name,
                                       double number)
{
    ensure_parson_initialized();
    return json_object_set_number((JSON_Object *)object, name, number);
}
G_JSON_Status G_json_object_set_boolean(G_JSON_Object *object, const char *name,
                                        int boolean)
{
    ensure_parson_initialized();
    return json_object_set_boolean((JSON_Object *)object, name, boolean);
}

G_JSON_Status G_json_object_set_null(G_JSON_Object *object, const char *name)
{
    ensure_parson_initialized();
    return json_object_set_null((JSON_Object *)object, name);
}

G_JSON_Status G_json_object_dotset_string(G_JSON_Object *object,
                                          const char *name, const char *string)
{
    ensure_parson_initialized();
    return json_object_dotset_string((JSON_Object *)object, name, string);
}

const char *G_json_object_dotget_string(G_JSON_Object *object, const char *name)
{
    ensure_parson_initialized();
    return json_object_dotget_string((JSON_Object *)object, name);
}

G_JSON_Status G_json_object_dotset_number(G_JSON_Object *object,
                                          const char *name, double number)
{
    ensure_parson_initialized();
    return json_object_dotset_number((JSON_Object *)object, name, number);
}

double G_json_object_dotget_number(G_JSON_Object *object, const char *name)
{
    ensure_parson_initialized();
    return json_object_dotget_number((JSON_Object *)object, name);
}

G_JSON_Status G_json_object_dotset_null(G_JSON_Object *object, const char *name)
{
    ensure_parson_initialized();
    return json_object_dotset_null((JSON_Object *)object, name);
}

G_JSON_Array *G_json_array(const G_JSON_Value *value)
{
    ensure_parson_initialized();
    return (G_JSON_Array *)json_array((const JSON_Value *)value);
}

G_JSON_Value *G_json_array_get_value(const G_JSON_Array *array, size_t index)
{
    ensure_parson_initialized();
    return (G_JSON_Value *)json_array_get_value((const JSON_Array *)array,
                                                index);
}

const char *G_json_array_get_string(const G_JSON_Array *array, size_t index)
{
    ensure_parson_initialized();
    return json_array_get_string((const JSON_Array *)array, index);
}

double G_json_array_get_number(const G_JSON_Array *array, size_t index)
{
    ensure_parson_initialized();
    return json_array_get_number((const JSON_Array *)array, index);
}

int G_json_array_get_boolean(const G_JSON_Array *array, size_t index)
{
    ensure_parson_initialized();
    return json_array_get_boolean((const JSON_Array *)array, index);
}

G_JSON_Status G_json_array_append_value(G_JSON_Array *array,
                                        G_JSON_Value *value)
{
    ensure_parson_initialized();
    return json_array_append_value((JSON_Array *)array, (JSON_Value *)value);
}

G_JSON_Status G_json_array_append_string(G_JSON_Array *array,
                                         const char *string)
{
    ensure_parson_initialized();
    return json_array_append_string((JSON_Array *)array, string);
}

G_JSON_Status G_json_array_append_number(G_JSON_Array *array, double number)
{
    ensure_parson_initialized();
    return json_array_append_number((JSON_Array *)array, number);
}

G_JSON_Status G_json_array_append_boolean(G_JSON_Array *array, int boolean)
{
    ensure_parson_initialized();
    return json_array_append_boolean((JSON_Array *)array, boolean);
}

G_JSON_Status G_json_array_append_null(G_JSON_Array *array)
{
    ensure_parson_initialized();
    return json_array_append_null((JSON_Array *)array);
}

void G_json_set_float_serialization_format(const char *format)
{
    ensure_parson_initialized();
    json_set_float_serialization_format(format);
}

char *G_json_serialize_to_string_pretty(const G_JSON_Value *value)
{
    ensure_parson_initialized();
    return json_serialize_to_string_pretty((const JSON_Value *)value);
}

char *G_json_serialize_to_string(const G_JSON_Value *value)
{
    ensure_parson_initialized();
    return json_serialize_to_string((const JSON_Value *)value);
}

void G_json_free_serialized_string(char *string)
{
    ensure_parson_initialized();
    json_free_serialized_string(string);
}

void G_json_value_free(G_JSON_Value *value)
{
    ensure_parson_initialized();
    json_value_free((JSON_Value *)value);
}
